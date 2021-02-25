#include <gtest/gtest.h>
#include <slight.h>
#include <sqlite3.h>

#include <cstdio>
#include <memory>

using slight::Bind;

// @todo get_schema_version should return and int
// @todo forward iterator. it's not natural but worth it for your users

void check(const slight::Statement& stmt)
{
    if (stmt.error())
    {
        throw std::runtime_error(std::to_string(stmt.error_code()) + ": " + stmt.error_msg());
    }
}

///  taken from sqlite3.h
///
//** CAPI3REF: 64-Bit Integer Types
//** KEYWORDS: sqlite_int64 sqlite_uint64
//**
//** Because there is no cross-platform way to specify 64-bit integer types
//** SQLite includes typedefs for 64-bit signed and unsigned integers.
//**
//** The sqlite3_int64 and sqlite3_uint64 are the preferred type definitions.
//** The sqlite_int64 and sqlite_uint64 types are supported for backwards
//** compatibility only.
//**
//** ^The sqlite3_int64 and sqlite_int64 types can store integer values
//** between -9223372036854775808 and +9223372036854775807 inclusive.  ^The
//** sqlite3_uint64 and sqlite_uint64 types can store integer values
//** between 0 and +18446744073709551615 inclusive.
//*/
class TestSlight : public ::testing::Test {
public:
    TestSlight()
        : db()
    {
        remove("tests.db");
        db = slight::Database::make_create_read_write("tests.db");

        auto create = db->prepare(
            "CREATE TABLE test(id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, slight_int32 INT, "
            "slight_uint32 INT, slight_int64 INT, slight_uint64 INT, slight_float FLOAT)"
        );
        create->step();
        check(*create);

        auto insert = db->prepare(
            "INSERT INTO test (name, slight_int32, slight_uint32, slight_int64, slight_uint64, slight_float) VALUES "
            "('name1', 0, 0, 0, 0, 0.0),"
            "('name2', 2147483647, 4294967295, 9223372036854775808, 18446744073709551615, 189324123401393032.291302),"
            "('name3', -2147483648, 0, -9223372036854775808, 0, -189324123401393032.291302),"
            "('future proof', 0, 0, 0, 0, 0.0),"
            "('future proof', 0, 0, 0, 0, 0.0),"
            "('future proof', 0, 0, 0, 0, 0.0)"
        );
        insert->step();
        check(*insert);
    }
    ~TestSlight() override
    {
        db.reset();
    }

    std::unique_ptr<slight::Database> db;
};

TEST_F(TestSlight, test_done)
{
    auto select = db->prepare("SELECT * FROM test");
    EXPECT_TRUE(select->ready());

    select->step();
    select->step();
    select->step();
    select->step();
    select->step();
    select->step();
    EXPECT_TRUE(select->ready());

    select->step();
    EXPECT_FALSE(select->ready());
    EXPECT_TRUE(select->done());
}

TEST_F(TestSlight, test_malformed_query)
{
    auto select = db->prepare("SELECT x FROM test");
    EXPECT_TRUE(select->error());
    EXPECT_FALSE(select->ready());
    EXPECT_FALSE(select->done());
    EXPECT_FALSE(select->has_row());
    EXPECT_EQ(select->error_code(), SQLITE_ERROR);
    EXPECT_NE(select->error_msg(), "");
}

TEST_F(TestSlight, test_error)
{
}

TEST_F(TestSlight, test_select_all)
{
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
    EXPECT_FLOAT_EQ(select->get<slight::flt>(7),     0.0);

    select->step();
    EXPECT_TRUE(select->ready());
    EXPECT_TRUE(select->has_row());
    EXPECT_FALSE(select->done());
    EXPECT_FALSE(select->error());

    EXPECT_EQ(       select->get<slight::i32>(1),                         2);
    EXPECT_STREQ(   select->get<slight::text>(2),                   "name2");
    EXPECT_EQ(       select->get<slight::i32>(3),                2147483647);
    EXPECT_EQ(       select->get<slight::u32>(4),                4294967295);
    EXPECT_EQ(       select->get<slight::i64>(5),      9.223372036854776e18);
    EXPECT_FLOAT_EQ( select->get<slight::flt>(7), 189324123401393032.291302);

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
    EXPECT_FLOAT_EQ( select->get<slight::flt>(7), -189324123401393032.291302);

    while (select->step())
        EXPECT_TRUE(select->has_row());
    EXPECT_TRUE(select->done());
}

TEST_F(TestSlight, test_reset)
{
    auto select = db->prepare("SELECT name FROM test");

    select->step();
    EXPECT_STREQ(select->get<slight::text>(1), "name1");

    select->step();
    EXPECT_STREQ(select->get<slight::text>(1), "name2");

    select->reset();

    select->step();
    EXPECT_STREQ(select->get<slight::text>(1), "name1");
}

