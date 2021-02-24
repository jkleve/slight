# slight
[![Build Status](https://travis-ci.org/jkleve/slight.svg?branch=main)](https://travis-ci.org/jkleve/slight)

minimal sqlite3 C++11 wrapper

```c++
#include <slight.h>
#include <iostream>

int main()
{
    using slight::Bind;
    using slight::Database;

    auto db = Database::open_create_read_write("slight.db");

    // create table 'slight'
    auto create_my_table_stmt = db.prepare(
        "CREATE TABLE IF NOT EXISTS slight "
        "(id PRIMARY KEY, name VARCHAR(200) NOT NULL)");
    create_my_table_stmt->step();
    assert(!create_my_table_stmt->error());

    auto insert_stmt = db.prepare(
        "INSERT INTO slight VALUES (?, ?)");
    auto select_stmt = db.prepare("SELECT * FROM slight");

    // insert (1, minimal) into table 'slight'
    insert_stmt->bind({Bind(1), Bind("minimal")});
    insert_stmt->step();

    // did you try to insert a duplicate key?
    if (insert_stmt->error())
    {
        std::cout << insert_stmt->error_detail() << std::endl;
        return -1;
    }

    // print out contents
    select_stmt->for_each([](slight::Statement* stmt) {
        auto key = stmt->get<slight::i32>(1);
        auto name = stmt->get<slight::text>(2);

        std::cout << "(" << key << ", " << name << ")\n";
        return !stmt->error();
    });

    return 0;
}
```