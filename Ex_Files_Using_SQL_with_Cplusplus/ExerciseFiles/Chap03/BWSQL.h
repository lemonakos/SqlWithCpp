//  BWSQL.h
//  Copyright 2021 BHG [bw.org]
//  as of 2021-06-01 bw

#ifndef BWSQL_H
#define BWSQL_H

#include <sqlite3.h>
#include <sqlcpp.h>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <memory>

namespace bw {

#define _BWSQL_VERSION "1.0.7"

class BWSQL {
    const char * _filename = nullptr;
    sqlite3 * _db = nullptr;
    sqlite3_stmt * _stmt = nullptr;
    int _num_sql_columns = 0;
    const char ** _sql_colnames = nullptr;
    const char ** _row =  nullptr;

public:
    // ctor/dtor
    BWSQL(const char * filename);
    ~BWSQL();

    // sql methods
    int sql_prepare(const char * sql, ...);
    int sql_do(const char * sql, ...);
    const char * sql_value(const char * sql, ...);
    const char ** fetch_row();
    const char ** sql_column_names();
    int num_sql_columns() const;

    // utilities
    const char * version() const;
    const char * sqlite_version();
    const char * filename();
    int num_params(const char * sql);
    void reset_stmt();
    void reset();
    void error(const char * str = nullptr);
    void error_msg(const char * str = nullptr);
    sqlite3 * db() const;
    sqlite3_stmt * stmt() const;

    // rule of five stuff
    BWSQL()                     = delete;   // no default constructor
    BWSQL(const BWSQL &)        = delete;   // no copy
    BWSQL & operator = (BWSQL)  = delete;   // no assignment
    BWSQL(BWSQL &&)             = delete;   // no move

private:
    void _init();

protected:
    int _sql_prepare(const char * sql, va_list ap);
};

}

#endif // BWSQL_H
