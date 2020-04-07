#include <gtest/gtest.h>
#include <slight.h>

#include <cstdio>
#include <memory>

using slight::Bind;
using slight::Database;
using slight::Q;
using slight::Result;

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
        db = std::make_unique<slight::Database>("tests.db", slight::Access::kCreateReadWrite);
    }
    ~TestSlight() override
    {
        db.reset();
        remove("tests.db");
    }

    std::unique_ptr<slight::Database> db;
};

void fail_test(Result& result)
{
    FAIL() << result.error_msg();
}

void create_test_table(Database& db)
{
    auto result = db.start("CREATE TABLE test(ID INT PRIMARY KEY NOT NULL, NAME TEXT NOT NULL)");
    if (result.ready())
    {
        result.step();
    }
    else
    {
        fail_test(result);
    }
}

void table_exists(Database& db)
{
    auto result = db.start(
        Q("SELECT name FROM sqlite_master WHERE type='table' AND name='test'"));
    if (result.ready())
    {
        result.step();
        EXPECT_FALSE(result.done());
    }
    else
    {
        fail_test(result);
    }
}

void table_does_not_exists(Database& db)
{
    auto result = db.start(
        Q("SELECT name FROM sqlite_master WHERE type='table' AND name='test'"));
    if (result.ready())
    {
        result.step();
        EXPECT_TRUE(result.done());
    }
    else
    {
        fail_test(result);
    }
}

TEST_F(TestSlight, tableDoesNotExist)
{
    auto result = db->check_exists("test");
    EXPECT_FALSE(result.ready());
    EXPECT_TRUE(result.done());
    EXPECT_FALSE(result.error());
    EXPECT_EQ(result.get<slight::kText>(1), nullptr);

    auto verify_exists = db->start(
        Q("SELECT name FROM sqlite_master WHERE type='table' AND name='test'"));
    EXPECT_FALSE(verify_exists.ready());
    EXPECT_TRUE(verify_exists.done());
    EXPECT_FALSE(verify_exists.error());
    EXPECT_EQ(verify_exists.get<slight::kText>(1), nullptr);
}

TEST_F(TestSlight, tableExists)
{
    auto result = db->run("CREATE TABLE test(id INT PRIMARY KEY NOT NULL, name TEXT NOT NULL, slight_int32 INT, "
                          "slight_int64 INT, slight_uint32 INT, slight_float FLOAT)");
    EXPECT_FALSE(result.ready());
    EXPECT_TRUE(result.done());
    EXPECT_FALSE(result.error());

    auto verify_exists = db->start(
        Q("SELECT name FROM sqlite_master WHERE type='table' AND name='test'"));
    EXPECT_TRUE(verify_exists.ready());
    EXPECT_FALSE(verify_exists.done());
    EXPECT_FALSE(verify_exists.error());
    EXPECT_STREQ(verify_exists.get<slight::kText>(1), "test");

    verify_exists.step();
    EXPECT_FALSE(verify_exists.ready());
    EXPECT_TRUE(verify_exists.done());
    EXPECT_FALSE(verify_exists.error());
    EXPECT_EQ(verify_exists.get<slight::kText>(1), nullptr);
}

TEST_F(TestSlight, getSchemaVersion)
{
    auto result = db->get_schema_version();
    EXPECT_TRUE(result.ready());
    EXPECT_FALSE(result.done());
    EXPECT_FALSE(result.error());
    EXPECT_EQ(result.get<slight::kInt>(1), 0);

    result.step();
    EXPECT_FALSE(result.ready());
    EXPECT_TRUE(result.done());
    EXPECT_FALSE(result.error());
    EXPECT_EQ(result.get<slight::kInt>(1), 0);
}

TEST_F(TestSlight, setSchemaVersion)
{
    auto set_version = db->set_schema_version(1);
    EXPECT_FALSE(set_version.ready());
    EXPECT_TRUE(set_version.done());
    EXPECT_FALSE(set_version.error());

    auto get_version = db->get_schema_version();
    EXPECT_TRUE(get_version.ready());
    EXPECT_FALSE(get_version.done());
    EXPECT_FALSE(get_version.error());
    EXPECT_EQ(get_version.get<slight::kInt>(1), 1);

    get_version.step();
    EXPECT_FALSE(get_version.ready());
    EXPECT_TRUE(get_version.done());
    EXPECT_FALSE(get_version.error());
    EXPECT_EQ(get_version.get<slight::kInt>(1), 0);
}

