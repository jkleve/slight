#include "slight.h"
#include <sqlite3.h>

#include <iostream> // @todo

namespace slight {

Query::Query(std::string&& query) : m_query(query), m_binds() {}
Query::Query(const char* query, Bind&& bind) : m_query(query), m_binds({bind, }) {}
Query::Query(const char* query, std::initializer_list<Bind>&& binds) : m_query(query), m_binds(binds) {}
Query::Query(std::string&& query, std::initializer_list<Bind>&& binds) : m_query(query), m_binds(binds) {}

const char* Query::query() const { return m_query.c_str(); }
size_t Query::size() const       { return  m_query.size(); }

bool Query::compile(sqlite3* db, sqlite3_stmt** stmt)
{
    if (sqlite3_prepare_v3(db, query(), size(), 0, stmt, nullptr) != SQLITE_OK)
        return false;

    uint32_t i = 1;
    for (const auto& bind : m_binds)
    {
        int status = SQLITE_OK;
        switch (bind.type()) // @todo move this to bind.bind(sqlite3_stmt*)
        {
            case kInt:
                status = sqlite3_bind_int(*stmt, i++, bind.i32());
                break;
            case kInt64:
                status = sqlite3_bind_int64(*stmt, i++, bind.i64());
                break;
            case kUint:
                status = sqlite3_bind_int64(*stmt, i++, static_cast<sqlite3_int64>(bind.u32()));
                break;
            case kFloat:
                status = sqlite3_bind_double(*stmt, i++, bind.real());
                break;
            case kText:
                status = sqlite3_bind_text(*stmt, i++, bind.c_str(), bind.str_size(), nullptr);
                break;
            default:
                std::cout << "Not implemented" << std::endl;
                break;
        }
        if (status != SQLITE_OK)
        {
            return false;
        }
    }
    return true;
}

} // namespace slight
