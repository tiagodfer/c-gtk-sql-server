#ifndef QUERIES_H
#define QUERIES_H

#include <sqlite3.h>
#include <jansson.h>

json_t* people_by_cpf(sqlite3 *db, const char *cpf);
json_t* people_by_name(sqlite3 *db, const char *name);
json_t* people_by_exact_name(sqlite3 *db, const char *name);

#endif
