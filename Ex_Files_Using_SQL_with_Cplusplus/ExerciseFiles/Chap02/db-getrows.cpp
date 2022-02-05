//  db-getrows.cpp
//  Copyright 2021 BHG [bw.org]
//  as of 2021-05-30 bw

#include <cstdio>
#include <sqlite3.h>
#include <sqlcpp.h>

constexpr const char * db_file = DB_PATH "/scratch.db";

int main() {
    sqlite3 * db = nullptr;
    int rc = sqlite3_open(db_file, &db);
    if(rc != SQLITE_OK) {
        printf("sqlite3_open returned an error (%d)\n", rc);
        return 1;
    }

    sqlite3_stmt * stmt = nullptr;
    rc = sqlite3_prepare_v2(db, "SELECT * FROM customer", -1, &stmt, nullptr);
    if(rc != SQLITE_OK) {
        printf("sqlite3_prepare_v2 returned %d\n", rc);
        sqlite3_close(db);
        exit(1);
    }

    int col_count = sqlite3_column_count(stmt);
    int row_count = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        for(int i = 0; i < col_count; ++i) {
            fputs((const char *) sqlite3_column_text(stmt, i), stdout);
            if(i < col_count - 1) fputs(", ", stdout);
        }
        ++row_count;
        puts("");
    }
    printf("rows retrieved: %d\n", row_count);
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}
