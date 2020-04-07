#include "slight.h"
#include <sqlite3.h>

#include <iostream> // @todo

namespace slight {

Q::Q(std::string&& query) : m_query(query), m_binds() {}
Q::Q(const char* query, Bind&& bind) : m_query(query), m_binds({bind, }) {}
Q::Q(const char* query, const Bind& bind) : m_query(query), m_binds({bind, }) {}
Q::Q(const char* query, std::initializer_list<Bind>&& binds) : m_query(query), m_binds(binds) {}
Q::Q(std::string&& query, Bind&& bind) : m_query(query), m_binds({bind, }) {}
Q::Q(std::string&& query, std::initializer_list<Bind>&& binds) : m_query(query), m_binds(binds) {}

} // namespace slight
