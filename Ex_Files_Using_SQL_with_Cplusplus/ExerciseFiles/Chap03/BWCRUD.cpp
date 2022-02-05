//  CRUD.cpp
//  Copyright 2021 BHG [bw.org]
//  as of 2021-06-01 bw

#include "BWCRUD.h"

namespace bw {

// MARK: - constructors

BWCRUD::BWCRUD(const char * filename, const char * tablename)
: BWSQL(filename)
{
    _db = db();
    init_sec_db();

    // WAL mode allows sec_db read access to open tables
    // sqlite.org/wal.html
    sql_do("PRAGMA journal_mode=WAL");

    if(tablename) {
        _reset_table_name();
        _table_name = tablename;
    }

    return;
}

void BWCRUD::init_sec_db() {
    static BWSQL sec_db(filename());
    _sec_db = & sec_db;
}

// MARK: - CRUD methods

// va_list requires one named argument
// so it's called none – it's ignored just give it a zero
int BWCRUD::insert(int none, ...) {
    const char * buf = nullptr;
    if(!_table_name or !col_names()) {
        puts("insert: no table or column names");
        return 0;
    }
    char * sql = new char [MAX_SMALL_STRING_LENGTH];
    memset((void *) sql, 0, MAX_SMALL_STRING_LENGTH);

    const char * col_names = columns_string();
    const char * place_holders = columns_placeholder_string();

    // skip the id column (it's always the first col)
    buf = cstring_join(_col_count - 1, ",", _col_names + 1);
    memcpy((void *) col_names, buf, strnlen(buf, MAX_SMALL_STRING_LENGTH));
    buf = cstring_multiply(_col_count - 1, ",", "?");
    memcpy((void *) place_holders, buf, strnlen(buf, MAX_SMALL_STRING_LENGTH));

    sqlite3_snprintf(MAX_SMALL_STRING_LENGTH, sql,
                     "INSERT INTO %s (%s) VALUES (%s)",
                     _table_name, col_names, place_holders);
    
    va_list ap;
    va_start(ap, none);
    _sql_prepare(sql, ap);
    va_end(ap);
    sqlite3_step(stmt());

    delete [] sql;
    return sqlite3_changes(db());
}

// returns column count of prepared statement
int BWCRUD::get_rows() {
    const char * sql = _build_query("SELECT * FROM %s");
    return sql_prepare(sql);
}

const char ** BWCRUD::get_row(int id) {
    char idstr[MAX_TINY_STRING_LENGTH];
    find_rows("id", bw_itoa(id, idstr));
    return fetch_row();
}

// returns column count of prepared statement
int BWCRUD::find_rows(const char * col, const char * value) {
    char sql[MAX_SMALL_STRING_LENGTH];
    memset((void *) sql, 0, MAX_SMALL_STRING_LENGTH);
    sqlite3_snprintf(MAX_SMALL_STRING_LENGTH, sql,
                     "SELECT * FROM %s WHERE %s LIKE ?",
                     _table_name, col);
    return sql_prepare(sql, value);
}

// returns first row in result
const char ** BWCRUD::find_row(const char * col, const char * value) {
    find_rows(col, value);
    return fetch_row();
}

// returns id of first row in result
int BWCRUD::find_row_id(const char * col, const char * value) {
    find_rows(col, value);
    return atoi(fetch_row()[0]);
}

int BWCRUD::update_row(int row_id, ...) {
    size_t buflen = 0;
    if(!_table_name or !col_names()) {
        puts("insert: no table or column names");
        return 0;
    }
    char * sql = new char [MAX_SMALL_STRING_LENGTH];
    memset((void *) sql, 0, MAX_SMALL_STRING_LENGTH);

    // build the convoluted update string
    // Part 1 – "UPDATE {table} SET "
    sqlite3_snprintf(MAX_SMALL_STRING_LENGTH, sql, "UPDATE %s SET ", _table_name);
    buflen = strnlen(sql, MAX_SMALL_STRING_LENGTH);

    // {column} = ?, {column} = ?, [...]
    for(int i = 1; i < _col_count; ++i) {
        if(buflen >= MAX_SMALL_STRING_LENGTH - 64){
            return 0;   // query too long for buffer
        }
        sqlite3_snprintf(MAX_SMALL_STRING_LENGTH - (int) buflen, sql + buflen, "%s = ?%s",
                         _col_names[i],
                         (i < _col_count - 1) ? ", " : "");
        buflen = strnlen(sql, MAX_SMALL_STRING_LENGTH);
    }

    // keep checking buflen - important!
    if(buflen >= MAX_SMALL_STRING_LENGTH - 64){
        return 0;   // query too long for buffer
    }

    // last part!
    sqlite3_snprintf(MAX_SMALL_STRING_LENGTH - (int) buflen, sql + buflen, " WHERE id = %d", row_id);
    buflen = strnlen(sql, MAX_SMALL_STRING_LENGTH);

    va_list ap;
    va_start(ap, row_id);
    _sql_prepare(sql, ap);
    va_end(ap);
    sqlite3_step(stmt());
    return sqlite3_changes(db());
}

int BWCRUD::delete_row(int id) {
    char idstr[MAX_TINY_STRING_LENGTH];
    find_rows("id", bw_itoa(id, idstr));
    sql_do(_build_query("DELETE FROM %s WHERE id = ?"), bw_itoa(id, idstr));
    return sqlite3_changes(db());
}

void BWCRUD::begin() {
    sql_do("BEGIN");
}

void BWCRUD::commit() {
    sql_do("COMMIT");
    reset_stmt();
}

int BWCRUD::count_rows() {
    // use the sec database so we don't interfere with an ongoing statement
    const char * cc = _sec_db->sql_value(_build_query("SELECT COUNT(*) FROM %s"));
    if(cc) {
        return atoi(cc);
    } else {
        return 0;
    }
}

int BWCRUD::col_count() {
    // use the sec database so we don't interfere with an ongoing statement
    if(_table_name && !_col_count) {
        const char * cc = _sec_db->sql_value(_build_query("SELECT COUNT(*) FROM pragma_table_info('%s');"));
        if(cc) {
            _col_count = atoi(cc);
        }
    }
    return _col_count;
}

const char ** BWCRUD::col_names(){
    if(!_table_name) {
        return nullptr;
    }
    if(!_col_count) {
        col_count();
    }
    if(!_col_names) {
        // we need space for both pointers and storage
        // this is freed from the destructor
        _col_names = new const char * [_col_count];
        _col_names_buf = new char [_col_count * MAX_SMALL_STRING_LENGTH];
        memset((void *) _col_names, 0, _col_count * sizeof(const char *));
        memset((void *) _col_names_buf, 0, _col_count * MAX_SMALL_STRING_LENGTH);

        // use the sec database so we don't interfere with an ongoing statement
        _sec_db->sql_prepare(_build_query("SELECT name FROM pragma_table_info('%s');"));

        for(int i = 0; i < _col_count; ++i) {
            const char ** row = _sec_db->fetch_row();
            if(row && row[0]) {
                if(i == 0 && !(row[0][0] == 'i' && row[0][1] == 'd' && row[0][2] == 0)) {
                    this->_reset_table_name();
                    puts("col_names: first column must be id");
                    exit(0);
                }
                const char * v = _col_names_buf + (i * MAX_SMALL_STRING_LENGTH);
                memcpy((void *)v, (const void *) row[0], strnlen(row[0], MAX_SMALL_STRING_LENGTH));
                _col_names[i] = v;
            }
        }
        
    }
    return _col_names;
}

bool BWCRUD::have_table(const char * name) {
    if(!name) {
        name = _table_name;
    }

    // use the sec database so we don't interfere with an ongoing statement
    const char * rc = _sec_db->sql_value("SELECT name FROM sqlite_master WHERE type = 'table' AND name = ?", name);
    return rc ? true : false;
}

int BWCRUD::drop_table() {
    return sql_do(_build_query("DROP TABLE IF EXISTS %s"));
}

// MARK: - utilities

// join cstrings with optional separator
// uses sqlite3's dynamic string object
const char * BWCRUD::cstring_join(int count, const char * sep, ...) {
    static char cstr[MAX_SMALL_STRING_LENGTH];
    memset((void *) cstr, 0, MAX_SMALL_STRING_LENGTH);
    size_t sep_len = 0;
    size_t len = 0;
    va_list ap;
    va_start(ap, sep);
    if(sep) {
        sep_len = strnlen(sep, MAX_SMALL_STRING_LENGTH);
    }
    sqlite3_str * s_str = sqlite3_str_new(_db);
    for(; count > 0; --count) {
        const char * next_str = va_arg(ap, const char *);
        if(next_str == nullptr) {
            break;
        }
        size_t next_len = strnlen(next_str, MAX_SMALL_STRING_LENGTH);
        if(len + next_len < MAX_SMALL_STRING_LENGTH) {
            sqlite3_str_appendall(s_str, next_str);
            len += next_len;
        } else {
            break;
        }
        if(count > 1 && len < MAX_SMALL_STRING_LENGTH + sep_len) {
            sqlite3_str_appendall(s_str, sep);
            len += sep_len;
        }
    }
    memcpy((void *) cstr, sqlite3_str_value(s_str), len);
    sqlite3_str_finish(s_str);
    va_end(ap);
    return cstr;
}

const char * BWCRUD::cstring_join(int count, const char * sep, const char ** strs) {
    static char cstr[MAX_SMALL_STRING_LENGTH];
    memset((void *) cstr, 0, MAX_SMALL_STRING_LENGTH);
    size_t sep_len = 0;
    size_t len = 0;
    if(sep) {
        sep_len = strnlen(sep, MAX_SMALL_STRING_LENGTH);
    }
    sqlite3_str * s_str = sqlite3_str_new(_db);
    for(int index = 0; index < count; ++index) {
        const char * next_str = strs[index];
        if(next_str == nullptr) {
            break;
        }
        size_t next_len = strnlen(next_str, MAX_SMALL_STRING_LENGTH);
        if(len + next_len < MAX_SMALL_STRING_LENGTH) {
            sqlite3_str_appendall(s_str, next_str);
            len += next_len;
        } else {
            break;
        }
        if(index < (count - 1) && sep_len && len < MAX_SMALL_STRING_LENGTH + sep_len) {
            sqlite3_str_appendall(s_str, sep);
            len += sep_len;
        }
    }
    memcpy((void *) cstr, sqlite3_str_value(s_str), len);
    sqlite3_str_finish(s_str);
    return cstr;
}

const char * BWCRUD::cstring_multiply(int count, const char * sep, const char * str) {
    static char cstr[MAX_SMALL_STRING_LENGTH];
    memset((void *) cstr, 0, MAX_SMALL_STRING_LENGTH);
    size_t str_len = strnlen(str, MAX_SMALL_STRING_LENGTH);
    size_t sep_len = 0;
    size_t len = 0;
    if(sep) {
        sep_len = strnlen(sep, MAX_SMALL_STRING_LENGTH);
    }
    if(str_len <= 0) {
        return cstr;
    }
    sqlite3_str * s_str = sqlite3_str_new(_db);
    for(int index = 0; index < count; ++index) {
        if(len + str_len < MAX_SMALL_STRING_LENGTH) {
            sqlite3_str_appendall(s_str, str);
            len += str_len;
        }
        if(index < (count - 1) && sep_len && len < MAX_SMALL_STRING_LENGTH + sep_len) {
            sqlite3_str_appendall(s_str, sep);
            len += sep_len;
        }
    }
    memcpy((void *) cstr, sqlite3_str_value(s_str), len);
    sqlite3_str_finish(s_str);
    return cstr;
}

const char * BWCRUD::columns_string() {
    static char col_names[MAX_SMALL_STRING_LENGTH];
    memset((void *) col_names, 0, MAX_SMALL_STRING_LENGTH);

    // skip the id column (always first)
    const char * buf = cstring_join(_col_count - 1, ",", _col_names + 1);
    memcpy((void *) col_names, buf, strnlen(buf, MAX_SMALL_STRING_LENGTH));
    return col_names;
}

const char * BWCRUD::columns_placeholder_string() {
    static char col_placeholders[MAX_SMALL_STRING_LENGTH];
    memset((void *) col_placeholders, 0, MAX_SMALL_STRING_LENGTH);

    const char * buf = cstring_multiply(_col_count - 1, ",", "?");
    memcpy((void *) col_placeholders, buf, strnlen(buf, MAX_SMALL_STRING_LENGTH));
    return col_placeholders;
}

const char * BWCRUD::table_name() {
    return _table_name;
}

void BWCRUD::table_name(const char * tablename) {
    _reset_table_name();
    _table_name = tablename;

    // init column count for table
    // do this now because it tends to
    // interrupt other things
    col_count();
}

const char * BWCRUD::version() const {
    return _BWCRUD_VERSION;
}

const char * BWCRUD::error_message() {
    return sqlite3_errmsg(_db);
}

const char * BWCRUD::bw_itoa(int value, char * strbuf) {
    sqlite3_snprintf(MAX_TINY_STRING_LENGTH, strbuf, "%d", value);
    return strbuf;
}

// MARK: - private

const char * BWCRUD::_build_query(const char * qstr) {
    if(!_table_name) {
        return qstr;
    }
    static char query[MAX_SMALL_STRING_LENGTH];
    memset(query, 0, MAX_SMALL_STRING_LENGTH);
    sqlite3_snprintf(MAX_SMALL_STRING_LENGTH, query, qstr, _table_name);
    return query;
}

void BWCRUD::_reset_table_name() {
    _table_name = nullptr;
    if(_col_names) {
        delete [] _col_names;
        _col_names = nullptr;
    }
    if(_col_names_buf) {
        delete [] _col_names_buf;
        _col_names_buf = nullptr;
    }
    _col_count = 0;
}

}
