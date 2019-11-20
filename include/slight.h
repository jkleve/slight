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
class Query;

enum Access {
    kReadOnly = SQLITE_OPEN_READONLY,
    kReadWrite = SQLITE_OPEN_READWRITE,
    kCreateReadWrite = SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE,
};

enum ColumnType { kNull, kInt, kInt64, kUint, kFloat, kText, kBlob, kDatetime };


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
template<> struct                Typer<kInt>   { typedef int32_t     Type; };
template<> struct                Typer<kInt64> { typedef int64_t     Type; };
template<> struct                Typer<kUint>  { typedef uint32_t    Type; };
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
    typename Typer<type>::Type get(int index);

private:
    int m_error;
    sqlite3* m_db;
    stmt_ptr m_stmt;
};

class Bind {
public:
    explicit Bind(int32_t i);
    explicit Bind(int64_t i);
    explicit Bind(uint32_t i);
    explicit Bind(float f);
    explicit Bind(const char* str);
    Bind(const char* column, int32_t i);
    Bind(const char* column, int64_t i);
    Bind(const char* column, uint32_t i);
    Bind(const char* column, float f);
    Bind(const char* column, const char* str);

    ColumnType             type() const;
    int32_t                 i32() const;
    int64_t                 i64() const;
    uint32_t                u32() const;
    float                  real() const;
    const std::string&      str() const;
    size_t             str_size() const;
    const char*           c_str() const;

private:
    ColumnType m_type;
    int64_t m_i;
    float m_f;
    std::string m_str;
};

class Query {
    friend Database;
public:
    explicit Query(std::string&& query);
    Query(const char* query, Bind&& bind);
    Query(const char* query, std::initializer_list<Bind>&& binds);
    Query(std::string&& query, std::initializer_list<Bind>&& binds);
    ~Query() = default;

    const char* query() const;
    size_t       size() const;

private:
    bool compile(sqlite3* db, sqlite3_stmt** stmt);

    std::string m_query;
    std::vector<Bind> m_binds;
};

class Database {
public:
    Database(const std::string& path, Access access);
    virtual ~Database();

    Result backup(int page_size); // @todo
    Result backup(const std::string& backup_path, int page_size, int flags); // @todo
    Result getSchemaVersion();
    Result setSchemaVersion(int version);
    Result run(Query&& query);
    Result run(std::initializer_list<Query>&& queries);

private:
    struct transaction_t {
        transaction_t(sqlite3* db);
        ~transaction_t();
        void commit();
    private:
        sqlite3* db;
        const std::string path;
        bool committed;
    };
    sqlite3* m_db;
};

} // namespace slight

#ifdef SLIGHT_HEADER
#include "../src/Bind.cpp"
#include "../src/Database.cpp"
#include "../src/Query.cpp"
#include "../src/Result.cpp"
#include "../src/stmpt_ptr.cpp"
#include "../src/utils.cpp"
#endif

#endif // SLIGHT_H
