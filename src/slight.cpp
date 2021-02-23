#include "slight.h"
#include "sqlite3.h"

#include <cassert> // assert
#include <cstring> // strlen
#include <functional>

namespace slight {

/// @brief Statement
///
/// @note This class is used to access the results of a database operation.
///
struct Statement::details {
    details(sqlite3* db, std::string statement)
        : statement(std::move(statement))
        , sqlite_errcode(sqlite3_errcode(db))
        , db(db)
        , stmt() {}
    ~details() { sqlite3_finalize(stmt); }

    const std::string statement;

    int sqlite_errcode;
    std::string sqlite_errmsg;
    sqlite3* db;
    sqlite3_stmt* stmt;
};

bool Statement::ready() const { return !(done() || error()); }
bool Statement::has_row() const { return me->sqlite_errcode == SQLITE_ROW; }
bool Statement::done() const { return me->sqlite_errcode == SQLITE_DONE; }
bool Statement::error() const
{
    return
        me->sqlite_errcode != SQLITE_OK &&
        me->sqlite_errcode != SQLITE_ROW &&
        me->sqlite_errcode != SQLITE_DONE;
}
int Statement::error_code() const { return me->sqlite_errcode; }
const std::string& Statement::error_msg() const { return me->sqlite_errmsg; }
std::string Statement::error_detail() const { return "\"" + me->statement + "\" - " + error_msg(); }

int sqlite_bind(sqlite3_stmt* stmt, const Bind& bind, int index)
{
    assert(index > 0);
    switch (bind.data_type) {
        case Bind::DataType::i32:
            return sqlite3_bind_int(stmt, index, bind.i);
        case Bind::DataType::i64:
        case Bind::DataType::u32:
            return sqlite3_bind_int64(stmt, index, bind.i);
        case Bind::DataType::flt:
            return sqlite3_bind_double(stmt, index, bind.f);
        case Bind::DataType::str:
            return sqlite3_bind_text(stmt, index, bind.str, strlen(bind.str), nullptr);
    }
}

void Statement::bind(const Bind& b)
{
    int index = 0;

    if (b.type == Bind::Type::column)
    {
        index = sqlite3_bind_parameter_index(me->stmt, b.column);
    }
    else if (b.type == Bind::Type::index)
    {
        index = b.index;
    }
    else
    {
        index++;
    }

    me->sqlite_errcode = sqlite_bind(me->stmt, b, index);
}

void Statement::bind(std::initializer_list<const Bind>&& binds)
{
    int index = 0;
    for (auto b : binds)
    {
        if (error())
            break;

        switch (b.type) {
            case Bind::Type::column:
                index = sqlite3_bind_parameter_index(me->stmt, b.column);
                break;
            case Bind::Type::index:
                index = b.index;
                break;
            default:
                index++;
                break;
        }

        sqlite_bind(me->stmt, b, index);
    }
}

void Statement::step()
{
    if (!error())
    {
        me->sqlite_errcode = sqlite3_step(me->stmt);
        if (error())
            me->sqlite_errmsg = sqlite3_errmsg(me->db);
    }
}

void Statement::reset() { sqlite3_reset(me->stmt); }

template<>
Typer<i32>::Type Statement::get<i32>(int index) { return sqlite3_column_int(me->stmt, index - 1); }

template<>
Typer<i64>::Type Statement::get<i64>(int index) { return sqlite3_column_int64(me->stmt, index - 1); }

template<>
Typer<u32>::Type Statement::get<u32>(int index)
    { return static_cast<Typer<u32>::Type>(sqlite3_column_int64(me->stmt, index - 1)); }

template<>
Typer<flt>::Type Statement::get<flt>(int index)
    { return static_cast<Typer<flt>::Type>(sqlite3_column_double(me->stmt, index - 1)); }

template<>
Typer<text>::Type Statement::get<text>(int index)
    { return reinterpret_cast<Typer<text>::Type>(sqlite3_column_text(me->stmt, index - 1)); }

void Statement::for_each(const std::function<bool(Statement*)>& fn)
{
    for (step(); has_row(); step())
        fn(this);
}

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

struct Database::details {
    details(const std::string& path, int access)
    {
        status.opened = sqlite3_open_v2(path.c_str(), &db, access, nullptr) == SQLITE_OK;
        if (!status.opened)
            status.error_msg = sqlite3_errmsg(db);
        else
            // status.path = sqlite3_db_filename(db, path.c_str());
            status.path = path;
    }
    ~details() { sqlite3_close(db); }

    sqlite3* db{nullptr};
    struct {
        bool opened{false};
        std::string path;
        std::string error_msg;
    } status;
};

Database open(const std::string& path, int access)
{
    auto me = new Database::details(path, access);
    return Database(me);
}

Database Database::open_read_only(const std::string& path)
    { return open(path, SQLITE_OPEN_READONLY); }
Database Database::open_read_write(const std::string& path)
    { return open(path, SQLITE_OPEN_READWRITE); }
Database Database::open_create_read_write(const std::string& path)
    { return open(path, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE); }

std::unique_ptr<Database> make(const std::string& path, int access)
{
    auto me = new Database::details(path, access);
    return std::unique_ptr<Database>(new Database(me));
}

std::unique_ptr<Database> Database::make_read_only(const std::string& path)
    { return make(path, SQLITE_OPEN_READONLY); }
std::unique_ptr<Database> Database::make_read_write(const std::string& path)
    { return make(path, SQLITE_OPEN_READWRITE); }
std::unique_ptr<Database> Database::make_create_read_write(const std::string& path)
    { return make(path, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE); }

std::shared_ptr<Statement> Database::prepare(const std::string& statement)
{
    assert(!statement.empty());

    auto stmt_details = new Statement::details(me->db, statement);

    stmt_details->sqlite_errcode =
            sqlite3_prepare_v3(me->db, statement.c_str(), statement.size(), 0, &stmt_details->stmt, nullptr);

    auto stmt = std::make_shared<Statement>(stmt_details);
    if (stmt->error())
        stmt_details->sqlite_errmsg = sqlite3_errmsg(me->db);

    return stmt;
}

std::shared_ptr<Statement> Database::get_schema_version()
{
    auto stmt = prepare("PRAGMA user_version");
    stmt->step();
    return stmt;
}

std::shared_ptr<Statement> Database::set_schema_version(SchemaVersion version)
{
    static const size_t max_size = 31; // 20 for 'PRAGMA user_version=', 10 for int, 1 for null char
    char statement[max_size];
    snprintf(statement, max_size, "PRAGMA user_version=%d", version);
    auto stmt = prepare(statement);
    if (stmt->ready())
    {
        stmt->step();
    }
    return stmt;
}

const std::string& Database::path() const { return me->status.path; }
bool Database::opened() const { return me->status.opened; }
const std::string& Database::error_msg() const { return me->status.error_msg; }

} // namespace slight
