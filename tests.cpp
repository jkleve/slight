#include <gtest/gtest.h>
#include <slight.h>

#include <cstdio>
#include <memory>

using slight::Bind;

#if __cplusplus <= 201103L
namespace std {
template<class T, class... Args>
unique_ptr<T> make_unique(Args&&... args) { return unique_ptr<T>(new T(std::forward<Args>(args)...)); }
} // namespace std
#endif

class TestSlight : public ::testing::Test {
public:
    TestSlight()
        : db()
    {
        remove("tests.db");
        db = slight::Database::make_create_read_write("tests.db");
    }
    ~TestSlight() override
    {
        db.reset();
        remove("tests.db");
    }

    std::unique_ptr<slight::Database> db;
};

//void create_test_table(Database& db)
//{
//    auto stmt = db.prepare("CREATE TABLE test(ID INT PRIMARY KEY NOT NULL, NAME TEXT NOT NULL)");
//    if (stmt.ready())
//    {
//        stmt.step();
//    }
//    else
//    {
//        fail_test(stmt);
//    }
//}
//
//void table_exists(Database& db)
//{
//    auto result = db.start(
//        "SELECT name FROM sqlite_master WHERE type='table' AND name='test'");
//    if (result.ready())
//    {
//        result.step();
//        EXPECT_FALSE(result.done());
//    }
//    else
//    {
//        fail_test(result);
//    }
//}
//
//void table_does_not_exists(Database& db)
//{
//    auto result = db.start(
//        "SELECT name FROM sqlite_master WHERE type='table' AND name='test'");
//    if (result.ready())
//    {
//        result.step();
//        EXPECT_TRUE(result.done());
//    }
//    else
//    {
//        fail_test(result);
//    }
//}
//
//TEST_F(TestSlight, tableDoesNotExist)
//{
//    auto result = db->check_exists("test");
//    EXPECT_FALSE(result.ready());
//    EXPECT_TRUE(result.done());
//    EXPECT_FALSE(result.error());
//    EXPECT_EQ(result.get<slight::kText>(1), nullptr);
//
//    auto verify_exists = db->start(
//        "SELECT name FROM sqlite_master WHERE type='table' AND name='test'");
//    EXPECT_FALSE(verify_exists.ready());
//    EXPECT_TRUE(verify_exists.done());
//    EXPECT_FALSE(verify_exists.error());
//    EXPECT_EQ(verify_exists.get<slight::kText>(1), nullptr);
//}

//TEST_F(TestSlight, tableExists)
//{
//    auto result = db->run("CREATE TABLE test(id INT PRIMARY KEY NOT NULL, name TEXT NOT NULL, slight_int32 INT, "
//                          "slight_int64 INT, slight_uint32 INT, slight_float FLOAT)");
//    EXPECT_FALSE(result.ready());
//    EXPECT_TRUE(result.done());
//    EXPECT_FALSE(result.error());
//
//    auto verify_exists = db->start(
//        "SELECT name FROM sqlite_master WHERE type='table' AND name='test'");
//    EXPECT_TRUE(verify_exists.ready());
//    EXPECT_FALSE(verify_exists.done());
//    EXPECT_FALSE(verify_exists.error());
//    EXPECT_STREQ(verify_exists.get<slight::kText>(1), "test");
//
//    verify_exists.step();
//    EXPECT_FALSE(verify_exists.ready());
//    EXPECT_TRUE(verify_exists.done());
//    EXPECT_FALSE(verify_exists.error());
//    EXPECT_EQ(verify_exists.get<slight::kText>(1), nullptr);
//}

// @todo open_read_only
// @todo open_read_write
// @todo open_create_read_write

TEST_F(TestSlight, initial_schema_version)
{
    auto ctx = db->get_schema_version();
    EXPECT_TRUE(ctx->ready());
    EXPECT_TRUE(ctx->has_row());
    EXPECT_FALSE(ctx->done());
    EXPECT_FALSE(ctx->error());
    EXPECT_EQ(ctx->get<slight::i32>(1), 0);

    ctx->step();
    EXPECT_FALSE(ctx->ready());
    EXPECT_FALSE(ctx->has_row());
    EXPECT_TRUE(ctx->done());
    EXPECT_FALSE(ctx->error());
    EXPECT_EQ(ctx->get<slight::i32>(1), 0);
}

