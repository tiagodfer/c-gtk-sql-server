#ifndef GLOBALS_H
#define GLOBALS_H

#include <glib.h>

typedef struct {
    char *cpf_path;
    char *cnpj_path;
    int port;
    char *interface;
} ServerParams;

extern GMutex server_mutex;
extern gboolean server_running;
extern GThread *server_thread;

#endif
