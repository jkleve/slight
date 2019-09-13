#include "slight.h"
#include <sqlite3.h>

#include <iostream> // @todo

namespace slight {

Sql::Sql(std::string&& sql) : m_sql(sql), m_binds() {}
Sql::Sql(const char* sql, std::initializer_list<Bind>&& binds) : m_sql(sql), m_binds(binds) {}
Sql::Sql(std::string&& sql, std::initializer_list<Bind>&& binds) : m_sql(sql), m_binds(binds) {}

const char* Sql::query() const { return m_sql.c_str(); }
size_t Sql::size() const       { return  m_sql.size(); }

bool Sql::compile(sqlite3* db, sqlite3_stmt** stmt)
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
                status = sqlite3_bind_int(*stmt, i++, bind.u32());
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
