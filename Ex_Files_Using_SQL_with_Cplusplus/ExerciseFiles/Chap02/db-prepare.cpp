//  db-prepare.cpp
//  Copyright 2021 BHG [bw.org]
//  as of 2021-05-30 bw

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
    
    int rc = sqlite3_open(db_file, &db);
    if(rc == SQLITE_OK) {
        puts("database open");
    } else {
        printf("sqlite3_open returned an error (%d)\n", rc);
        return 1;
    }

    // create the table
    puts("create the table");
    rc = sqlite3_exec(db, sql_create, nullptr, nullptr, nullptr);
    if(rc != SQLITE_OK) {
        puts(sqlite3_errmsg(db));
        return 1;
    }

    // fetch all rows
    puts("fetch all rows");
    sqlite3_prepare_v2(db, "SELECT * FROM temp", -1, &stmt, nullptr);
    int col_count = sqlite3_column_count(stmt);
    int row_count = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        printf("  row %d: ", ++row_count);
        for(int i = 0; i < col_count; ++i) {
            printf("%s", sqlite3_column_text(stmt, i));
            printf("%s", (i < col_count - 1) ? ", " : "\n" );
        }
    }
    sqlite3_finalize(stmt);

    // find a row
    puts("find row with a='four'");
    const char * sql_prepare = "SELECT * FROM temp WHERE a = ?";
    const char * param1 = "four";
    sqlite3_prepare_v2(db, sql_prepare, -1, &stmt, nullptr);
    printf("statement: %s\n", sql_prepare);
    printf("the statement has %d parameter(s)\n", sqlite3_bind_parameter_count(stmt));

    // bind the string to the statement
    // sqlite3_bind_text(stmt, param_position, param_string, param_length (or -1 for strlen), destructor or constant);
    sqlite3_bind_text(stmt, 1, param1, -1, SQLITE_STATIC);
    col_count = sqlite3_column_count(stmt);
    row_count = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        printf("  row %d: ", ++row_count);
        for(int i = 0; i < col_count; ++i) {
            printf("%s", sqlite3_column_text(stmt, i));
            printf("%s", (i < col_count - 1) ? ", " : "\n" );
        }
    }
    sqlite3_finalize(stmt);

    puts("drop table");
    rc = sqlite3_exec(db, "DROP TABLE IF EXISTS temp", nullptr, nullptr, nullptr);
    if(rc != SQLITE_OK) {
        puts(sqlite3_errmsg(db));
        return 1;
    }

    puts("close db");
    sqlite3_close(db);
    return 0;
}
