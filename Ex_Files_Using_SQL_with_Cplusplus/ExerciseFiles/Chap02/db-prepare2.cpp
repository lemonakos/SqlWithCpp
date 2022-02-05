//  db-prepare2.cpp
//  Copyright 2021 BHG [bw.org]
//  as of 2021-05-30 bw

#include <cstdio>
#include <sqlite3.h>
#include <sqlcpp.h>

constexpr const char * db_file = DB_PATH "/scratch.db";

constexpr const char * sql_drop = "DROP TABLE IF EXISTS temp";
constexpr const char * sql_create = "CREATE TABLE IF NOT EXISTS temp ( a TEXT, b TEXT, c TEXT )";
constexpr const char * sql_insert = "INSERT INTO temp VALUES (?, ?, ?)";
constexpr const char * sql_begin = "BEGIN";
constexpr const char * sql_commit = "COMMIT";
constexpr const char * sql_select_all = "SELECT * FROM temp";
constexpr const char * sql_select_where = "SELECT * FROM temp WHERE a = ?";

constexpr const char * insert_strings[] = {
    "one", "two", "three",
    "two", "three", "four",
    "three", "four", "five",
    "four", "five", "six",
    "five", "six", "seven"
};

int sql_do(sqlite3 * db, const char * sql) {
    printf("sql_do: %s\n", sql);
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    if(rc != SQLITE_OK) {
        puts(sqlite3_errstr(rc));
        exit(1);
    }
    return sqlite3_changes(db);
}

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

    puts("drop the table");
    sql_do(db, sql_drop);

    puts("create the table");
    sql_do(db, sql_create);

    puts("begin transaction");
    sql_do(db, sql_begin);

    // insert rows using prepared statement
    puts("insert rows");
    sqlite3_prepare_v2(db, sql_insert, -1, &stmt, nullptr);
    int num_params = sqlite3_bind_parameter_count(stmt);
    printf("The statement %s has %d parameter(s)\n", sql_insert, num_params);
    int num_rows = sizeof(insert_strings) / sizeof(const char *) / num_params;
    printf("num rows is %d\n", num_rows);

    int istr_index = 0;
    int rows_inserted = 0;
    for(int rownum = 0; rownum < num_rows; ++rownum) {
        for(int colnum = 0; colnum < num_params; ++colnum) {
            // sqlite3_bind_text(stmt, param_idx, param_string, param_len (or -1 for strlen), destructor or constant);
            sqlite3_bind_text(stmt, colnum + 1, insert_strings[istr_index], -1, SQLITE_STATIC);
            ++istr_index;
        }
        sqlite3_step(stmt);
        rows_inserted += sqlite3_changes(db);
        sqlite3_reset(stmt);    // reset the statement for next run
    }
    sqlite3_finalize(stmt);
    printf("insterted %d rows\n", rows_inserted);

    // commit transaction
    puts("commit transaction");
    sql_do(db, sql_commit);

    // fetch all rows
    puts("fetch all rows");
    sqlite3_prepare_v2(db, sql_select_all, -1, &stmt, nullptr);
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
    const char * param1 = "four";
    sqlite3_prepare_v2(db, sql_select_where, -1, &stmt, nullptr);
    printf("The statement %s has %d parameter(s)\n", sql_select_where, sqlite3_bind_parameter_count(stmt));

    // bind the string to the statement
    sqlite3_bind_text(stmt, 1, param1, -1, SQLITE_STATIC);

    // fetch result
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
    sql_do(db, sql_drop);

    puts("close db");
    sqlite3_close(db);
    return 0;
}
