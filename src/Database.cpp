#include "slight.h"
#include <sqlite3.h>
#include <string>

namespace slight {

Database::Database(const std::string& path, Access access)
    : m_open(false)
    , m_db(nullptr)
{
    if (sqlite3_open_v2(path.c_str(), &m_db, access, nullptr) == SQLITE_OK)
    {
        m_open = true;
    }
}

Database::~Database()
{
    sqlite3_close(m_db);
}

// @todo implemented backup
Result Database::backup(const std::string& backup_path, int page_size, int flags)
{
    sqlite3* backup_db;
    //int num_pages = 1;
    //auto status = sqlite3_open_v2(path, &db, backup_path, &backup_db);
    //auto backup = sqlite3_backup_init(db, "main", backup_path, "main");
    //const int res = sqlite3_backup_step(backup_db, 1);
    //sqlite3_backup_finish(m_db);
}

// @todo implement backup(size)
Result Database::backup(int page_size) {}

// @todo test
Result Database::getSchemaVersion()
{
    return run(Query("PRAGMA user_version"));
}

// @todo test
Result Database::setSchemaVersion(int version)
{
    return run(Query("PRAGMA user_version = ?", Bind(version)));
}

Result Database::run(Query&& query)
{
    if (!m_open)
    {
        return Result(m_db, stmt_ptr(nullptr), kError);
    }

    stmt_ptr stmt(m_db);
    query.compile(m_db, *stmt);
    return stmt ? Result(m_db, std::move(stmt)) : Result(m_db, std::move(stmt), kError);
}

Result Database::run(std::initializer_list<Query>&& queries)
{
    if (!m_open)
    {
        return Result(m_db, stmt_ptr(nullptr), kError);
    }

    transaction_t t(m_db);
    stmt_ptr stmt(m_db);
    for (auto query : queries)
    {
        if (!query.compile(m_db, *stmt))
        {
            return Result(m_db, std::move(stmt), kError);
        }

        auto result = Result(m_db, stmt);
        while (result.step()) {}
        if (result.error())
        {
            return Result(m_db, std::move(stmt), kError);
        }

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
