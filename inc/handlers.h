#ifndef HANDLERS_H
#define HANDLERS_H

#include <openssl/ssl.h>
#include <sqlite3.h>

void handle_get_person_by_cpf(SSL *ssl, sqlite3 *db, const char *cpf);
void handle_get_person_by_name(SSL *ssl, sqlite3 *db, const char *name);
void handle_get_person_by_exact_name(SSL *ssl, sqlite3 *db, const char *name);

#endif
