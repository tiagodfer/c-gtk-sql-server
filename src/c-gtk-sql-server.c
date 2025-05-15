#include <gtk/gtk.h>
#include <glib.h>
#include "gui.h"
#include "server.h"
#include "c-gtk-sql-server.h"
#include "globals.h"

static gpointer server_thread_func(gpointer data) {
    ServerParams *params = (ServerParams *)data;
    start_server(params->port, params->cpf_path, params->cnpj_path, params->interface);
    g_free(params->cpf_path);
    g_free(params->cnpj_path);
    g_free(params);
    return NULL;
}

void start_server_thread(ServerParams *params) {
    g_mutex_lock(&server_mutex);
    if (!server_running) {
        server_running = TRUE;
        server_thread = g_thread_new("server", server_thread_func, params);
    }
    g_mutex_unlock(&server_mutex);
}

void stop_server_thread() {
    g_mutex_lock(&server_mutex);
    if (server_running) {
        server_running = FALSE;
        stop_server();

        GThread *thread_to_join = server_thread;
        server_thread = NULL;
        g_mutex_unlock(&server_mutex);

        if (thread_to_join) {
            g_thread_join(thread_to_join);
        }
    } else {
        g_mutex_unlock(&server_mutex);
    }
}

static void activate(GtkApplication *app, gpointer data) {
    (void)data;
    create_main_window(app);
}

int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new("org.example.C_GTK_SQL_Server", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
