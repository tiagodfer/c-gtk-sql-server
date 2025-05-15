#include "queries.h"
#include <string.h>
#include <jansson.h>
#include <sqlite3.h>

json_t* people_by_cpf(sqlite3 *db, const char *cpf) {
    printf("[DEBUG] handle_get_person_by_cpf received cpf: '%s'\n", cpf);
    const char *sql = "SELECT cpf, nome, sexo, nasc FROM cpf WHERE cpf = ?";
    sqlite3_stmt *stmt;
    json_t *result = json_array();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, cpf, -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            json_t *entry = json_object();
            json_object_set_new(entry, "cpf", json_string((const char*)sqlite3_column_text(stmt, 0)));
            json_object_set_new(entry, "nome", json_string((const char*)sqlite3_column_text(stmt, 1)));
            json_object_set_new(entry, "sexo", json_string((const char*)sqlite3_column_text(stmt, 2)));
            json_object_set_new(entry, "nasc", json_string((const char*)sqlite3_column_text(stmt, 3)));
            json_array_append_new(result, entry);
        }
    }
    sqlite3_finalize(stmt);
    return result;
}

json_t* people_by_name(sqlite3 *db, const char *name) {
    printf("[DEBUG] handle_get_person_by_name received name: '%s'\n", name);
    const char *sql = "SELECT cpf, nome, sexo, nasc FROM cpf WHERE nome LIKE ?";
    sqlite3_stmt *stmt;
    json_t *result = json_array();
    char like_pattern[256];
    snprintf(like_pattern, sizeof(like_pattern), "%%%s%%", name);

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, like_pattern, -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            json_t *entry = json_object();
            json_object_set_new(entry, "cpf", json_string((const char*)sqlite3_column_text(stmt, 0)));
            json_object_set_new(entry, "nome", json_string((const char*)sqlite3_column_text(stmt, 1)));
            json_object_set_new(entry, "sexo", json_string((const char*)sqlite3_column_text(stmt, 2)));
            json_object_set_new(entry, "nasc", json_string((const char*)sqlite3_column_text(stmt, 3)));
            json_array_append_new(result, entry);
        }
    }
    sqlite3_finalize(stmt);
    return result;
}

json_t* people_by_exact_name(sqlite3 *db, const char *name) {
    printf("[DEBUG] handle_get_person_by_exact_name received name: '%s'\n", name);
    const char *sql = "SELECT cpf, nome, sexo, nasc FROM cpf WHERE nome = ? COLLATE NOCASE";
    sqlite3_stmt *stmt;
    json_t *result = json_array();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            json_t *entry = json_object();
            json_object_set_new(entry, "cpf", json_string((const char*)sqlite3_column_text(stmt, 0)));
            json_object_set_new(entry, "nome", json_string((const char*)sqlite3_column_text(stmt, 1)));
            json_object_set_new(entry, "sexo", json_string((const char*)sqlite3_column_text(stmt, 2)));
            json_object_set_new(entry, "nasc", json_string((const char*)sqlite3_column_text(stmt, 3)));
            json_array_append_new(result, entry);
        }
    }
    sqlite3_finalize(stmt);
    return result;
}
