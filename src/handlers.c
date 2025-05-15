#include "handlers.h"
#include "queries.h"
#include "server.h"
#include <jansson.h>

void handle_get_person_by_cpf(SSL *ssl, sqlite3 *db, const char *cpf) {
    const char *headers = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: application/json\r\n"
                          "Transfer-Encoding: chunked\r\n"
                          "Connection: close\r\n\r\n";
    SSL_write(ssl, headers, strlen(headers));

    json_t *result = people_by_cpf(db, cpf);
    char *results_json = json_dumps(result, JSON_COMPACT);
    char final_chunk[2048];

    snprintf(final_chunk, sizeof(final_chunk), "{\"results\":%s}", results_json);
    send_chunk(ssl, final_chunk);
    SSL_write(ssl, "0\r\n\r\n", 5);

    free(results_json);
    json_decref(result);
    printf("[CLIENT] CPF search completed for: %s\n", cpf);
}

void handle_get_person_by_name(SSL *ssl, sqlite3 *db, const char *name) {
    const char *headers = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: application/json\r\n"
                          "Transfer-Encoding: chunked\r\n"
                          "Connection: close\r\n\r\n";
    SSL_write(ssl, headers, strlen(headers));

    send_chunk(ssl, "{\"status\":\"searching\",\"message\":\"Iniciando busca...\",\"progress\":0,\"isComplete\":false}");
    send_chunk(ssl, "{\"status\":\"searching\",\"progress\":25,\"isComplete\":false}");
    send_chunk(ssl, "{\"status\":\"processing\",\"progress\":75,\"isComplete\":false}");

    json_t *result = people_by_name(db, name);
    char *results_json = json_dumps(result, JSON_COMPACT);
    char final_chunk[2048];

    snprintf(final_chunk, sizeof(final_chunk), 
        "{\"status\":\"complete\",\"progress\":100,\"isComplete\":true,\"results\":%s}", 
        results_json);
    send_chunk(ssl, final_chunk);
    SSL_write(ssl, "0\r\n\r\n", 5);

    free(results_json);
    json_decref(result);
    printf("[CLIENT] Name search completed for: %s\n", name);
}

void handle_get_person_by_exact_name(SSL *ssl, sqlite3 *db, const char *name) {
    const char *headers = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: application/json\r\n"
                          "Transfer-Encoding: chunked\r\n"
                          "Connection: close\r\n\r\n";
    SSL_write(ssl, headers, strlen(headers));

    json_t *result = people_by_exact_name(db, name);
    char *results_json = json_dumps(result, JSON_COMPACT);
    char final_chunk[2048];

    snprintf(final_chunk, sizeof(final_chunk), "{\"results\":%s}", results_json);
    send_chunk(ssl, final_chunk);
    SSL_write(ssl, "0\r\n\r\n", 5);

    free(results_json);
    json_decref(result);
    printf("[CLIENT] Exact name search completed for: %s\n", name);
}