TEST_F(TestSlight, set_get_schema_version)
{
    auto set_version = db->set_schema_version(1);
    EXPECT_FALSE(set_version->ready());
    EXPECT_FALSE(set_version->has_row());
    EXPECT_TRUE(set_version->done());
    EXPECT_FALSE(set_version->error());

    auto get_version = db->get_schema_version();
    EXPECT_TRUE(get_version->ready());
    EXPECT_TRUE(get_version->has_row());
    EXPECT_FALSE(get_version->done());
    EXPECT_FALSE(get_version->error());
    EXPECT_EQ(get_version->get<slight::i32>(1), 1);

    get_version->step();
    EXPECT_FALSE(set_version->ready());
    EXPECT_FALSE(set_version->has_row());
    EXPECT_TRUE(set_version->done());
    EXPECT_FALSE(set_version->error());
    EXPECT_EQ(get_version->get<slight::i32>(1), 0);
}

TEST_F(TestSlight, set_schema_version_max)
{
    auto set_version = db->set_schema_version(std::numeric_limits<int>::max());
    EXPECT_FALSE(set_version->ready());
    EXPECT_FALSE(set_version->has_row());
    EXPECT_TRUE(set_version->done());
    EXPECT_FALSE(set_version->error());

    auto get_version = db->get_schema_version();
    EXPECT_TRUE(get_version->ready());
    EXPECT_TRUE(get_version->has_row());
    EXPECT_FALSE(get_version->done());
    EXPECT_FALSE(get_version->error());
    EXPECT_EQ(get_version->get<slight::i32>(1), std::numeric_limits<int>::max());

    get_version->step();
    EXPECT_FALSE(set_version->ready());
    EXPECT_FALSE(set_version->has_row());
    EXPECT_TRUE(set_version->done());
    EXPECT_FALSE(set_version->error());
    EXPECT_EQ(get_version->get<slight::i32>(1), 0);
}

