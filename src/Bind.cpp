#include "slight.h"

namespace slight {

Bind::Bind(int32_t i) : m_type(kInt), m_i(static_cast<int64_t>(i)) {}
Bind::Bind(int64_t i) : m_type(kInt64), m_i(i) {}
Bind::Bind(uint32_t i) : m_type(kUint), m_i(static_cast<int64_t>(i)) {}
Bind::Bind(float f) : m_type(kFloat), m_f(f) {}
Bind::Bind(const char* str) : m_type(kText), m_str(str) {}

Bind::Bind(const char* column, int32_t i) : m_type(kInt), m_i(static_cast<int64_t>(i)) {}
Bind::Bind(const char* column, int64_t i) : m_type(kInt64), m_i(i) {}
Bind::Bind(const char* column, uint32_t i) : m_type(kUint), m_i(static_cast<int64_t>(i)) {}
Bind::Bind(const char* column, float f) : m_type(kFloat), m_f(f) {}
Bind::Bind(const char* column, const char* str) : m_type(kText), m_str(str) {}

ColumnType             Bind::type() const { return        m_type; }
int32_t                 Bind::i32() const { return           m_i; }
int64_t                 Bind::i64() const { return           m_i; }
uint32_t                Bind::u32() const { return           m_i; }
float                  Bind::real() const { return           m_f; }
const std::string&      Bind::str() const { return         m_str; }
size_t             Bind::str_size() const { return  m_str.size(); }
const char*           Bind::c_str() const { return m_str.c_str(); }

} // namespace slight
