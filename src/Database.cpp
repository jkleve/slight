#include "slight.h"
#include <sqlite3.h>
#include <string>

namespace slight {

Database::Database(const std::string& path, Access access)
    : m_db(nullptr)
{
    auto status = sqlite3_open_v2(path.c_str(), &m_db, access, nullptr);
    if (status != SQLITE_OK)
    {
        sqlite3_close(m_db);
    }
}

Database::~Database()
{
    sqlite3_close(m_db);
}

// @todo unimplemented methods
Result Database::backup() {}
Result Database::backup(int page_size) {}
Result Database::getSchemaVersion() {}
Result Database::setSchemaVersion(int version) {}

Result Database::build(Sql&& sql)
{
    if (!m_db)
        return Result(m_db, stmt_ptr(nullptr), Result::kError);

    stmt_ptr stmt(m_db);
    sql.compile(m_db, *stmt);
    return stmt ? Result(m_db, std::move(stmt)) : Result(m_db, std::move(stmt), Result::kError);
}

Result Database::run(std::initializer_list<Sql>&& sqls)
{
    if (!m_db)
        return Result(m_db, stmt_ptr(nullptr), Result::kError);

    bool success = true;
    transaction_t t(m_db);
    stmt_ptr stmt(m_db);
    for (auto sql : sqls)
    {
        if (!sql.compile(m_db, *stmt))
            return Result(m_db, std::move(stmt), Result::kError);

        auto result = Result(m_db, stmt);
        while (result.step()) {}
        if (result.error())
            return Result(m_db, std::move(stmt), Result::kError);

        sqlite3_reset(stmt.get());
    }

    t.commit();
    return Result(m_db, stmt);
}

Database::transaction_t::transaction_t(sqlite3* db) : db(db), committed(false)
{
    sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);
}

Database::transaction_t::~transaction_t()
{
    if (!committed)
    {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
    }
}

void Database::transaction_t::commit()
{
    if (!committed)
    {
        committed = true;
        sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    }
}

}
