#include "gui.h"
#include "server.h"
#include "c-gtk-sql-server.h"
#include "globals.h"
#include <ifaddrs.h>
#include <arpa/inet.h>

static GtkWidget *start_button;
static GtkWidget *stop_button;
static GtkWidget *port_entry;
static GtkWidget *cpf_entry;
static GtkWidget *cnpj_entry;
static GtkWidget *interface_dropdown;
static GPtrArray *interface_ips = NULL;

static void on_close_clicked(GtkButton *button, gpointer window) {
    (void)button;
    gtk_window_destroy(GTK_WINDOW(window));
}

void populate_interface_dropdown() {
    struct ifaddrs *ifaddr, *ifa;
    char addr[INET_ADDRSTRLEN];

    if (interface_ips) {
        g_ptr_array_free(interface_ips, TRUE);
    }
    interface_ips = g_ptr_array_new_with_free_func(g_free);

    GtkStringList *list = gtk_string_list_new(NULL);

    gtk_string_list_append(list, "All interfaces (0.0.0.0)");
    g_ptr_array_add(interface_ips, g_strdup("0.0.0.0"));

    gtk_string_list_append(list, "Localhost (127.0.0.1)");
    g_ptr_array_add(interface_ips, g_strdup("127.0.0.1"));

    if (getifaddrs(&ifaddr) != -1) {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                void *in_addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
                inet_ntop(AF_INET, in_addr, addr, sizeof(addr));
                char *entry = g_strdup_printf("%s (%s)", ifa->ifa_name, addr);
                gtk_string_list_append(list, entry);
                g_ptr_array_add(interface_ips, g_strdup(addr));
                g_free(entry);
            }
        }
        freeifaddrs(ifaddr);
    }

    gtk_drop_down_set_model(GTK_DROP_DOWN(interface_dropdown), G_LIST_MODEL(list));
    g_object_unref(list);

    gtk_drop_down_set_selected(GTK_DROP_DOWN(interface_dropdown), 0);
}


static void on_cpf_selected(GObject *source, GAsyncResult *result, gpointer data) {
    (void)data;
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
        gtk_entry_buffer_set_text(gtk_entry_get_buffer(GTK_ENTRY(cpf_entry)), filename, -1);
        g_free(filename);
        g_object_unref(file);
    }
}

static void on_cpf_button_clicked(GtkButton *button, gpointer data) {
    (void)button;
    GtkWindow *parent = GTK_WINDOW(data);

    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Select a CPF Database File");

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Database Files (*.db)");
    gtk_file_filter_add_pattern(filter, "*.db");

    GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    g_object_unref(filters);

    gtk_file_dialog_open(dialog, parent, NULL, on_cpf_selected, NULL);
}

static void on_cnpj_selected(GObject *source, GAsyncResult *result, gpointer data) {
    (void)data;
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
        gtk_entry_buffer_set_text(gtk_entry_get_buffer(GTK_ENTRY(cnpj_entry)), filename, -1);
        g_free(filename);
        g_object_unref(file);
    }
}

static void on_cnpj_button_clicked(GtkButton *button, gpointer data) {
    (void)button;
    GtkWindow *parent = GTK_WINDOW(data);

    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Select a CNPJ Database File");

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Database Files (*.db)");
    gtk_file_filter_add_pattern(filter, "*.db");

    GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    g_object_unref(filters);

    gtk_file_dialog_open(dialog, parent, NULL, on_cnpj_selected, NULL);
}

gboolean update_start_button_state(gpointer data) {
    (void)data;
    g_mutex_lock(&server_mutex);
    gtk_widget_set_sensitive(start_button, !server_running);
    g_mutex_unlock(&server_mutex);
    return G_SOURCE_REMOVE;
}

gboolean update_stop_button_state(gpointer data) {
    (void)data;
    g_mutex_lock(&server_mutex);
    gtk_widget_set_sensitive(stop_button, server_running);
    g_mutex_unlock(&server_mutex);
    return G_SOURCE_REMOVE;
}

