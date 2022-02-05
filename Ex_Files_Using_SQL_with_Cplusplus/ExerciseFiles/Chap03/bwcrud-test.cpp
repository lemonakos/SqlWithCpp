//  bwcrud-test.cpp
//  Copyright 2021 BHG [bw.org]
//  as of 2021-05-30 bw

#include <cstdio>
#include "BWCRUD.h"

constexpr const char * db_file =    DB_PATH "/scratch.db";

constexpr const char * table_name = "temp";
constexpr const char * sql_create = "CREATE TABLE IF NOT EXISTS temp"
                                    "( id INTEGER PRIMARY KEY, a TEXT, b TEXT, c TEXT )";
constexpr const char * sql_drop =   "DROP TABLE IF EXISTS temp";

constexpr const char * insert_strings[] = {
    "one",   "two",   "three",
    "two",   "three", "four",
    "three", "four",  "five",
    "four",  "five",  "six",
    "five",  "six",   "seven"
};

void display_row(bw::BWCRUD & db, const char ** row) {
    for(int i = 0; i < db.col_count(); ++i) {
        printf("%s ", row[i]);
    }
    puts("");
}

void display_rows(bw::BWCRUD & db) {
    const char ** row = nullptr;
    while((row = db.fetch_row())) {
        display_row(db, row);
    }
}

int main() {
    bw::BWCRUD db(db_file, table_name);
    const char ** row = nullptr;

    printf("BWCRUD version: %s, SQLite version: %s\n", db.version(), db.sqlite_version());

    puts(sql_drop);
    db.sql_do(sql_drop);
    puts(sql_create);
    db.sql_do(sql_create);

    puts("begin");
    db.begin();

    // TODO
    puts("insert rows");
    int num_cols = db.col_count() - 1;  // don't count id
    int num_rows = sizeof(insert_strings) / sizeof(const char *) / num_cols;
    int index = 0;
    int count = 0;
    for(int row_no = 0; row_no < num_rows; ++row_no) {
        const char * col1 = insert_strings[index++];
        const char * col2 = insert_strings[index++];
        const char * col3 = insert_strings[index++];
        count += db.insert(0, col1, col2, col3);
    }
    puts("commit");
    printf("%d rows inserted\n", count);
    db.commit();

    const char ** colnames = db.col_names();
    printf("col_names: %s\n", db.cstring_join(db.col_count(), ", ", colnames));
    printf("there are %d rows in %s\n", db.count_rows(), db.table_name());

    puts("get rows");
    db.get_rows();
    display_rows(db);

    const char * where_col = "b";
    const char * where_value = "%i%";
    printf("find_rows(%s, %s)\n", where_col, where_value);
    db.find_rows(where_col, where_value);
    display_rows(db);

    puts("find row id for b is four");
    int row_id = db.find_row_id("b", "four");
    printf("row id is %d\n", row_id);
    row = db.get_row(row_id);
    display_row(db, row);

    printf("delete row %d\n", row_id);
    db.delete_row(row_id);

    puts("get rows");
    db.get_rows();
    display_rows(db);

    puts("drop table");
    db.drop_table();

    return 0;
}
