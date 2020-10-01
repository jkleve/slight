#ifndef SLIGHT_H
#define SLIGHT_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

struct sqlite3;

namespace slight {

// @todo ColumnType & Bind::DataType are the same mapping. combine?
enum ColumnType { nil, i32, i64, u32, flt, text, blob, datetime };

template<ColumnType type> struct Typer {};
template<> struct                Typer<i32>  { typedef int32_t     Type; };
template<> struct                Typer<i64>  { typedef int64_t     Type; };
template<> struct                Typer<u32>  { typedef uint32_t    Type; };
template<> struct                Typer<flt>  { typedef double      Type; };
template<> struct                Typer<text> { typedef const char* Type; };

class Statement;

struct Bind final {
    enum class Type { empty, index, column };
    enum class DataType { i32, i64, u32, flt, str };

    explicit Bind(int32_t value);
    explicit Bind(int64_t value);
    explicit Bind(uint32_t value);
    explicit Bind(float value);
    explicit Bind(const char* value);
    Bind(const char* column, int32_t value);
    Bind(const char* column, int64_t value);
    Bind(const char* column, uint32_t value);
    Bind(const char* column, float value);
    Bind(const char* column, const char* value);
    //Bind(const char* column, const char* value, std::size_t size);
    ~Bind() = default;

    const Type type;
    const union {
        const int index;
        const char* column;
    };

    const DataType data_type;
    const union {
        const int64_t i;
        const float f;
        const char* str;
    };
};

class Database final {
public:
    using SchemaVersion = uint32_t;

    static Database open_read_only(const std::string& path);
    static Database open_read_write(const std::string& path);
    static Database open_create_read_write(const std::string& path);
    static std::unique_ptr<Database> make_read_only(const std::string& path);
    static std::unique_ptr<Database> make_read_write(const std::string& path);
    static std::unique_ptr<Database> make_create_read_write(const std::string& path);

    ~Database();

    //std::shared_ptr<Statement> check_exists(const std::string& table_name);

    //Statement backup(int page_size);
    //Statement backup(const std::string& backup_path, int page_size, int flags);

    std::shared_ptr<Statement> get_schema_version();
    std::shared_ptr<Statement> set_schema_version(SchemaVersion version);

    std::shared_ptr<Statement> prepare(const std::string& statement);
    //Statement prepare(const char* statement, const Bind& bind);
    //Statement prepare(const char* statement, std::initializer_list<const Bind&>&& binds);
    /// @feature: multiple queries at once

    // Statement execute(const char* query);
    // Statement execute(const char* query, Bind bind);
    // Statement execute(const char* query, std::initializer_list<Bind>& binds);
    // Statement execute(Q&& query);
    // Statement execute(std::initializer_list<Q>&& queries);

    const std::string& path;
    const bool opened;
    const std::string error;

private:
    Database(const std::string& path, bool opened, sqlite3* db);
    static Database open_db(const std::string& path, int access);
    static std::unique_ptr<Database> make_db(const std::string& path, int access);

    sqlite3* db;
};

/// Bind
void bind(Statement& statement, Bind bind);
void bind(Statement& statement, const std::vector<Bind>& bind);

/// Checks
bool is_ready(const Statement& statement);  /// Data ready and no errors
bool is_done(const Statement& statement);   /// No more data
bool did_error(const Statement& statement); /// Error

/// Modifiers
void step(Statement& statement);
void reset(Statement& statement);

/// Access
template<ColumnType type>
typename Typer<type>::Type get(Statement& statement, int index);

/// Errors
int error_code(const Statement& statement);
const char* error_msg(const Statement& statement);

/// @feature: connection ppol
//class ConnectionPool;

} // namespace slight

#endif // SLIGHT_H