static void on_start_clicked(GtkButton *button, gpointer data) {
    (void)button;
    (void)data;

    const char *port_text = gtk_editable_get_text(GTK_EDITABLE(port_entry));
    int port = atoi(port_text);
    const char *cpf_path = gtk_editable_get_text(GTK_EDITABLE(cpf_entry));
    if (!cpf_path || !*cpf_path) {
        gtk_widget_add_css_class(cpf_entry, "error");
        return;
    } else {
        gtk_widget_remove_css_class(cpf_entry, "error");
    }
    const char *cnpj_path = gtk_editable_get_text(GTK_EDITABLE(cnpj_entry));
    if (!cnpj_path || !*cnpj_path) {
        gtk_widget_add_css_class(cnpj_entry, "error");
        return;
    } else {
        gtk_widget_remove_css_class(cnpj_entry, "error");
    }

    guint selected_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(interface_dropdown));
    const char *interface_ip = g_ptr_array_index(interface_ips, selected_idx);

    ServerParams *params = g_new(ServerParams, 1);
    params->port = port;
    params->cpf_path = g_strdup(cpf_path);
    params->cnpj_path = g_strdup(cnpj_path);
    params->interface = g_strdup(interface_ip);

    start_server_thread(params);

    g_idle_add(update_start_button_state, NULL);
    g_idle_add(update_stop_button_state, NULL);
}

static void on_stop_clicked(GtkButton *button, gpointer data) {
    (void)button;
    (void)data;
    stop_server_thread();
    g_idle_add(update_start_button_state, NULL);
    g_idle_add(update_stop_button_state, NULL);
}

void create_main_window(GtkApplication *app) {
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css_provider, ".error { outline: 2px solid red; }");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(css_provider);

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "C GTK SQL Server");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 100);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_window_set_child(GTK_WINDOW(window), grid);

    cpf_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(cpf_entry), "Choose a file...");
    gtk_editable_set_editable(GTK_EDITABLE(cpf_entry), FALSE);
    gtk_grid_attach(GTK_GRID(grid), cpf_entry, 0, 0, 1, 1);

    GtkWidget *cpf_button = gtk_button_new_with_label("CPF Database");
    g_signal_connect(cpf_button, "clicked", G_CALLBACK(on_cpf_button_clicked), window);
    gtk_grid_attach(GTK_GRID(grid), cpf_button, 1, 0, 1, 1);

    cnpj_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(cnpj_entry), "Choose a file...");
    gtk_editable_set_editable(GTK_EDITABLE(cnpj_entry), FALSE);
    gtk_grid_attach(GTK_GRID(grid), cnpj_entry, 0, 1, 1, 1);

    GtkWidget *cnpj_button = gtk_button_new_with_label("CNPJ Database");
    g_signal_connect(cnpj_button, "clicked", G_CALLBACK(on_cnpj_button_clicked), window);
    gtk_grid_attach(GTK_GRID(grid), cnpj_button, 1, 1, 1, 1);

    GtkWidget *interface_label = gtk_label_new("Interface:");
    interface_dropdown = gtk_drop_down_new(NULL, NULL);
    gtk_grid_attach(GTK_GRID(grid), interface_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), interface_dropdown, 1, 2, 1, 1);
    populate_interface_dropdown();

    GtkWidget *port_label = gtk_label_new("Port:");
    port_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(port_entry), "Enter port");
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(port_entry));
    gtk_entry_buffer_set_text(buffer, "5050", -1);
    gtk_grid_attach(GTK_GRID(grid), port_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), port_entry, 1, 3, 1, 1);

    start_button = gtk_button_new_with_label("Start Server");
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), start_button, 0, 4, 1, 1);

    stop_button = gtk_button_new_with_label("Stop Server");
    g_signal_connect(stop_button, "clicked", G_CALLBACK(on_stop_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), stop_button, 1, 4, 1, 1);
    gtk_widget_set_sensitive(stop_button, FALSE);

    GtkWidget *close_button = gtk_button_new_with_label("Close");
    g_signal_connect(close_button, "clicked", G_CALLBACK(on_close_clicked), window);
    gtk_grid_attach(GTK_GRID(grid), close_button, 0, 5, 2, 1);

    gtk_window_present(GTK_WINDOW(window));
}
