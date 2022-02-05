//  BWSQL.cpp
//  Copyright 2021 BHG [bw.org]
//  as of 2021-06-01 bw

#include "BWSQL.h"

namespace bw {

// MARK: - constructors

void BWSQL::_init() {
    reset();
    int rc = sqlite3_open(_filename, &_db);
    if(rc) {
        error("sqlite_open");
    }
}

BWSQL::~BWSQL() {
    reset();
}

BWSQL::BWSQL(const char * filename)
:_filename(filename)
{
    _init();
}

// MARK: - sql methods
// all va_args are const char *

int BWSQL::_sql_prepare(const char * sql, va_list ap) {
    reset_stmt();
    int rc = sqlite3_prepare_v2(_db, sql, -1, &_stmt, nullptr);
    if(rc) {
        error_msg("sql_prepare");
        reset_stmt();
        return 0;
    }
    _num_sql_columns = sqlite3_column_count(_stmt);
    int col_count = sqlite3_bind_parameter_count(_stmt);
    if(col_count) {
        for(int param_no = 1; param_no <= col_count; ++param_no) {     // params start at 1
            const char * param = va_arg(ap, const char *);
            sqlite3_bind_text(_stmt, param_no, param, -1, SQLITE_STATIC);
        }
    }
    return col_count;
}

int BWSQL::sql_prepare(const char * sql, ...) {
    va_list ap;
    va_start(ap, sql);
    _sql_prepare(sql, ap);
    va_end(ap);
    return num_sql_columns();
}

int BWSQL::sql_do(const char * sql, ...) {
    va_list ap;
    va_start(ap, sql);
    _sql_prepare(sql, ap);
    va_end(ap);
    sqlite3_step(_stmt);
    reset_stmt();
    return sqlite3_changes(_db);
}

const char * BWSQL::sql_value(const char * sql, ...) {
    va_list ap;
    va_start(ap, sql);
    _sql_prepare(sql, ap);
    va_end(ap);
    const char ** row = fetch_row();
    if(row) {
        return row[0];
    } else {
        return nullptr;
    }
}

const char ** BWSQL::fetch_row() {
    // do we have a statement?
    if(!_stmt) {
        reset_stmt();
        return nullptr;
    }
    // get the next row, if avail
    if(sqlite3_step(_stmt) != SQLITE_ROW) {
        reset_stmt();
        return nullptr;
    }
    // make sure we have allocated space
    if(_num_sql_columns && !_row) {
        _row = new const char * [_num_sql_columns];
    }
    for(int index = 0; index < _num_sql_columns; ++index) {
        _row[index] = (const char *) sqlite3_column_text(_stmt, index);
    }
    return _row;
}

const char ** BWSQL::sql_column_names() {
    if(!_stmt) {
        reset_stmt();
        return nullptr;
    }
    if(_num_sql_columns && !_sql_colnames) {
        _sql_colnames = new const char * [_num_sql_columns];
    }
    for(int index = 0; index < _num_sql_columns; ++index) {
        _sql_colnames[index] = (const char *) sqlite3_column_name(_stmt, index);
    }
    return _sql_colnames;
}


int BWSQL::num_sql_columns() const {
    return _num_sql_columns;
}

// MARK: - utilities

const char * BWSQL::version() const {
    return _BWSQL_VERSION;
}

const char * BWSQL::sqlite_version() {
    return sql_value("SELECT sqlite_version()");
}

const char * BWSQL::filename() {
    return _filename;
}

// count bind parameters in an SQL string
int BWSQL::num_params(const char * sql) {
    constexpr char bind_char = '?';
    int count = 0;
    size_t len = strnlen(sql, MAX_STRING_LENGTH);
    for(int i = 0; i < len; ++i) {
        if(sql[i] == bind_char) ++count;
    }
    return count;
}

void BWSQL::reset_stmt() {
    _num_sql_columns = 0;
    if(_stmt) {
        sqlite3_finalize(_stmt);
        _stmt = nullptr;
    }
    if(_row) {
        delete [] _row;
        _row = nullptr;
    }
    if(_sql_colnames) {
        delete [] _sql_colnames;
        _sql_colnames = nullptr;
    }
}

void BWSQL::reset() {
    reset_stmt();
    if(_db) {
        sqlite3_close(_db);
        _db = nullptr;
    }
}

void BWSQL::error(const char * str) {
    if(str) {
        printf("%s: ", str);
    }
    if(_db) {
        printf("%s\n", sqlite3_errmsg(_db));
    } else {
        printf("unknown error\n");
    }
    reset();
    exit(0);
}

void BWSQL::error_msg(const char * str) {
    if(str) {
        printf("%s: ", str);
    }
    if(_db) {
        printf("%s\n", sqlite3_errmsg(_db));
    } else {
        printf("unknown error\n");
    }
}

sqlite3 * BWSQL::db() const {
    return _db;
}

sqlite3_stmt * BWSQL::stmt()const {
    return _stmt;
}

}
