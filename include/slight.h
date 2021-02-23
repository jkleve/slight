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

struct Bind;

struct Statement {
    struct details;

    explicit Statement(details* me) : me(me) {}
    ~Statement() = default;

    bool ready() const;
    bool has_row() const;
    bool done() const;
    bool error() const;
    int error_code() const;
    const std::string& error_msg() const;
    std::string error_detail() const;

    void bind(const Bind& bind);
    void bind(std::initializer_list<const Bind>&& binds);

    void step();
    void reset();

    template<ColumnType type>
    typename Typer<type>::Type get(int index);

    void for_each(const std::function<bool(Statement* stmt)>& fn);

private:
    details* me;
};

struct Bind final {
    enum class Type { empty, index, column };
    enum class DataType { i32, i64, u32, flt, str };

    explicit Bind(int32_t value);
    explicit Bind(int64_t value);
    explicit Bind(uint32_t value);
    explicit Bind(float value);
    explicit Bind(const char* value);
    Bind(int index, int32_t value);
    Bind(int index, int64_t value);
    Bind(int index, uint32_t value);
    Bind(int index, float value);
    Bind(int index, const char* value);
    Bind(const char* column, int32_t value);
    Bind(const char* column, int64_t value);
    Bind(const char* column, uint32_t value);
    Bind(const char* column, float value);
    Bind(const char* column, const char* value);
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
    struct details;
    using SchemaVersion = std::uint32_t;

    static Database open_read_only(const std::string& path);
    static Database open_read_write(const std::string& path);
    static Database open_create_read_write(const std::string& path);
    static std::unique_ptr<Database> make_read_only(const std::string& path);
    static std::unique_ptr<Database> make_read_write(const std::string& path);
    static std::unique_ptr<Database> make_create_read_write(const std::string& path);

    explicit Database(details* me) : me(me) {}
    ~Database() = default;

    std::shared_ptr<Statement> prepare(const std::string& statement);
    // should i have a prepare_new_connection so statements don't use the same db connection?

    std::shared_ptr<Statement> get_schema_version();
    std::shared_ptr<Statement> set_schema_version(SchemaVersion version);

    /// @brief Path to database on disk.
    const std::string& path() const;

    /// @brief Indication that the database was opened successfully.
    bool opened() const;

    /// @brief Error message if the database failed to open.
    const std::string& error_msg() const;

private:
    details* me;
};

} // namespace slight

#endif // SLIGHT_H
