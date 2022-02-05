//  bwsql-test.cpp
//  Copyright 2021 BHG [bw.org]
//  as of 2021-06-01 bw

#include <cstdio>
#include "BWSQL.h"

constexpr const char * db_file = DB_PATH "/scratch.db";

constexpr const char * sql_create = "CREATE TABLE IF NOT EXISTS temp"
                                    "( id INTEGER PRIMARY KEY, a TEXT, b TEXT, c TEXT )";
constexpr const char * sql_drop =   "DROP TABLE IF EXISTS temp";
constexpr const char * sql_begin = "BEGIN";
constexpr const char * sql_commit = "COMMIT";
constexpr const char * sql_insert = "INSERT INTO temp (a,b,c) VALUES (?, ?, ?)";
constexpr const char * sql_select_all = "SELECT * FROM temp";
constexpr const char * sql_select_where = "SELECT * FROM temp WHERE a = ?";
constexpr const char * sql_select_count = "SELECT COUNT(*) FROM temp";

constexpr const char * insert_strings[] = {
    "one",   "two",   "three",
    "two",   "three", "four",
    "three", "four",  "five",
    "four",  "five",  "six",
    "five",  "six",   "seven"
};

// print all rows from a prepared statement
void disp_rows(bw::BWSQL & db) {
    for(const char ** row = db.fetch_row(); row; row = db.fetch_row()) {
        for(int i = 0; i < db.num_sql_columns(); ++i) {
            printf("%s%s", row[i], (i < db.num_sql_columns() - 1) ? ", " : "\n");
        }
    }
}

int main() {
    bw::BWSQL db(db_file);

    printf("BWSQL version: %s, SQLite version: %s\n", db.version(), db.sqlite_version());

    puts(sql_drop);
    db.sql_do(sql_drop);
    puts(sql_create);
    db.sql_do(sql_create);

    puts(sql_begin);
    db.sql_do(sql_begin);

    puts("insert rows");
    int num_cols = db.num_params(sql_insert);
    printf("there are %d bind parameters in '%s'\n", num_cols, sql_insert);

    int num_rows = sizeof(insert_strings) / sizeof(const char *) / num_cols;
    int index = 0;
    int count = 0;
    for(int row_no = 0; row_no < num_rows; ++row_no) {
        const char * col1 = insert_strings[index++];
        const char * col2 = insert_strings[index++];
        const char * col3 = insert_strings[index++];
        count += db.sql_do(sql_insert, col1, col2, col3);
    }
    printf("%d rows inserted\n", count);
    puts(sql_commit);
    db.sql_do(sql_commit);

    printf("there are %s rows in the table\n", db.sql_value(sql_select_count));

    puts(sql_select_all);
    db.sql_prepare(sql_select_all);
    disp_rows(db);
    
    puts(sql_select_where);
    db.sql_prepare(sql_select_where, "four");
    disp_rows(db);

    puts(sql_drop);
    db.sql_do(sql_drop);
    
    return 0;
}
