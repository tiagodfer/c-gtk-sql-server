#ifndef GTK_SERVER_H
#define GTK_SERVER_H

#include <glib.h>
#include <gtk/gtk.h>

typedef struct {
    int port;
    char *file_path;
} ServerParams;

void start_server_thread(ServerParams *params);
void stop_server_thread(void);

#endif // GTK_SERVER_H
