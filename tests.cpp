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

void fail_test(slight::Statement& statement)
{
    FAIL() << error_msg(statement);
}

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

TEST_F(TestSlight, initial_schema_version)
{
    auto ctx = db->get_schema_version();
    EXPECT_TRUE(slight::is_ready(*ctx));
    EXPECT_TRUE(slight::has_row(*ctx));
    EXPECT_FALSE(slight::is_done(*ctx));
    EXPECT_FALSE(slight::did_error(*ctx));
    EXPECT_EQ(slight::get<slight::i32>(*ctx, 1), 0);

    slight::step(*ctx);
    EXPECT_FALSE(slight::is_ready(*ctx));
    EXPECT_FALSE(slight::has_row(*ctx));
    EXPECT_TRUE(slight::is_done(*ctx));
    EXPECT_FALSE(slight::did_error(*ctx));
    EXPECT_EQ(slight::get<slight::i32>(*ctx, 1), 0);
}

TEST_F(TestSlight, set_get_schema_version)
{
    auto set_version = db->set_schema_version(1);
    EXPECT_FALSE(slight::is_ready(*set_version));
    EXPECT_FALSE(slight::has_row(*set_version));
    EXPECT_TRUE(slight::is_done(*set_version));
    EXPECT_FALSE(slight::did_error(*set_version));

    auto get_version = db->get_schema_version();
    EXPECT_TRUE(slight::is_ready(*get_version));
    EXPECT_TRUE(slight::has_row(*get_version));
    EXPECT_FALSE(slight::is_done(*get_version));
    EXPECT_FALSE(slight::did_error(*get_version));
    EXPECT_EQ(slight::get<slight::i32>(*get_version, 1), 1);

    slight::step(*get_version);
    EXPECT_FALSE(slight::is_ready(*get_version));
    EXPECT_FALSE(slight::has_row(*get_version));
    EXPECT_TRUE(slight::is_done(*get_version));
    EXPECT_FALSE(slight::did_error(*get_version));
    EXPECT_EQ(slight::get<slight::i32>(*get_version, 1), 0);
}

TEST_F(TestSlight, set_schema_version_max)
{
    auto set_version = db->set_schema_version(std::numeric_limits<int>::max());
    EXPECT_FALSE(slight::is_ready(*set_version));
    EXPECT_FALSE(slight::has_row(*set_version));
    EXPECT_TRUE(slight::is_done(*set_version));
    EXPECT_FALSE(slight::did_error(*set_version));

    auto get_version = db->get_schema_version();
    EXPECT_TRUE(slight::is_ready(*get_version));
    EXPECT_TRUE(slight::has_row(*get_version));
    EXPECT_FALSE(slight::is_done(*get_version));
    EXPECT_FALSE(slight::did_error(*get_version));
    EXPECT_EQ(slight::get<slight::i32>(*get_version, 1), std::numeric_limits<int>::max());

    slight::step(*get_version);
    EXPECT_FALSE(slight::is_ready(*get_version));
    EXPECT_FALSE(slight::has_row(*get_version));
    EXPECT_TRUE(slight::is_done(*get_version));
    EXPECT_FALSE(slight::did_error(*get_version));
    EXPECT_EQ(slight::get<slight::i32>(*get_version, 1), 0);
}

TEST_F(TestSlight, insert_values)
{
    auto create = db->prepare(
        "CREATE TABLE test(id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, slight_int32 INT, "
        "slight_uint32 INT, slight_int64 INT, slight_float FLOAT)"
    );
    EXPECT_TRUE(slight::is_ready(*create));
    EXPECT_FALSE(slight::is_done(*create));
    EXPECT_FALSE(slight::did_error(*create));

    slight::step(*create);
    EXPECT_FALSE(slight::is_ready(*create));
    EXPECT_TRUE(slight::is_done(*create));
    EXPECT_FALSE(slight::did_error(*create));

    auto insert = db->prepare(
        "INSERT INTO test (name, slight_int32, slight_uint32, slight_int64, slight_float) VALUES "
        "('name1', 0, 0, 0, 0.0),"
        "('name2', 2147483647, 4294967295, 9.223372036854776e18, 189324123401393032.291302),"
        "('name3', -2147483648, 0, -9.223372036854776e18, -189324123401393032.291302)"
    );
    EXPECT_TRUE(slight::is_ready(*insert));
    EXPECT_FALSE(slight::is_done(*insert));
    EXPECT_FALSE(slight::did_error(*insert));

    slight::step(*insert);
    EXPECT_FALSE(slight::is_ready(*insert));
    EXPECT_TRUE(slight::is_done(*insert));
    EXPECT_FALSE(slight::did_error(*insert));

    auto select = db->prepare("SELECT * FROM test");
    EXPECT_TRUE(slight::is_ready(*select));
    EXPECT_FALSE(slight::has_row(*select));
    EXPECT_FALSE(slight::is_done(*select));
    EXPECT_FALSE(slight::did_error(*select));

    slight::step(*select);
    EXPECT_TRUE(slight::is_ready(*select));
    EXPECT_TRUE(slight::has_row(*select));
    EXPECT_FALSE(slight::is_done(*select));
    EXPECT_FALSE(slight::did_error(*select));

    EXPECT_EQ(      slight::get<slight::i32>(*select, 1),       1);
    EXPECT_STREQ(  slight::get<slight::text>(*select, 2), "name1");
    EXPECT_EQ(      slight::get<slight::i32>(*select, 3),       0);
    EXPECT_EQ(      slight::get<slight::u32>(*select, 4),       0);
    EXPECT_EQ(      slight::get<slight::i64>(*select, 5),       0);
    EXPECT_FLOAT_EQ(slight::get<slight::flt>(*select, 6),     0.0);

    slight::step(*select);
    EXPECT_TRUE(slight::is_ready(*select));
    EXPECT_TRUE(slight::has_row(*select));
    EXPECT_FALSE(slight::is_done(*select));
    EXPECT_FALSE(slight::did_error(*select));

    EXPECT_EQ(       slight::get<slight::i32>(*select, 1),                         2);
    EXPECT_STREQ(   slight::get<slight::text>(*select, 2),                   "name2");
    EXPECT_EQ(       slight::get<slight::i32>(*select, 3),                2147483647);
    EXPECT_EQ(       slight::get<slight::u32>(*select, 4),                4294967295);
    EXPECT_EQ(       slight::get<slight::i64>(*select, 5),      9.223372036854776e18);
    EXPECT_FLOAT_EQ( slight::get<slight::flt>(*select, 6), 189324123401393032.291302);

    slight::step(*select);
    EXPECT_TRUE(slight::is_ready(*select));
    EXPECT_TRUE(slight::has_row(*select));
    EXPECT_FALSE(slight::is_done(*select));
    EXPECT_FALSE(slight::did_error(*select));

    EXPECT_EQ(       slight::get<slight::i32>(*select, 1),                          3);
    EXPECT_STREQ(   slight::get<slight::text>(*select, 2),                    "name3");
    EXPECT_EQ(       slight::get<slight::i32>(*select, 3),                -2147483648);
    EXPECT_EQ(       slight::get<slight::u32>(*select, 4),                          0);
    EXPECT_EQ(       slight::get<slight::i64>(*select, 5),      -9.223372036854776e18);
    EXPECT_FLOAT_EQ( slight::get<slight::flt>(*select, 6), -189324123401393032.291302);

    slight::step(*select);
    EXPECT_FALSE(slight::is_ready(*select));
    EXPECT_FALSE(slight::has_row(*select));
    EXPECT_TRUE(slight::is_done(*select));
    EXPECT_FALSE(slight::did_error(*select));
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
