#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>

void create_main_window(GtkApplication *app);
gboolean update_start_button_state(gpointer data);
gboolean update_stop_button_state(gpointer data);

#endif
