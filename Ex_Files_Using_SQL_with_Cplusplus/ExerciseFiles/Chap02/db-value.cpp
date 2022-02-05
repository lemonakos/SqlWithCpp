//  db-value.cpp
//  Copyright 2021 BHG [bw.org]
//  as of 2021-05-30 bw

#include <cstdio>
#include <sqlite3.h>
#include <sqlcpp.h>

constexpr const char * db_file = DB_PATH "/scratch.db";

const char * sql_get_value_str(sqlite3 * db, const char * sql) {
    static char * result[MAX_SMALL_STRING_LENGTH];
    memset(result, 0, MAX_SMALL_STRING_LENGTH);

    // lambda callback for sqlite3_exec
    auto callback = [](void* dest, int numcols, char** data, char** columns) {
        memcpy(dest, data[0], strnlen(data[0], MAX_SMALL_STRING_LENGTH - 1));
        return SQLITE_OK;
    };

    int rc = sqlite3_exec(db, sql, callback, (void *) result, nullptr);
    if(rc != SQLITE_OK) {
        printf("sqlite3_exec: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }
    return (const char *) result;
}

int main() {
    sqlite3 * db = nullptr;
    const char * version_sql = "SELECT sqlite_version()";
    const char * count_sql = "SELECT COUNT(*) FROM customer";

    int rc = sqlite3_open(db_file, &db);
    if(rc) {
        printf("sqlite3_open error: %s\n", sqlite3_errstr(rc));
        exit(1);
    }

    printf("SQLite version %s\n", sql_get_value_str(db, version_sql));
    printf("Rows in customer table: %s\n", sql_get_value_str(db, count_sql));

    sqlite3_close(db);
    return 0;
}