TEST_F(TestSlight, test_select_index)
{
    auto select = db->prepare("SELECT id FROM test");

    select->step();
    EXPECT_STREQ(select->get<slight::text>(1), "1");
    EXPECT_DOUBLE_EQ(select->get<slight::flt>(1), 1.0);
    EXPECT_EQ(select->get<slight::i32>(1), 1);
    EXPECT_EQ(select->get<slight::u32>(1), 1);
    EXPECT_EQ(select->get<slight::i64>(1), 1);
    EXPECT_FALSE(select->error());

    select->step();
    select->step();
    EXPECT_STREQ(select->get<slight::text>(1), "3");
    EXPECT_DOUBLE_EQ(select->get<slight::flt>(1), 3.0);
    EXPECT_EQ(select->get<slight::i32>(1), 3);
    EXPECT_EQ(select->get<slight::u32>(1), 3);
    EXPECT_EQ(select->get<slight::i64>(1), 3);
}

TEST_F(TestSlight, test_select_i64_zero)
{
    auto select = db->prepare("SELECT slight_int64 FROM test");

    select->step();
    EXPECT_STREQ(select->get<slight::text>(1), "0");
    EXPECT_DOUBLE_EQ(select->get<slight::flt>(1), 0.0);
    EXPECT_EQ(select->get<slight::i32>(1), 0);
    EXPECT_EQ(select->get<slight::u32>(1), 0);
    EXPECT_EQ(select->get<slight::i64>(1), 0);
    EXPECT_FALSE(select->error());
}

TEST_F(TestSlight, test_select_i64_max)
{
    auto select = db->prepare("SELECT slight_int64 FROM test");

    select->step();
    select->step();

    EXPECT_STREQ(select->get<slight::text>(1), "9.22337203685478e+18");
    EXPECT_DOUBLE_EQ(select->get<slight::flt>(1), 9.223372036854776e18);
    EXPECT_EQ(select->get<slight::i32>(1), -1);
    EXPECT_EQ(select->get<slight::u32>(1), 4294967295);
    EXPECT_EQ(select->get<slight::i64>(1), 9.223372036854776e18);
    EXPECT_FALSE(select->error());
}

TEST_F(TestSlight, test_select_u32)
{
    auto select = db->prepare("SELECT name FROM test");

    select->step();
    EXPECT_STREQ(select->get<slight::text>(1), "name1");
    EXPECT_DOUBLE_EQ(select->get<slight::flt>(1), 0.0);
    EXPECT_EQ(select->get<slight::i32>(1), 0);
    EXPECT_EQ(select->get<slight::u32>(1), 0);
    EXPECT_EQ(select->get<slight::i64>(1), 0);
    EXPECT_FALSE(select->error());
}

TEST_F(TestSlight, test_select_text)
{
    auto select = db->prepare("SELECT name FROM test");

    select->step();
    EXPECT_STREQ(select->get<slight::text>(1), "name1");
    EXPECT_DOUBLE_EQ(select->get<slight::flt>(1), 0.0);
    EXPECT_EQ(select->get<slight::i32>(1), 0);
    EXPECT_EQ(select->get<slight::u32>(1), 0);
    EXPECT_EQ(select->get<slight::i64>(1), 0);
    EXPECT_FALSE(select->error());
}

TEST_F(TestSlight, test_select_flt)
{
    auto select = db->prepare("SELECT name FROM test");

    select->step();
    EXPECT_STREQ(select->get<slight::text>(1), "name1");
    EXPECT_DOUBLE_EQ(select->get<slight::flt>(1), 0.0);
    EXPECT_EQ(select->get<slight::i32>(1), 0);
    EXPECT_EQ(select->get<slight::u32>(1), 0);
    EXPECT_EQ(select->get<slight::i64>(1), 0);
    EXPECT_FALSE(select->error());
}

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
    EXPECT_TRUE(set_version->done());

    auto get_version = db->get_schema_version();
    EXPECT_TRUE(get_version->has_row());
    EXPECT_EQ(get_version->get<slight::i32>(1), 1);
    EXPECT_EQ(get_version->get<slight::i64>(1), 1);
    EXPECT_EQ(get_version->get<slight::u32>(1), 1);

    get_version->step();
    EXPECT_FALSE(set_version->has_row());
    EXPECT_TRUE(set_version->done());
    EXPECT_EQ(get_version->get<slight::i32>(1), 0);
}

TEST_F(TestSlight, set_schema_version_max)
{
    auto set_version = db->set_schema_version(std::numeric_limits<std::uint32_t>::max());
    EXPECT_TRUE(set_version->done());

    auto get_version = db->get_schema_version();
    EXPECT_TRUE(get_version->has_row());
    EXPECT_EQ(get_version->get<slight::i32>(1), std::numeric_limits<std::uint32_t>::max());
    EXPECT_EQ(get_version->get<slight::i64>(1), -1);
    EXPECT_EQ(get_version->get<slight::u32>(1), std::numeric_limits<std::uint32_t>::max());

    get_version->step();
    EXPECT_FALSE(set_version->has_row());
    EXPECT_TRUE(set_version->done());
    EXPECT_EQ(get_version->get<slight::i32>(1), 0);
}

TEST_F(TestSlight, bind)
{
    // expect:
    // -
}
