#ifndef SLIGHT_H
#define SLIGHT_H

#include <stddef.h>
#include <initializer_list>
#include <string>
#include <vector>

#define SQLITE_OK 0
#define SQLITE_OPEN_READONLY         0x00000001  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_READWRITE        0x00000002  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_CREATE           0x00000004  /* Ok for sqlite3_open_v2() */

struct sqlite3;
struct sqlite3_stmt;

namespace slight {

class Bind;
class Database;
class Result;
class Sql;

enum Access {
    kReadOnly = SQLITE_OPEN_READONLY,
    kReadWrite = SQLITE_OPEN_READWRITE,
    kCreateReadWrite = SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE,
};

enum ColumnType { kNull, kInt, kFloat, kText, kBlob, kDatetime };


struct stmt_ptr {
    explicit stmt_ptr(sqlite3* db); // ctor
    stmt_ptr(stmt_ptr&& other) noexcept; // move
    stmt_ptr(const stmt_ptr& other); // copy
    ~stmt_ptr();

    stmt_ptr& operator=(const stmt_ptr& other); // assignment
    sqlite3_stmt** operator*(); // dereference
    explicit operator bool() const; // conditional check

    sqlite3_stmt* get() const;

private:
    bool         finalize;
    sqlite3*           db;
    sqlite3_stmt*    stmt;
};

template<ColumnType type> struct Typer {};
template<> struct                Typer<kInt>   { typedef int         Type; };
template<> struct                Typer<kFloat> { typedef double      Type; };
template<> struct                Typer<kText>  { typedef const char* Type; };

class Result {
public:
    static constexpr int kError = 1;

    Result(sqlite3* db, stmt_ptr stmt, int error = SQLITE_OK);
    ~Result() = default;

    explicit operator bool() const;

    Result&           step();
    bool              done() const;
    bool             error() const;
    int         error_code() const;
    const char*  error_msg() const;

    template<ColumnType type>
    typename Typer<type>::Type get(uint32_t index);

private:
    int m_error;
    sqlite3* m_db;
    stmt_ptr m_stmt;
};

class Bind {
public:
    explicit Bind(uint32_t i);
    explicit Bind(float f);
    explicit Bind(const char* str);

    ColumnType             type() const;
    uint32_t                u32() const;
    float                  real() const;
    const std::string&      str() const;
    size_t             str_size() const;
    const char*           c_str() const;

private:
    ColumnType m_type;
    uint32_t m_i;
    float m_f;
    std::string m_str;
};

class Sql {
    friend Database;
public:
    explicit Sql(std::string&& sql);
    Sql(const char* sql, std::initializer_list<Bind>&& binds);
    Sql(std::string&& sql, std::initializer_list<Bind>&& binds);
    ~Sql() = default;

    const char* query() const;
    size_t       size() const;

private:
    bool compile(sqlite3* db, sqlite3_stmt** stmt);

    std::string m_sql;
    std::vector<Bind> m_binds;
};

class Database {
public:
    Database(const std::string& path, Access access);
    virtual ~Database();

    Result backup(); // @todo
    Result backup(int page_size); // @todo
    Result getSchemaVersion();
    Result setSchemaVersion(int version);
    Result build(Sql&& sql);
    Result run(std::initializer_list<Sql>&& sqls);

private:
    struct transaction_t {
        transaction_t(sqlite3* db);
        ~transaction_t();
        void commit();
    private:
        sqlite3* db;
        bool committed;
    };
    sqlite3* m_db;
};

} // namespace slight

#endif // SLIGHT_H
