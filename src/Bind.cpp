#include "slight.h"
#include <sqlite3.h>

#include <iostream> // @todo remove

namespace slight {

Bind::Bind(int32_t i)
    : m_index(-1)
    , m_column_name(nullptr)
    , m_type(kInt)
    , m_i(static_cast<int64_t>(i)) {}
Bind::Bind(int64_t i)
    : m_index(-1)
    , m_column_name(nullptr)
    , m_type(kInt64)
    , m_i(i) {}
Bind::Bind(uint32_t i)
    : m_index(-1)
    , m_column_name(nullptr)
    , m_type(kUint)
    , m_i(static_cast<int64_t>(i)) {}
Bind::Bind(float f)
    : m_index(-1)
    , m_column_name(nullptr)
    , m_type(kFloat)
    , m_f(f) {}
Bind::Bind(const char* str)
    : m_index(-1)
    , m_column_name(nullptr)
    , m_type(kText)
    , m_str(str)
    , m_str_size(str ? strlen(str) : 0) {}
Bind::Bind(int index, int32_t i)
    : m_index(index)
    , m_column_name(nullptr)
    , m_type(kInt)
    , m_i(i) {}
Bind::Bind(int index, int64_t i)
    : m_index(index)
    , m_column_name(nullptr)
    , m_type(kInt64)
    , m_i(i) {}
Bind::Bind(int index, uint32_t i)
    : m_index(index)
    , m_column_name(nullptr)
    , m_type(kUint)
    , m_i(i) {}
Bind::Bind(int index, float f)
    : m_index(index)
    , m_column_name(nullptr)
    , m_type(kFloat)
    , m_f(f) {}
Bind::Bind(int index, const char* str)
    : m_index(index)
    , m_column_name(nullptr)
    , m_type(kText)
    , m_str(str)
    , m_str_size(str ? strlen(str) : 0) {}
Bind::Bind(const char* column, int32_t i)
    : m_index(-1)
    , m_column_name(column)
    , m_type(kInt)
    , m_i(static_cast<int64_t>(i)) {}
Bind::Bind(const char* column, int64_t i)
    : m_index(-1)
    , m_column_name(column)
    , m_type(kInt64)
    , m_i(i) {}
Bind::Bind(const char* column, uint32_t i)
    : m_index(-1)
    , m_column_name(column)
    , m_type(kUint)
    , m_i(static_cast<int64_t>(i)) {}
Bind::Bind(const char* column, float f)
    : m_index(-1)
    , m_column_name(column)
    , m_type(kFloat)
    , m_f(f) {}
Bind::Bind(const char* column, const char* str)
    : m_index(-1)
    , m_column_name(column)
    , m_type(kText)
    , m_str(str)
    , m_str_size(str ? strlen(str) : 0) {}

int Bind::column() const { return m_index; }
const char* Bind::column_name() const { return m_column_name; }

int Bind::bind(sqlite3_stmt* stmt, int column) const
{
    int status = SQLITE_OK;
    switch (m_type)
    {
        case kInt:
            status = sqlite3_bind_int(stmt, column, m_i);
            break;
        case kInt64:
            status = sqlite3_bind_int64(stmt, column, m_i);
            break;
        case kUint:
            status = sqlite3_bind_int64(stmt, column, static_cast<sqlite3_int64>(m_i));
            break;
        case kFloat:
            status = sqlite3_bind_double(stmt, column, m_f);
            break;
        case kText:
            status = sqlite3_bind_text(stmt, column, m_str, m_str_size, nullptr);
            break;
        case kNull:
        case kBlob:
        case kDatetime:
        default:
            std::cout << "Not implemented\n";
    }
    return status;
}

} // namespace slight
