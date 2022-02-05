//  db-query.cpp
//  Copyright 2021 BHG [bw.org]
//  as of 2021-03-23 bw

#include <cstdio>
#include <sqlite3.h>
#include <sqlcpp.h>

constexpr const char * db_file = DB_PATH "/scratch.db";

constexpr const char * sql_create =
    "DROP TABLE IF EXISTS temp;"
    "BEGIN;"
    "CREATE TABLE IF NOT EXISTS temp ( a TEXT, b TEXT, c TEXT );"
    "INSERT INTO temp VALUES ('one', 'two', 'three');"
    "INSERT INTO temp VALUES ('four', 'five', 'six');"
    "INSERT INTO temp VALUES ('seven', 'eight', 'nine');"
    "COMMIT;"
;

int main() {
    sqlite3 * db = nullptr;
    sqlite3_stmt * stmt = nullptr;
    char * errmsg = nullptr;
    
    int rc = sqlite3_open(db_file, &db);
    if(rc == SQLITE_OK) {
        puts("database open");
    } else {
        printf("sqlite3_open returned an error (%d)\n", rc);
        return 1;
    }

    // create the table
    puts("create the table");
    rc = sqlite3_exec(db, sql_create, nullptr, nullptr, &errmsg);
    if(rc != SQLITE_OK) {
        puts(errmsg);
        sqlite3_free(errmsg);
        return 1;
    }

    // prepare a statement
    puts("prepare statement");
    sqlite3_prepare_v2(db, "SELECT * FROM temp", -1, &stmt, nullptr);

    puts("fetch rows");
    int col_count = sqlite3_column_count(stmt);
    int row_count = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        printf("row %d: ", ++row_count);
        for(int i = 0; i < col_count; ++i) {
            printf("%s", sqlite3_column_text(stmt, i));
            if(i < col_count - 1) printf(", ");
        }
        puts("");
    }

    puts("finalize stmt");
    sqlite3_finalize(stmt);

    puts("drop table");
    rc = sqlite3_exec(db, "DROP TABLE IF EXISTS temp", nullptr, nullptr, &errmsg);
    if(rc != SQLITE_OK) {
        puts(errmsg);
        sqlite3_free(errmsg);
        return 1;
    }

    puts("close db");
    sqlite3_close(db);
    return 0;
}
