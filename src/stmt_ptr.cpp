#include "slight.h"
#include <sqlite3.h>

namespace slight {

stmt_ptr::stmt_ptr(sqlite3* db) : finalize(true), db(db), stmt(nullptr) {}

stmt_ptr::stmt_ptr(stmt_ptr&& other) noexcept : finalize(other.finalize), db(other.db), stmt(other.stmt)
{
    other.finalize = false;
}

stmt_ptr::stmt_ptr(const stmt_ptr& other) : finalize(false), db(other.db), stmt(other.stmt) {}

stmt_ptr& stmt_ptr::operator=(const stmt_ptr& other)
{
    finalize = false;
    db = other.db;
    stmt = other.stmt;
    return *this;
}

stmt_ptr::~stmt_ptr()
{
    if (finalize)
    {
        sqlite3_finalize(stmt);
        stmt = nullptr;
    }
}

sqlite3_stmt** stmt_ptr::operator*() { return &stmt; }
stmt_ptr::operator bool() const      { return  stmt; }
sqlite3_stmt* stmt_ptr::get() const  { return  stmt; }

} // namespace slight
