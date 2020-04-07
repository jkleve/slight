#include "slight.h"
#include <sqlite3.h>
#include <cstdio>
#include <string>

namespace slight {

namespace details {
bool is_error(int error);
bool is_done(int error);

struct transaction_t {
    explicit transaction_t(sqlite3* db);
    ~transaction_t();
    void commit();
private:
    sqlite3* db;
    const std::string path;
    bool committed;
};

transaction_t::transaction_t(sqlite3* db) : db(db), committed(false)
{
    sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);
}

transaction_t::~transaction_t()
{
    if (!committed)
    {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
    }
}

void transaction_t::commit()
{
    if (!committed)
    {
        committed = true;
        sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    }
}
} // namespace details

bool slight_open(sqlite3** db, const char* path, Access access)
{
    return sqlite3_open_v2(path, db, access, nullptr) == SQLITE_OK;
}

/// Order of functions:
///  1. compile
///  2. bind
///  3. run
///
/// slight_exec does all 3 steps

bool slight_bind(sqlite3_stmt* stmt, const Q& query)
{
    size_t i = 1;
    for (auto& bind : query.binds())
    {
        auto column = i;
        if (bind.column_name())
        {
            column = sqlite3_bind_parameter_index(stmt, bind.column_name());
        }
        else if (bind.column() != Bind::kColumnIndexNotSet)
        {
            column = bind.column();
        }
        else
        {
            ++i;
        }
        if (bind.bind(stmt, column) != SQLITE_OK)
        {
            return false;
        }
    }
    return true;
}

int slight_run(sqlite3_stmt* stmt)
{
    int error = SLIGHT_OK;
    while (!details::is_done(error) && !details::is_error(error))
    {
        error = sqlite3_step(stmt);
    }
    return error;
}

bool slight_compile(sqlite3* db, sqlite3_stmt** stmt, const Q& query)
{
    // true if (prepare != OK || !bind)
    return !(sqlite3_prepare_v3(db, query.str(), query.str_size(), 0, stmt, nullptr) != SQLITE_OK ||
             !slight_bind(*stmt, query));
}

Result slight_execute(sqlite3* db, const Q& query)
{
    details::stmt_ptr stmt(db);
    if (slight_compile(db, *stmt, query))
    {
        slight_run(stmt.get());
    }
    return {db, std::move(stmt), sqlite3_errcode(db)};
}

Result slight_execute(sqlite3* db, const std::vector<Q>& queries)
{
    details::stmt_ptr stmt(db);
    for (const auto& query : queries)
    {
        // compile, run, reset
        if (!slight_compile(db, *stmt, query) || details::is_error(slight_run(stmt.get())) ||
            sqlite3_reset(stmt.get()) != SQLITE_OK)
        {
            break;
        }
    }
    return {db, std::move(stmt), sqlite3_errcode(db)};
}

Result slight_start(sqlite3* db, const Q& query)
{
    details::stmt_ptr stmt(db);
    if (!slight_compile(db, *stmt, query))
    {
        return {db, std::move(stmt), sqlite3_errcode(db)};
    }
    return std::move(Result(db, std::move(stmt), sqlite3_errcode(db)).step());
}

Database::Database(std::string path, Access access)
    : m_path(std::move(path))
    , m_open(false)
    , m_db(nullptr)
    , m_connection_pool(nullptr)
{
    if (slight_open(&m_db, m_path.c_str(), access))
    {
        m_open = true;
    }
}

Database::Database(sqlite3* db)
    : m_path(sqlite3_db_filename(db, ""))
    , m_open(true)
    , m_db(db)
    , m_connection_pool(nullptr)
{}

Database::~Database()
{
    sqlite3_close(m_db);
}

Result Database::check_exists(const std::string& table_name)
{
    return start("SELECT name FROM sqlite_master WHERE type='table' AND name='test'");
}

// @todo implemented backup
Result Database::backup(const std::string& backup_path, int page_size, int flags)
{
    //sqlite3* backup_db;
    //return Result(m_db, stmt_ptr(nullptr));
    //int num_pages = 1;
    //auto status = sqlite3_open_v2(path, &db, backup_path, &backup_db);
    //auto backup = sqlite3_backup_init(db, "main", backup_path, "main");
    //const int res = sqlite3_backup_step(backup_db, 1);
    //sqlite3_backup_finish(m_db);
}

// @todo implement backup(size)
Result Database::backup(int page_size) {}

// @todo test
Result Database::get_schema_version()
{
    return start(Q("PRAGMA user_version"));
}

Result Database::set_schema_version(uint32_t version)
{
    static const size_t max_size = 31; // 20 for PRAGMA, 10 for int, 1 for null char
    char query[max_size];
    snprintf(query, max_size, "PRAGMA user_version=%d", version);
    auto result = start(Q(query));
    if (result.ready())
    {
        result.step();
    }
    return std::move(result);
}

Result Database::start(const char* query, Bind bind) { return start(Q(query, bind)); }
Result Database::start(const char* query) { return start(Q(query)); }

Result Database::start(Q&& query)
{
    if (!m_open)
    {
        return Result(m_db, details::stmt_ptr(nullptr), SLIGHT_ERROR);
    }

    return slight_start(m_db, query);
}

Result Database::run(const char* query, Bind bind) { return run(Q(query, bind)); }
Result Database::run(const char* query) { return run(Q(query)); }

Result Database::run(Q&& query)
{
    if (!m_open)
    {
        return {m_db, details::stmt_ptr(nullptr), SLIGHT_ERROR};
    }

    return slight_execute(m_db, query);
}

Result Database::run(std::initializer_list<Q>&& queries)
{
    if (!m_open)
    {
        return Result(m_db, details::stmt_ptr(nullptr), SLIGHT_ERROR);
    }

    details::transaction_t t(m_db);
    auto result = slight_execute(m_db, queries);
    if (!result.error())
    {
        t.commit();
    }
    return result;
}

sqlite3* Database::getConnection()
{
    return m_db ? m_db : m_connection_pool->pop();
}

ConnectionPool::ConnectionPool(std::string path, Access access, size_t max_connections)
    : m_path(std::move(path))
    , m_access(access)
    , m_max_connections(max_connections)
    , m_num_connections(0)
    , m_mutex()
    , m_connection_available()
    , m_pool()
{
    assert(max_connections > 0);
    m_pool.reserve(m_max_connections);
}

ConnectionPool::~ConnectionPool()
{
    for (auto conn : m_pool)
    {
        sqlite3_close(conn);
    }
}

sqlite3* ConnectionPool::pop()
{
    std::unique_lock<std::mutex> lock_pool(m_mutex);

    if (m_pool.empty())
    {
        if (m_num_connections >= m_max_connections)
        {
            m_connection_available.wait(lock_pool, [this] { return !m_pool.empty(); });
        }

        auto conn = m_pool.back();
        m_pool.pop_back();
        return conn;
    }
    else
    {
        sqlite3* conn;
        if (slight_open(&conn, m_path.c_str(), m_access))
        {
            m_num_connections++;
            return conn;
        }
    }
    return nullptr;
}

void ConnectionPool::push(sqlite3* connection)
{
    std::unique_lock<std::mutex> lock_pool(m_mutex);
    m_pool.push_back(connection);
    m_connection_available.notify_one();
}

}
