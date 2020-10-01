#include "slight.h"
#include "sqlite3.h"

namespace slight {

// @todo lol this ain't gonna work. this is exactly the same problem we ran into last time where we want Statement
// @todo to be private but give users access to state changing code to modify statement. we need an opaque reference.
/// @brief Statement
///
/// @note This class is used to access the results of a database operation.
///
struct Statement {
    Statement(sqlite3* db, std::string statement)
        : statement(std::move(statement))
        , sqlite_error(sqlite3_errcode(db))
        , db(db)
        , stmt()
    {}
    ~Statement() { sqlite3_finalize(stmt); }

    const std::string statement;

    int sqlite_error;
    sqlite3* db;
    sqlite3_stmt* stmt;
};

void reset(Statement& statement)
{
    sqlite3_reset(statement.stmt);
}

bool is_done(const Statement& statement)
{
    return statement.sqlite_error == SQLITE_DONE;
}

bool did_error(const Statement& statement)
{
    return
        statement.sqlite_error != SQLITE_OK &&
        statement.sqlite_error != SQLITE_ROW &&
        statement.sqlite_error != SQLITE_DONE;
}

int error_code(const Statement& statement)
{
    return sqlite3_errcode(statement.db);
}

const char* error_msg(const Statement& statement)
{
    return sqlite3_errmsg(statement.db);
}

bool is_ready(const Statement& statement)
{
    return !(did_error(statement) || is_done(statement));
}

void step(Statement& statement)
{
    if (!did_error(statement))
    {
        statement.sqlite_error = sqlite3_step(statement.stmt);
    }
}

void bind(Statement& statement, Bind bind)
{
    int index = 0;
    if (bind.type == Bind::Type::column)
    {
        index = sqlite3_bind_parameter_index(statement.stmt, bind.column);
    }

    switch (bind.data_type) {
        case Bind::DataType::i32:
            statement.sqlite_error = sqlite3_bind_int(statement.stmt, index, bind.i);
            break;
        case Bind::DataType::i64:
        case Bind::DataType::u32:
            statement.sqlite_error = sqlite3_bind_int64(statement.stmt, index, bind.i);
            statement.sqlite_error = sqlite3_bind_int64(statement.stmt, index, bind.i);
            break;
        case Bind::DataType::flt:
            statement.sqlite_error = sqlite3_bind_double(statement.stmt, index, bind.f);
            break;
        case Bind::DataType::str:
            statement.sqlite_error = sqlite3_bind_text(statement.stmt, index, bind.str, strlen(bind.str), nullptr);
            break;
    }
}

void bind(Statement& statement, const std::vector<Bind>& binds)
{
    for (auto b : binds)
    {
        if (!is_ready(statement))
        {
            break;
        }

        bind(statement, b);
    }
}

template<>
Typer<i32>::Type get<i32>(Statement& statement, int index)
{
    return sqlite3_column_int(statement.stmt, index - 1);
}

template<>
Typer<i64>::Type get<i64>(Statement& statement, int index)
{
    return sqlite3_column_int64(statement.stmt, index - 1);
}

template<>
Typer<u32>::Type get<u32>(Statement& statement, int index)
{
    return static_cast<Typer<u32>::Type>(sqlite3_column_int64(statement.stmt, index - 1));
}

template<>
Typer<flt>::Type get<flt>(Statement& statement, int index)
{
    return static_cast<Typer<flt>::Type>(sqlite3_column_double(statement.stmt, index - 1));
}

template<>
Typer<text>::Type get<text>(Statement& statement, int index)
{
    return reinterpret_cast<Typer<text>::Type>(sqlite3_column_text(statement.stmt, index - 1));
}

Database::Database(const std::string& path, bool opened, sqlite3* db)
    : path(path)
    , opened(opened)
    , error(opened ? "" : sqlite3_errmsg(db))
    , db(db)
{}

Database Database::open_db(const std::string& path, int access)
{
    sqlite3* db;
    auto status = sqlite3_open_v2(path.c_str(), &db, access, nullptr);
    return Database(path, status == SQLITE_OK, db);
}

Database Database::open_read_only(const std::string& path)
{
    return open_db(path, SQLITE_OPEN_READONLY);
}

Database Database::open_read_write(const std::string& path)
{
    return open_db(path, SQLITE_OPEN_READWRITE);
}

Database Database::open_create_read_write(const std::string& path)
{
    return open_db(path, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE);
}

std::unique_ptr<Database> Database::make_db(const std::string& path, int access)
{
    sqlite3* db;
    auto status = sqlite3_open_v2(path.c_str(), &db, access, nullptr);
    return std::unique_ptr<Database>(new Database(path, status == SQLITE_OK, db));
}

std::unique_ptr<Database> Database::make_read_only(const std::string& path)
{
    return make_db(path, SQLITE_OPEN_READONLY);
}

std::unique_ptr<Database> Database::make_read_write(const std::string& path)
{
    return make_db(path, SQLITE_OPEN_READWRITE);
}

std::unique_ptr<Database> Database::make_create_read_write(const std::string& path)
{
    return make_db(path, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE);
}

Database::~Database()
{
    sqlite3_close(db);
}

std::shared_ptr<Statement> Database::prepare(const std::string& statement)
{
    assert(!statement.empty());

    auto stmt = std::make_shared<Statement>(db, statement);
    sqlite3_prepare_v3(db, statement.c_str(), statement.size(), 0, &stmt->stmt, nullptr);

    assert(sqlite3_bind_parameter_count(stmt->stmt) == 0);
    return stmt;
}

std::shared_ptr<Statement> Database::get_schema_version()
{
    auto stmt = prepare("PRAGMA user_version");
    step(*stmt);
    return std::move(stmt);
}

std::shared_ptr<Statement> Database::set_schema_version(SchemaVersion version)
{
    static const size_t max_size = 31; // 20 for PRAGMA, 10 for int, 1 for null char
    char statement[max_size];
    snprintf(statement, max_size, "PRAGMA user_version=%d", version);
    auto stmt = prepare(statement);
    if (is_ready(*stmt))
    {
        step(*stmt);
    }
    return std::move(stmt);
}

} // namespace slight
