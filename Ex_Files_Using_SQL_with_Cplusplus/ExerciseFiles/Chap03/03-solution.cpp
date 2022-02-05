//  03-solution.cpp
//  Copyright 2021 BHG [bw.org]
//  as of 2021-05-31 bw

#include <cstdio>
#include "BWCRUD.h"
#include "03-solution.h"

constexpr const char * db_file = DB_PATH "/scratch.db";
constexpr const char * table_name = "domains";
constexpr const char * sql_create =
    "CREATE TABLE IF NOT EXISTS domains ("
    "id INTEGER PRIMARY KEY,"
    "domain VARCHAR(127) UNIQUE NOT NULL,"
    "description VARCHAR(255)"
    ")"
;

constexpr const char * menu[] = {
    "L) List domains",
    "A) Add domain",
    "E) Edit domain",
    "F) Find domain",
    "D) Delete domain",
    "X) Drop table and exit",
    "Q) Quit"
};

constexpr const char * prompt = "Select an action or Q to quit";

void display_row(bw::BWCRUD & db, const char ** row) {
    if(!row) {
        puts("display_row: No row.");
        return;
    }
    for(int i = 0; i < db.col_count(); ++i) {
        printf("%s ", row[i]);
    }
    newline();
}

void display_rows(bw::BWCRUD & db) {
    const char ** row = nullptr;

    while((row = db.fetch_row())) {
        display_row(db, row);
    }
}

void newline() {
    puts("");
}

const char * promptline(const char * prompt) {
    static char buf[MAX_SMALL_STRING_LENGTH];
    memset(buf, 0, MAX_SMALL_STRING_LENGTH);
    printf("%s > ", prompt);
    fgets(buf, MAX_SMALL_STRING_LENGTH, stdin);
    for(char c : "\x0a\x0d") {     // trim CR/LF
        char * loc = strrchr(buf, c);
        if(loc) *loc = 0;
    }
    return buf;
}

void do_menu(bw::BWCRUD & db) {
    char response = 'x';
    const char * buf = nullptr;
    while(true) {
        // print the menu
        newline();
        for( const char * s : menu) {
            puts(s);
        }

        // get the response
        buf = promptline(prompt);
        size_t bufsize = strnlen(buf, MAX_SMALL_STRING_LENGTH);

        // one character only, please
        if(bufsize != 1) {
            puts("Input too long or empty");
            continue;
        }
        response = buf[0];
        
        // process the response
        if(response >= 'a' && response <= 'z') {   // make it uppercase
            response -= 0x20;
        }
        
        // check if we know this one
        if(!strchr("LEAFDXQ", response)) {
            puts("Invalid response");
            continue;
        }

        // we can handle this one right here
        if(response == 'Q') {
            puts("Quit.");
            break;
        }
     
        // do it
        do_jump(db, response);
    }
}

void do_jump(bw::BWCRUD & db, const char & response) {
    switch (response) {
        case 'L':
            do_list(db);
            break;
        case 'A':
            do_add(db);
            break;
        case 'E':
            do_edit(db);
            break;
        case 'F':
            do_find(db);
            break;
        case 'D':
            do_delete(db);
            break;
        case 'X':
            do_drop_exit(db);
            break;
        default:
            break;
    }
    
}

void do_list(bw::BWCRUD & db) {
    newline();
    puts("List domains:");
    db.get_rows();
    display_rows(db);
}

void do_add(bw::BWCRUD & db) {
    char domain[MAX_SMALL_STRING_LENGTH];
    char description[MAX_SMALL_STRING_LENGTH];
    const char * buf = nullptr;
    newline();
    puts("Add domain");
    buf = promptline("Domain name");
    if(buf) {
        memcpy(domain, buf, MAX_SMALL_STRING_LENGTH);
    }
    buf = promptline("Description");
    if(buf) {
        memcpy(description, buf, MAX_SMALL_STRING_LENGTH);
    }
    printf("Adding domain %s with description %s\n", domain, description);
    int rc = db.insert(0, domain, description);
    if(!rc) {
        printf("Could not add record: %s\n", db.error_message());
    }
}

void do_edit(bw::BWCRUD & db) {
    newline();
    char domain[MAX_SMALL_STRING_LENGTH];
    memset(domain, 0, MAX_SMALL_STRING_LENGTH);

    puts("Edit domain");
    const char * buf = promptline("Domain name");
    memcpy(domain, buf, strnlen(buf, MAX_SMALL_STRING_LENGTH));

    int row_id = db.find_row_id("domain", buf);
    display_row(db, db.get_row(row_id));

    buf = promptline("Update description (blank to cancel)");
    if(buf[0]) {
        db.update_row(row_id, domain, buf);
    } else {
        puts("Cancel");
        return;
    }
}

void do_find(bw::BWCRUD & db) {
    newline();
    const char * buf = promptline("Find domain");
    const char ** row = db.find_row("domain", buf);
    if(row) {
        display_row(db, row);
    } else {
        puts("Not found.");
    }
}

void do_delete(bw::BWCRUD & db) {
    newline();
    int row_id = 0;
    puts("Delete domain");
    const char * buf = promptline("Domain");
    const char ** row = db.find_row("domain", buf);
    if(row) {
        display_row(db, row);
        row_id = atoi(row[0]);
    } else {
        puts("Not found.");
        return;
    }
    while(true) {
        buf = promptline("Delete (Y/N)?");
        if(buf[1]) {
            puts("Invalid response");
            continue;
        }
        if(buf[0] == 'n' || buf[0] == 'N') {
            puts("Cancel");
            return;
        }
        if(buf[0] == 'y' || buf[0] == 'Y') {
            break;
        }
        puts("Invalid response");
    }
    db.delete_row(row_id);
    printf("Row %d deleted\n", row_id);
}

void do_drop_exit(bw::BWCRUD & db) {
    newline();
    puts("Drop tabel and exit");
    db.drop_table();
    exit(0);
}

int main() {
    bw::BWCRUD db(db_file);
    printf("BWCRUD version: %s, SQLite version: %s\n", db.version(), db.sqlite_version());
    if(!db.have_table(table_name)) {
        puts("create table");
        db.sql_do(sql_create);
    }
    db.table_name(table_name);

    do_menu(db);

    return 0;
}
