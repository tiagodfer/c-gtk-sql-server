#include "gui.h"
#include "server.h"
#include "c-gtk-sql-server.h"

GtkWidget *start_button;
GtkWidget *stop_button;
GtkWidget *port_entry;
GtkWidget *file_entry;

static void on_close_clicked(GtkButton *button, gpointer window) {
    gtk_window_destroy(GTK_WINDOW(window));
}

static void on_file_selected(GObject *source, GAsyncResult *result, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    GError *error = NULL;
    GFile *file = gtk_file_dialog_open_finish(dialog, result, &error);

    if (error) {
        g_warning("File selection error: %s", error->message);
        g_error_free(error);
        return;
    }

    if (file) {
        char *filename = g_file_get_path(file);
        gtk_entry_buffer_set_text(gtk_entry_get_buffer(GTK_ENTRY(file_entry)), filename, -1);
        g_free(filename);
        g_object_unref(file);
    }
}

static void on_file_button_clicked(GtkButton *button, gpointer user_data) {
    GtkWindow *parent = GTK_WINDOW(user_data);

    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Select a Database File");

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Database Files (*.db)");
    gtk_file_filter_add_pattern(filter, "*.db");

    GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    g_object_unref(filters);

    gtk_file_dialog_open(dialog, parent, NULL, on_file_selected, NULL);
}

static void on_start_clicked(GtkButton *button, gpointer user_data) {
    const char *port_text = gtk_editable_get_text(GTK_EDITABLE(port_entry));
    int port = atoi(port_text);

    const char *file_path = gtk_editable_get_text(GTK_EDITABLE(file_entry));
    if (!file_path || !*file_path) {
        return;
    }

    ServerParams *params = g_new(ServerParams, 1);
    params->port = port;
    params->file_path = g_strdup(file_path);

    start_server_thread(params);

    g_idle_add(update_start_button_state, NULL);
    g_idle_add(update_stop_button_state, NULL);
}

static void on_stop_clicked(GtkButton *button, gpointer user_data) {
    stop_server_thread();
    g_idle_add(update_start_button_state, NULL);
    g_idle_add(update_stop_button_state, NULL);
}

gboolean update_start_button_state(gpointer data) {
    g_mutex_lock(&server_mutex);
    gtk_widget_set_sensitive(start_button, !server_running);
    g_mutex_unlock(&server_mutex);
    return G_SOURCE_REMOVE;
}

gboolean update_stop_button_state(gpointer data) {
    g_mutex_lock(&server_mutex);
    gtk_widget_set_sensitive(stop_button, server_running);
    g_mutex_unlock(&server_mutex);
    return G_SOURCE_REMOVE;
}

void create_main_window(GtkApplication *app) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "C GTK SQL Server");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 100);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_window_set_child(GTK_WINDOW(window), grid);

    file_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(file_entry), "Choose a file...");
    gtk_editable_set_editable(GTK_EDITABLE(file_entry), FALSE);
    gtk_grid_attach(GTK_GRID(grid), file_entry, 0, 0, 1, 1);

    GtkWidget *file_button = gtk_button_new_with_label("CPF Database");
    g_signal_connect(file_button, "clicked", G_CALLBACK(on_file_button_clicked), window);
    gtk_grid_attach(GTK_GRID(grid), file_button, 1, 0, 1, 1);

    GtkWidget *port_label = gtk_label_new("Port:");
    port_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(port_entry), "Enter port");
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(port_entry));
    gtk_entry_buffer_set_text(buffer, "5050", -1);
    gtk_grid_attach(GTK_GRID(grid), port_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), port_entry, 1, 1, 1, 1);

    start_button = gtk_button_new_with_label("Start Server");
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), start_button, 0, 2, 1, 1);

    stop_button = gtk_button_new_with_label("Stop Server");
    g_signal_connect(stop_button, "clicked", G_CALLBACK(on_stop_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), stop_button, 1, 2, 1, 1);
    gtk_widget_set_sensitive(stop_button, FALSE);

    GtkWidget *close_button = gtk_button_new_with_label("Close");
    g_signal_connect(close_button, "clicked", G_CALLBACK(on_close_clicked), window);
    gtk_grid_attach(GTK_GRID(grid), close_button, 0, 3, 2, 1);

    gtk_window_present(GTK_WINDOW(window));
}
