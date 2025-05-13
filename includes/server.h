#ifndef SERVER_H
#define SERVER_H

#include <glib.h>

extern GMutex server_mutex;
extern gboolean server_running;

int start_server(int port, const char *cpf_path);
int stop_server();

#endif
