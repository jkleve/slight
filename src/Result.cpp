#include "slight.h"
#include <sqlite3.h>

namespace slight {

template<>
Typer<kInt>::Type Result::get<kInt>(int index)
{
    return sqlite3_column_int(m_stmt.get(), index - 1);
}

template<>
Typer<kInt64>::Type Result::get<kInt64>(int index)
{
    return sqlite3_column_int64(m_stmt.get(), index - 1);
}

template<>
Typer<kUint>::Type Result::get<kUint>(int index)
{
    return static_cast<Typer<kUint>::Type>(sqlite3_column_int64(m_stmt.get(), index - 1));
}

template<>
Typer<kText>::Type Result::get<kText>(int index)
{
    return reinterpret_cast<const char*>(sqlite3_column_text(m_stmt.get(), index - 1));
}

Result::Result(sqlite3* db, stmt_ptr stmt, int error) : m_error(error), m_db(db), m_stmt(std::move(stmt))
{}

bool Result::done() const
{
    return m_error == SQLITE_DONE;
}

bool Result::error() const
{
    return m_error != SQLITE_OK && m_error != SQLITE_ROW && m_error != SQLITE_DONE;
}

int Result::error_code() const
{
    return sqlite3_errcode(m_db);
}

const char* Result::error_msg() const
{
    return sqlite3_errmsg(m_db);
}

Result::operator bool() const
{
    return !(error() || done());
}

Result& Result::step()
{
    if (!error())
    {
        m_error = sqlite3_step(m_stmt.get());
    }
    return *this;
}

} // namespace slight