TEST_F(TestSlight, insert_values)
{
    auto create = db->prepare(
        "CREATE TABLE test(id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, slight_int32 INT, "
        "slight_uint32 INT, slight_int64 INT, slight_float FLOAT)"
    );
    EXPECT_TRUE(create->ready());
    EXPECT_FALSE(create->done());
    EXPECT_FALSE(create->error());

    create->step();
    EXPECT_FALSE(create->ready());
    EXPECT_TRUE(create->done());
    EXPECT_FALSE(create->error());

    auto insert = db->prepare(
        "INSERT INTO test (name, slight_int32, slight_uint32, slight_int64, slight_float) VALUES "
        "('name1', 0, 0, 0, 0.0),"
        "('name2', 2147483647, 4294967295, 9.223372036854776e18, 189324123401393032.291302),"
        "('name3', -2147483648, 0, -9.223372036854776e18, -189324123401393032.291302)"
    );
    EXPECT_TRUE(insert->ready());
    EXPECT_FALSE(insert->done());
    EXPECT_FALSE(insert->error());

    insert->step();
    EXPECT_FALSE(insert->ready());
    EXPECT_TRUE(insert->done());
    EXPECT_FALSE(insert->error());

    auto select = db->prepare("SELECT * FROM test");
    EXPECT_TRUE(select->ready());
    EXPECT_FALSE(select->has_row());
    EXPECT_FALSE(select->done());
    EXPECT_FALSE(select->error());

    select->step();
    EXPECT_TRUE(select->ready());
    EXPECT_TRUE(select->has_row());
    EXPECT_FALSE(select->done());
    EXPECT_FALSE(select->error());

    EXPECT_EQ(      select->get<slight::i32>(1),       1);
    EXPECT_STREQ(  select->get<slight::text>(2), "name1");
    EXPECT_EQ(      select->get<slight::i32>(3),       0);
    EXPECT_EQ(      select->get<slight::u32>(4),       0);
    EXPECT_EQ(      select->get<slight::i64>(5),       0);
    EXPECT_FLOAT_EQ(select->get<slight::flt>(6),     0.0);

    select->step();
    EXPECT_TRUE(select->ready());
    EXPECT_TRUE(select->has_row());
    EXPECT_FALSE(select->done());
    EXPECT_FALSE(select->error());

/* @todo taken from sqlite3.h
** CAPI3REF: 64-Bit Integer Types
** KEYWORDS: sqlite_int64 sqlite_uint64
**
** Because there is no cross-platform way to specify 64-bit integer types
** SQLite includes typedefs for 64-bit signed and unsigned integers.
**
** The sqlite3_int64 and sqlite3_uint64 are the preferred type definitions.
** The sqlite_int64 and sqlite_uint64 types are supported for backwards
** compatibility only.
**
** ^The sqlite3_int64 and sqlite_int64 types can store integer values
** between -9223372036854775808 and +9223372036854775807 inclusive.  ^The
** sqlite3_uint64 and sqlite_uint64 types can store integer values
** between 0 and +18446744073709551615 inclusive.
*/
    EXPECT_EQ(       select->get<slight::i32>(1),                         2);
    EXPECT_STREQ(   select->get<slight::text>(2),                   "name2");
    EXPECT_EQ(       select->get<slight::i32>(3),                2147483647);
    EXPECT_EQ(       select->get<slight::u32>(4),                4294967295);
    EXPECT_EQ(       select->get<slight::i64>(5),      9.223372036854776e18);
    EXPECT_FLOAT_EQ( select->get<slight::flt>(6), 189324123401393032.291302);

    select->step();
    EXPECT_TRUE(select->ready());
    EXPECT_TRUE(select->has_row());
    EXPECT_FALSE(select->done());
    EXPECT_FALSE(select->error());

    EXPECT_EQ(       select->get<slight::i32>(1),                          3);
    EXPECT_STREQ(   select->get<slight::text>(2),                    "name3");
    EXPECT_EQ(       select->get<slight::i32>(3),                -2147483648);
    EXPECT_EQ(       select->get<slight::u32>(4),                          0);
    EXPECT_EQ(       select->get<slight::i64>(5),      -9.223372036854776e18);
    EXPECT_FLOAT_EQ( select->get<slight::flt>(6), -189324123401393032.291302);

    select->step();
    EXPECT_FALSE(select->ready());
    EXPECT_FALSE(select->has_row());
    EXPECT_TRUE(select->done());
    EXPECT_FALSE(select->error());
}

TEST_F(TestSlight, bind)
{
    // expect:
    // -
}

//TEST_F(TestSlight, insertValuesIteratively)
//{
//    auto create = db->run("CREATE TABLE test(id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL");
//    EXPECT_FALSE(create.ready());
//    EXPECT_TRUE(create.done());
//    EXPECT_FALSE(create.error());
//
//    auto insert = db->run({
//        Q("INSERT INTO test (name) VALUES(?),", Bind("name 1")),
//        Q("INSERT INTO test (name) VALUES(?),", Bind("name 2")),
//        Q("INSERT INTO test (name) VALUES(?),", Bind("name 3"))
//    });
//
//    EXPECT_FALSE(insert.ready());
//    EXPECT_TRUE(insert.done());
//    EXPECT_FALSE(insert.error());
//
//    auto select = db->start("SELECT * FROM test");
//    EXPECT_TRUE(select.ready());
//    EXPECT_FALSE(select.done());
//    EXPECT_FALSE(select.error());
//
//    EXPECT_STREQ(select.get<slight::kText>(1), "name1");
//
//    select.step();
//    EXPECT_TRUE(select.ready());
//    EXPECT_FALSE(select.done());
//    EXPECT_FALSE(select.error());
//
//    EXPECT_STREQ(select.get<slight::kText>(1), "name2");
//
//    select.step();
//    EXPECT_TRUE(select.ready());
//    EXPECT_FALSE(select.done());
//    EXPECT_FALSE(select.error());
//
//    EXPECT_STREQ(select.get<slight::kText>(1), "name3");
//
//    select.step();
//    EXPECT_FALSE(select.ready());
//    EXPECT_TRUE(select.done());
//    EXPECT_FALSE(select.error());
//}
