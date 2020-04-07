#include "slight.h"
#include <sqlite3.h>

namespace slight {

namespace details {
bool is_error(int error) { return error != SQLITE_OK && error != SQLITE_ROW && error != SQLITE_DONE; }
bool is_done(int error) { return error == SQLITE_DONE; }
} // namespace details

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
Typer<kFloat>::Type Result::get<kFloat>(int index)
{
    return static_cast<Typer<kFloat>::Type>(sqlite3_column_double(m_stmt.get(), index - 1));
}

template<>
Typer<kText>::Type Result::get<kText>(int index)
{
    return reinterpret_cast<Typer<kText>::Type>(sqlite3_column_text(m_stmt.get(), index - 1));
}

Result::Result(sqlite3* db, details::stmt_ptr&& stmt, int error)
    : m_error(error), m_db(db), m_stmt(std::move(stmt))
{}

Result::Result(Result&& other) noexcept
    : m_error(other.m_error), m_db(other.m_db), m_stmt(std::move(other.m_stmt))
{}

bool Result::done() const
{
    return details::is_done(m_error);
}

bool Result::error() const
{
    return details::is_error(m_error);
}

int Result::error_code() const
{
    return sqlite3_errcode(m_db);
}

const char* Result::error_msg() const
{
    return sqlite3_errmsg(m_db);
}

bool Result::ready() const
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
