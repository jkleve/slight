#include "slight.h"
#include "sqlite3.h"

#include <cassert> // assert
#include <cstring> // strlen

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
        , sqlite_errcode(sqlite3_errcode(db))
        , db(db)
        , stmt()
    {}
    ~Statement() { sqlite3_finalize(stmt); }

    const std::string statement;

    int sqlite_errcode;
    sqlite3* db;
    sqlite3_stmt* stmt;
};

Bind::Bind(int32_t i)
    : type(Type::empty)
    , data_type(DataType::i32)
    , i(static_cast<int64_t>(i)) {}
Bind::Bind(int64_t i)
    : type(Type::empty)
    , data_type(DataType::i64)
    , i(i) {}
Bind::Bind(uint32_t i)
    : type(Type::empty)
    , data_type(DataType::u32)
    , i(static_cast<int64_t>(i)) {}
Bind::Bind(float f)
    : type(Type::empty)
    , data_type(DataType::flt)
    , f(f) {}
Bind::Bind(const char* str)
    : type(Type::empty)
    , data_type(DataType::str)
    , str(str) {}
Bind::Bind(int index, int32_t i)
    : type(Type::index)
    , index(index)
    , data_type(DataType::i32)
    , i(i) {}
Bind::Bind(int index, int64_t i)
    : type(Type::index)
    , index(index)
    , data_type(DataType::i64)
    , i(i) {}
Bind::Bind(int index, uint32_t i)
    : type(Type::index)
    , index(index)
    , data_type(DataType::u32)
    , i(i) {}
Bind::Bind(int index, float f)
    : type(Type::index)
    , index(index)
    , data_type(DataType::flt)
    , f(f) {}
Bind::Bind(int index, const char* str)
    : type(Type::index)
    , index(index)
    , data_type(DataType::str)
    , str(str) {}
Bind::Bind(const char* column, int32_t i)
    : type(Type::column)
    , column(column)
    , data_type(DataType::i32)
    , i(static_cast<int64_t>(i)) {}
Bind::Bind(const char* column, int64_t i)
    : type(Type::column)
    , column(column)
    , data_type(DataType::i64)
    , i(i) {}
Bind::Bind(const char* column, uint32_t i)
    : type(Type::column)
    , column(column)
    , data_type(DataType::u32)
    , i(static_cast<int64_t>(i)) {}
Bind::Bind(const char* column, float f)
    : type(Type::column)
    , column(column)
    , data_type(DataType::flt)
    , f(f) {}
Bind::Bind(const char* column, const char* str)
    : type(Type::column)
    , column(column)
    , data_type(DataType::str)
    , str(str) {}

void reset(Statement& statement)
{
    sqlite3_reset(statement.stmt);
}

bool is_done(const Statement& statement)
{
    return statement.sqlite_errcode == SQLITE_DONE;
}

bool did_error(const Statement& statement)
{
    return
        statement.sqlite_errcode != SQLITE_OK &&
        statement.sqlite_errcode != SQLITE_ROW &&
        statement.sqlite_errcode != SQLITE_DONE;
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

bool has_row(const Statement& statement)
{
    return statement.sqlite_errcode == SQLITE_ROW;
}

void step(Statement& statement)
{
    if (!did_error(statement))
    {
        statement.sqlite_errcode = sqlite3_step(statement.stmt);
    }
}

void bind(Statement& statement, const Bind& bind, int index)
{
    assert(index > 0);
    switch (bind.data_type) {
        case Bind::DataType::i32:
            statement.sqlite_errcode = sqlite3_bind_int(statement.stmt, index, bind.i);
            break;
        case Bind::DataType::i64:
        case Bind::DataType::u32:
            statement.sqlite_errcode = sqlite3_bind_int64(statement.stmt, index, bind.i);
            break;
        case Bind::DataType::flt:
            statement.sqlite_errcode = sqlite3_bind_double(statement.stmt, index, bind.f);
            break;
        case Bind::DataType::str:
            statement.sqlite_errcode = sqlite3_bind_text(statement.stmt, index, bind.str, strlen(bind.str), nullptr);
            break;
    }
}

void bind(Statement& statement, const Bind& b)
{
    int index = 0;

    if (b.type == Bind::Type::column)
    {
        index = sqlite3_bind_parameter_index(statement.stmt, b.column);
    }
    else if (b.type == Bind::Type::index)
    {
        index = b.index;
    }
    else
    {
        index++;
    }

    bind(statement, b, index);
}

void bind(Statement& statement, std::initializer_list<const Bind>&& binds)
{
    int index = 0;
    for (auto b : binds)
    {
        if (did_error(statement))
        {
            break;
        }

        if (b.type == Bind::Type::column)
        {
            index = sqlite3_bind_parameter_index(statement.stmt, b.column);
        }
        else if (b.type == Bind::Type::index)
        {
            index = b.index;
        }
        else
        {
            index++;
        }

        bind(statement, b, index);
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

    stmt->sqlite_errcode = sqlite3_errcode(db);

    return stmt;
}

std::shared_ptr<Statement> Database::get_schema_version()
{
    auto stmt = prepare("PRAGMA user_version");
    step(*stmt);
    return stmt;
}

std::shared_ptr<Statement> Database::set_schema_version(SchemaVersion version)
{
    static const size_t max_size = 31; // 20 for 'PRAGMA user_version=', 10 for int, 1 for null char
    char statement[max_size];
    snprintf(statement, max_size, "PRAGMA user_version=%d", version);
    auto stmt = prepare(statement);
    if (is_ready(*stmt))
    {
        step(*stmt);
    }
    return stmt;
}

} // namespace slight