TEST_F(TestSlight, setSchemaVersionMax)
{
    auto set_version = db->set_schema_version(std::numeric_limits<int>::max());
    EXPECT_FALSE(set_version.ready());
    EXPECT_TRUE(set_version.done());
    EXPECT_FALSE(set_version.error());

    auto get_version = db->get_schema_version();
    EXPECT_TRUE(get_version.ready());
    EXPECT_FALSE(get_version.done());
    EXPECT_FALSE(get_version.error());
    EXPECT_EQ(get_version.get<slight::kInt>(1), std::numeric_limits<int>::max());

    get_version.step();
    EXPECT_FALSE(get_version.ready());
    EXPECT_TRUE(get_version.done());
    EXPECT_FALSE(get_version.error());
    EXPECT_EQ(get_version.get<slight::kInt>(1), 0);
}

TEST_F(TestSlight, insertValues)
{
    auto create = db->run("CREATE TABLE test(id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, slight_int32 INT, "
                          "slight_uint32 INT, slight_int64 INT, slight_float FLOAT)");
    EXPECT_FALSE(create.ready());
    EXPECT_TRUE(create.done());
    EXPECT_FALSE(create.error());

    auto insert = db->run("INSERT INTO test (name, slight_int32, slight_uint32, slight_int64, slight_float) VALUES "
                          "('name1', 0, 0, 0, 0.0),"
                          "('name2', 2147483647, 4294967295, 9.223372036854776e18, 189324123401393032.291302),"
                          "('name3', -2147483648, 0, -9.223372036854776e18, -189324123401393032.291302)"
                          );
    EXPECT_FALSE(insert.ready());
    EXPECT_TRUE(insert.done());
    EXPECT_FALSE(insert.error());

    auto select = db->start("SELECT * FROM test");
    EXPECT_TRUE(select.ready());
    EXPECT_FALSE(select.done());
    EXPECT_FALSE(select.error());

    EXPECT_EQ(        select.get<slight::kInt>(1),       1);
    EXPECT_STREQ(    select.get<slight::kText>(2), "name1");
    EXPECT_EQ(        select.get<slight::kInt>(3),       0);
    EXPECT_EQ(       select.get<slight::kUint>(4),       0);
    EXPECT_EQ(      select.get<slight::kInt64>(5),       0);
    EXPECT_FLOAT_EQ(select.get<slight::kFloat>(6),     0.0);

    select.step();
    EXPECT_TRUE(select.ready());
    EXPECT_FALSE(select.done());
    EXPECT_FALSE(select.error());

    EXPECT_EQ(        select.get<slight::kInt>(1),                         2);
    EXPECT_STREQ(    select.get<slight::kText>(2),                   "name2");
    EXPECT_EQ(        select.get<slight::kInt>(3),                2147483647);
    EXPECT_EQ(       select.get<slight::kUint>(4),                4294967295);
    EXPECT_EQ(      select.get<slight::kInt64>(5),      9.223372036854776e18);
    EXPECT_FLOAT_EQ(select.get<slight::kFloat>(6), 189324123401393032.291302);

    select.step();
    EXPECT_TRUE(select.ready());
    EXPECT_FALSE(select.done());
    EXPECT_FALSE(select.error());

    EXPECT_EQ(        select.get<slight::kInt>(1),                          3);
    EXPECT_STREQ(    select.get<slight::kText>(2),                    "name3");
    EXPECT_EQ(        select.get<slight::kInt>(3),                -2147483648);
    EXPECT_EQ(       select.get<slight::kUint>(4),                          0);
    EXPECT_EQ(      select.get<slight::kInt64>(5),      -9.223372036854776e18);
    EXPECT_FLOAT_EQ(select.get<slight::kFloat>(6), -189324123401393032.291302);

    select.step();
    EXPECT_FALSE(select.ready());
    EXPECT_TRUE(select.done());
    EXPECT_FALSE(select.error());
}
