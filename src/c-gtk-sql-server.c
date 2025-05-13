#include <gtk/gtk.h>
#include <glib.h>
#include "gui.h"
#include "server.h"
#include "c-gtk-sql-server.h"

GMutex server_mutex;
gboolean server_running = FALSE;
GThread *server_thread = NULL;

static gpointer server_thread_func(gpointer data) {
    ServerParams *params = (ServerParams *)data;
    start_server(params->port, params->file_path);
    g_free(params->file_path);
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
        g_mutex_unlock(&server_mutex);
        if (server_thread) {
            g_thread_join(server_thread);
            server_thread = NULL;
        }
    } else {
        g_mutex_unlock(&server_mutex);
    }
}

static void activate(GtkApplication *app, gpointer user_data) {
    create_main_window(app);
}

int main(int argc, char *argv[]) {
    g_mutex_init(&server_mutex);
    GtkApplication *app = gtk_application_new("org.example.C_GTK_SQL_Server", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
