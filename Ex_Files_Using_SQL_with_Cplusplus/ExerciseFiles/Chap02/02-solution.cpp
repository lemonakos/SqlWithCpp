//  02-solution.cpp
//  copy one table to another
//  Copyright 2021 BHG [bw.org]
//  as of 2021-05-31 bw

#include <cstdio>
#include <sqlite3.h>
#include <sqlcpp.h>

constexpr const char * db_file = DB_PATH "/scratch.db";

constexpr const char * table1 = "temp1";
constexpr const char * table2 = "temp2";
constexpr const char * sql_drop = "DROP TABLE IF EXISTS %s";
constexpr const char * sql_create = "CREATE TABLE IF NOT EXISTS %s ( a TEXT, b TEXT, c TEXT )";
constexpr const char * sql_insert = "INSERT INTO %s VALUES (?, ?, ?)";
constexpr const char * sql_begin = "BEGIN";
constexpr const char * sql_commit = "COMMIT";
constexpr const char * sql_select_all = "SELECT * FROM %s";

constexpr const char * insert_strings[] = {
    "one", "two", "three",
    "two", "three", "four",
    "three", "four", "five",
    "four", "five", "six",
    "five", "six", "seven",
    "six", "seven", "eight",
    "seven", "eight", "nine",
    "eight", "nine", "ten",
    "nine", "ten", "eleven",
    "ten", "eleven", "twelve"
};

char sql_buf1[MAX_SMALL_STRING_LENGTH];
char sql_buf2[MAX_SMALL_STRING_LENGTH];

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
    sqlite3_stmt * stmt1 = nullptr;
    sqlite3_stmt * stmt2 = nullptr;

    int rc = sqlite3_open(db_file, &db);
    if(rc == SQLITE_OK) {
        puts("database open");
    } else {
        printf("sqlite3_open returned an error (%d)\n", rc);
        return 1;
    }

    puts("drop the tables");
    sqlite3_snprintf(MAX_SMALL_STRING_LENGTH, sql_buf1, sql_drop, table1);
    sql_do(db, sql_buf1);
    sqlite3_snprintf(MAX_SMALL_STRING_LENGTH, sql_buf2, sql_drop, table2);
    sql_do(db, sql_buf2);

    puts("create the tables");
    sqlite3_snprintf(MAX_SMALL_STRING_LENGTH, sql_buf1, sql_create, table1);
    sql_do(db, sql_buf1);
    sqlite3_snprintf(MAX_SMALL_STRING_LENGTH, sql_buf2, sql_create, table2);
    sql_do(db, sql_buf2);

    puts("populate table1");
    sql_do(db, sql_begin);
    sqlite3_snprintf(MAX_SMALL_STRING_LENGTH, sql_buf1, sql_insert, table1);
    sqlite3_prepare_v2(db, sql_buf1, -1, &stmt1, nullptr);
    int num_cols = sqlite3_bind_parameter_count(stmt1);
    printf("The statement %s has %d parameter(s)\n", sql_buf1, num_cols);
    int num_rows = sizeof(insert_strings) / sizeof(const char *) / num_cols;
    printf("num rows is %d\n", num_rows);

    int index = 0;
    int row_count = 0;
    for(int rownum = 0; rownum < num_rows; ++rownum) {
        for(int colnum = 0; colnum < num_cols; ++colnum) {
            sqlite3_bind_text(stmt1, colnum + 1, insert_strings[index], -1, nullptr);
            ++index;
        }
        sqlite3_step(stmt1);
        row_count += sqlite3_changes(db);
        sqlite3_reset(stmt1);    // reset the statement for next run
    }
    sqlite3_finalize(stmt1);
    printf("insterted %d rows into table1\n", row_count);
    puts("commit transaction");
    sql_do(db, sql_commit);

    puts("copy from table1 to table2");
    sql_do(db, sql_begin);
    sqlite3_snprintf(MAX_SMALL_STRING_LENGTH, sql_buf1, sql_select_all, table1);
    sqlite3_snprintf(MAX_SMALL_STRING_LENGTH, sql_buf2, sql_insert, table2);
    sqlite3_prepare_v2(db, sql_buf1, -1, &stmt1, nullptr);  // SELECT temp1
    sqlite3_prepare_v2(db, sql_buf2, -1, &stmt2, nullptr);  // INSERT temp2
    row_count = 0;
    int col_count = sqlite3_column_count(stmt1);
    while(sqlite3_step(stmt1) == SQLITE_ROW) {
        for(int col = 0; col < col_count; ++col) {
            const char * value = (const char *) sqlite3_column_text(stmt1, col);
            sqlite3_bind_text(stmt2, col + 1, value, -1, SQLITE_STATIC);
        }
        sqlite3_step(stmt2);
        sqlite3_reset(stmt2);
        ++row_count;
    }
    sqlite3_finalize(stmt1);
    sqlite3_finalize(stmt2);
    printf("%d rows copied\n", row_count);
    puts("commit transaction");
    sql_do(db, sql_commit);

    puts("read rows from table2");
    sqlite3_snprintf(MAX_SMALL_STRING_LENGTH, sql_buf2, sql_select_all, table2);
    sqlite3_prepare_v2(db, sql_buf2, -1, &stmt2, nullptr);
    col_count = sqlite3_column_count(stmt2);
    row_count = 0;
    while (sqlite3_step(stmt2) == SQLITE_ROW) {
        printf("  row %d: ", ++row_count);
        for (int i = 0; i < col_count; ++i) {
            printf("%s", sqlite3_column_text(stmt2, i));
            printf("%s", (i < col_count - 1) ? ", " : "\n");
        }
    }
    sqlite3_finalize(stmt2);

    puts("drop the tables");
    sqlite3_snprintf(MAX_SMALL_STRING_LENGTH, sql_buf1, sql_drop, table1);
    sql_do(db, sql_buf1);
    sqlite3_snprintf(MAX_SMALL_STRING_LENGTH, sql_buf2, sql_drop, table2);
    sql_do(db, sql_buf2);

    puts("close db");
    sqlite3_close(db);
    return 0;
}
