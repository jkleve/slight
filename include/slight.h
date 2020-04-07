#ifndef SLIGHT_H
#define SLIGHT_H

#include <cstddef>
#include <condition_variable>
#include <initializer_list>
#include <mutex>
#include <string>
#include <vector>

#define SQLITE_OK                             0  /* Successful result */
#define SQLITE_ERROR                          1  /* Generic error */
#define SQLITE_OPEN_READONLY         0x00000001  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_READWRITE        0x00000002  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_CREATE           0x00000004  /* Ok for sqlite3_open_v2() */

// @todo would you ever want a transaction where you do an insert, select, update? or something like that
// where you would want to be iterating on the select results and is that allowed with sqlite3?
struct sqlite3;
struct sqlite3_stmt;

namespace slight {

class Bind;
class Database;
class Result;
class Q;

enum Access {
    kReadOnly = SQLITE_OPEN_READONLY,
    kReadWrite = SQLITE_OPEN_READWRITE,
    kCreateReadWrite = SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE,
};

enum ColumnType { kNull, kInt, kInt64, kUint, kFloat, kText, kBlob, kDatetime };

static const int SLIGHT_OK = SQLITE_OK;
static const int SLIGHT_ERROR = SQLITE_ERROR;

namespace details {
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
} // namespace details

template<ColumnType type> struct Typer {};
template<> struct                Typer<kInt>   { typedef int32_t     Type; };
template<> struct                Typer<kInt64> { typedef int64_t     Type; };
template<> struct                Typer<kUint>  { typedef uint32_t    Type; };
template<> struct                Typer<kFloat> { typedef double      Type; };
template<> struct                Typer<kText>  { typedef const char* Type; };

/// @brief Result
///
/// @note This class is used to access the results of a database operation.
///
class Result {
public:
    Result(sqlite3* db, details::stmt_ptr&& stmt, int error = SLIGHT_OK);
    Result(Result&& other) noexcept; // move
    Result(const Result& other) = delete; // copy
    ~Result() = default;

    Result& operator=(const Result& other) = delete; // assignment

    Result&           step();       /// Step to the next row
    bool              done() const; /// No more data
    bool             ready() const; /// No error and more data
    bool             error() const; /// Error
    int         error_code() const;
    const char*  error_msg() const;

    template<ColumnType type>
    typename Typer<type>::Type get(int index);

private:
    int m_error;
    sqlite3* m_db;
    details::stmt_ptr m_stmt;
};

class Bind {
    friend Q;
public:
    static const int kColumnIndexNotSet = -1;

    explicit Bind(int32_t i);
    explicit Bind(int64_t i);
    explicit Bind(uint32_t i);
    explicit Bind(float f);
    explicit Bind(const char* str);
    Bind(int index, int32_t i);
    Bind(int index, int64_t i);
    Bind(int index, uint32_t i);
    Bind(int index, float f);
    Bind(int index, const char* str);
    Bind(const char* column, int32_t i);
    Bind(const char* column, int64_t i);
    Bind(const char* column, uint32_t i);
    Bind(const char* column, float f);
    Bind(const char* column, const char* str);

    int bind(sqlite3_stmt* stmt, int column) const;
    int column() const;
    const char* column_name() const;

private:
    int m_index;
    const char* m_column_name;
    ColumnType m_type;
    union {
        int64_t m_i;
        float m_f;
        const char* m_str;
    };
    size_t m_str_size;
};

class Q {
public:
    explicit Q(std::string&& query);
    Q(const char* query, Bind&& bind);
    Q(const char* query, const Bind& bind);
    Q(const char* query, std::initializer_list<Bind>&& binds);
    Q(std::string&& query, Bind&& bind);
    Q(std::string&& query, std::initializer_list<Bind>&& binds);
    ~Q() = default;

    const char* str() const { return m_query.c_str(); }
    size_t str_size() const { return m_query.size(); }
    const std::vector<Bind>& binds() const { return m_binds; }

private:
    const std::string m_query;
    const std::vector<Bind> m_binds;
};

class ConnectionPool;

class Database {
public:
    // Database open(ConnectionPool* connection_pool);

    Database(std::string path, Access access);
    virtual ~Database();

    Result check_exists(const std::string& table_name);

    Result backup(int page_size); // @todo
    Result backup(const std::string& backup_path, int page_size, int flags); // @todo
    Result get_schema_version();
    Result set_schema_version(uint32_t version);
    Result start(const char* query);
    Result start(const char* query, Bind bind);
    Result start(Q&& query);
    // Result start(Q&& query, Bind bind);
    Result run(const char* query);
    Result run(const char* query, Bind bind);
    Result run(Q&& query);
    Result run(std::initializer_list<Q>&& queries);

    // @todo async
    //Result async(Q&& query, std::function<void(Result)> callback);

private:
    Database(sqlite3* db);
    sqlite3* getConnection();

    std::string m_path;
    bool m_open;
    sqlite3* m_db;
    ConnectionPool* m_connection_pool;
};

class ConnectionPool {
public:
    ConnectionPool(std::string path, Access access, size_t max_connections);
    ~ConnectionPool();

    sqlite3* pop();
    void push(sqlite3* connection);

private:
    const std::string m_path;
    const Access m_access;
    const size_t m_max_connections;
    size_t m_num_connections;
    mutable std::mutex m_mutex;
    mutable std::condition_variable m_connection_available;
    std::vector<sqlite3*> m_pool;
};

} // namespace slight

//#ifdef SLIGHT_HEADER
//#include "../src/Bind.cpp"
//#include "../src/Database.cpp"
//#include "../src/Query.cpp"
//#include "../src/Result.cpp"
//#include "../src/stmpt_ptr.cpp"
//#include "../src/utils.cpp"
//#endif

#endif // SLIGHT_H
