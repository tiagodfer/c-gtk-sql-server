#include "server.h"
#include "queries.h"
#include "globals.h"
#include "handlers.h"
#include <glib.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sqlite3.h>
#include <jansson.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

typedef struct {
    int client_sock;
    char* cpf_path;
    char* cnpj_path;
} ThreadData;

GMutex server_mutex;
gboolean server_running = FALSE;
GThread *server_thread = NULL;

static int server_sockfd = -1;
static SSL_CTX *ssl_ctx = NULL;

void send_chunk(SSL *ssl, const char *data) {
    char chunk_header[32];
    size_t data_len = strlen(data);
    snprintf(chunk_header, sizeof(chunk_header), "%zx\r\n", data_len);
    SSL_write(ssl, chunk_header, strlen(chunk_header));
    SSL_write(ssl, data, data_len);
    SSL_write(ssl, "\r\n", 2);
}

static void handle_client(SSL *ssl, sqlite3 *cpf_db, sqlite3 *cnpj_db) {
    (void)cnpj_db; // remove this for cnpj queries
    char buffer[4096];
    ssize_t bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    if (bytes <= 0) {
        printf("[CLIENT] Failed to read request or connection closed\n");
        return;
    }

    buffer[bytes] = '\0';
    printf("[CLIENT] Received request: %s\n", buffer);

    char method[16], path[256];
    if (sscanf(buffer, "%15s %255s", method, path) != 2) {
        const char *bad_req = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        SSL_write(ssl, bad_req, strlen(bad_req));
        return;
    }

    if (strcmp(method, "GET") != 0) {
        const char *bad_method = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
        SSL_write(ssl, bad_method, strlen(bad_method));
        return;
    }

    const char *cpf_prefix = "/get-person-by-cpf/";
    const char *name_prefix = "/get-person-by-name/";
    const char *exact_name_prefix = "/get-person-by-exact-name/";

    if (strncmp(path, cpf_prefix, strlen(cpf_prefix)) == 0) {
        const char *cpf_number = path + strlen(cpf_prefix);
        handle_get_person_by_cpf(ssl, cpf_db, cpf_number);
        return;
    } else if (strncmp(path, name_prefix, strlen(name_prefix)) == 0) {
        const char *name = path + strlen(name_prefix);
        handle_get_person_by_name(ssl, cpf_db, name);
        return;
    } else if (strncmp(path, exact_name_prefix, strlen(exact_name_prefix)) == 0) {
        const char *name = path + strlen(exact_name_prefix);
        handle_get_person_by_exact_name(ssl, cpf_db, name);
        return;
    }

    const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    SSL_write(ssl, not_found, strlen(not_found));
}

static gpointer handle_client_thread(gpointer data) {
    ThreadData *thread_data = (ThreadData *)data;
    int client_sock = thread_data->client_sock;
    char *cpf_path = thread_data->cpf_path;
    char *cnpj_path = thread_data->cnpj_path;

    SSL *ssl = SSL_new(ssl_ctx);
    SSL_set_fd(ssl, client_sock);

    printf("[THREAD] Starting SSL handshake for client fd=%d\n", client_sock);
    if (SSL_accept(ssl) <= 0) {
        printf("[THREAD] SSL handshake failed for client fd=%d\n", client_sock);
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(client_sock);
        g_free(thread_data->cpf_path);
        g_free(thread_data->cnpj_path);
        g_free(thread_data);
        return NULL;
    }

    sqlite3 *cpf_db;
    if (sqlite3_open_v2(cpf_path, &cpf_db, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX, NULL) != SQLITE_OK) {
        fprintf(stderr, "[THREAD] Database error: %s\n", sqlite3_errmsg(cpf_db));
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client_sock);
        g_free(thread_data->cpf_path);
        g_free(thread_data);
        return NULL;
    }
    sqlite3_exec(cpf_db, "PRAGMA synchronous = NORMAL;", NULL, NULL, NULL);
    sqlite3_exec(cpf_db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);

    sqlite3 *cnpj_db;
    if (sqlite3_open_v2(cnpj_path, &cnpj_db, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX, NULL) != SQLITE_OK) {
        fprintf(stderr, "[THREAD] Database error: %s\n", sqlite3_errmsg(cnpj_db));
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client_sock);
        g_free(thread_data->cnpj_path);
        g_free(thread_data);
        return NULL;
    }
    sqlite3_exec(cnpj_db, "PRAGMA synchronous = NORMAL;", NULL, NULL, NULL);
    sqlite3_exec(cnpj_db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);

    printf("[THREAD] SSL handshake successful for client fd=%d\n", client_sock);
    handle_client(ssl, cpf_db, cnpj_db);

    printf("[THREAD] Closing connection for client fd=%d\n", client_sock);
    SSL_shutdown(ssl);
    SSL_free(ssl);
    sqlite3_close(cpf_db);
    sqlite3_close(cnpj_db);
    close(client_sock);
    g_free(thread_data->cpf_path);
    g_free(thread_data->cnpj_path);
    g_free(thread_data);
    return NULL;
}

int start_server(int port, const char *cpf_path, const char *cnpj_path, const char *interface) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, interface, &addr.sin_addr) != 1) {
        fprintf(stderr, "[SERVER] Invalid interface IP: %s\n", interface);
        return -1;
    }

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ssl_ctx = SSL_CTX_new(TLS_server_method());

    if (!ssl_ctx ||
        !SSL_CTX_use_certificate_file(ssl_ctx, "cert.pem", SSL_FILETYPE_PEM) ||
        !SSL_CTX_use_PrivateKey_file(ssl_ctx, "key.pem", SSL_FILETYPE_PEM)) {
        ERR_print_errors_fp(stderr);
        stop_server();
        return -1;
    }

    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
        bind(server_sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0 ||
        listen(server_sockfd, 10) < 0) {
        perror("Server error");
        stop_server();
        return -1;
    }

    printf("[SERVER] Started on %s:%i.\n", interface, port);
    while (TRUE) {
        struct timeval tv = {1, 0};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(server_sockfd, &fds);

        int sel_result = select(server_sockfd+1, &fds, NULL, NULL, &tv);
        g_mutex_lock(&server_mutex);
        if (!server_running) {
            g_mutex_unlock(&server_mutex);
            break;
        }
        g_mutex_unlock(&server_mutex);

        if (sel_result > 0) {
            int client_sock = accept(server_sockfd, NULL, NULL);
            if (client_sock < 0) continue;

            printf("[SERVER] Accepted connection (fd=%d)\n", client_sock);
            ThreadData *data = g_new(ThreadData, 1);
            data->client_sock = client_sock;
            data->cpf_path = g_strdup(cpf_path);
            data->cnpj_path = g_strdup(cnpj_path);

            GThread *client_thread = g_thread_new(
                "client_handler",
                (GThreadFunc)handle_client_thread,
                data
            );
            g_thread_unref(client_thread);
        }
    }

    close(server_sockfd);
    server_sockfd = -1;
    return 0;
}

int stop_server() {
    if (server_sockfd != -1) {
        close(server_sockfd);
        server_sockfd = -1;
    }
    if (ssl_ctx) {
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = NULL;
    }
    printf("[SERVER] Stopped\n");
    return 0;
}
