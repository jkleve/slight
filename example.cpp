#include <slight.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

int main()
{
    //
    // user input
    //
    std::uint32_t id = 0;
    std::uint32_t age = 0;
    std::string id_str, age_str, name, notes;

    std::cout << "Enter id: ";
    std::getline(std::cin, id_str);

    try
    {
        id = std::stol(id_str);
    }
    catch (const std::exception& e)
    {
        std::cout << "Invalid id. Must be an integer." << std::endl;
    }

    std::cout << "Enter name: ";
    std::getline(std::cin, name);

    std::cout << "Enter age: ";
    std::getline(std::cin, age_str);

    try
    {
        age = std::stol(age_str);
    }
    catch (const std::exception& e)
    {
        std::cout << "Invalid age. Must be an integer." << std::endl;
    }

    std::cout << "Enter notes: ";
    std::getline(std::cin, notes);

    //
    // slight demo
    //
    using slight::Bind;
    using slight::Database;

    auto db = Database::open_create_read_write("my.db");

    auto create_my_table_stmt = db.prepare(
        "CREATE TABLE IF NOT EXISTS my_table_name ("
        "id PRIMARY KEY,"
        "name VARCHAR(200) NOT NULL,"
        "age INTEGER,"
        "notes TEXT NOT NULL)");
    auto create_other_table_stmt = db.prepare(
        "CREATE TABLE IF NOT EXISTS my_other_table "
        "(id PRIMARY KEY, name VARCHAR(200) NOT NULL)");

    create_my_table_stmt->step();
    assert(!create_my_table_stmt->error());

    create_other_table_stmt->step();
    assert(!create_other_table_stmt->error());

    auto insert_stmt = db.prepare(
        "INSERT INTO my_table_name VALUES (?, ?, ?, ?)");
    auto insert_variables = db.prepare(
        "INSERT INTO my_table_name VALUES (:id, ?, :id, ?)");
    auto select_stmt = db.prepare("SELECT * FROM my_table_name");
    auto select_name_stmt = db.prepare("SELECT name FROM my_table_name");
    auto err = db.prepare("slight MALFORMED query");

    assert(insert_stmt->ready());
    insert_stmt->bind({Bind(id), Bind(name.c_str()), Bind(age), Bind(notes.c_str())});
    insert_stmt->step();

    if (insert_stmt->error())
    {
        std::cout << insert_stmt->error_detail() << std::endl;
        return -1;
    }

    std::cout << "Contents are:" << std::endl;
    while (select_stmt->step())
    {
        auto user_id = select_stmt->get<slight::i32>(1);
        auto user_name = select_stmt->get<slight::text>(2);
        auto user_age = select_stmt->get<slight::i32>(3);
        auto user_notes = select_stmt->get<slight::text>(4);

        std::cout <<
                  " User: id (" << user_id << "), " <<
                  "name: " << user_name << ", " <<
                  "age: " << user_age << ", " <<
                  "notes: " << user_notes << std::endl;
    };

    std::cout << "All names:" << std::endl;
    while (select_name_stmt->step())
        std::cout << " " << select_name_stmt->get<slight::text>(1) << std::endl;

    insert_variables->bind({Bind(id, ":id"), Bind(name.c_str()), Bind(notes.c_str())});
    insert_variables->step();

    select_name_stmt->reset();
    std::cout << "All names again:" << std::endl;
    while (select_name_stmt->step())
        std::cout << " " << select_name_stmt->get<slight::text>(1) << std::endl;

    std::cout << "Example error:" << std::endl;
    std::cout << " " << err->error_code() << ": " << err->error_msg();

    return 0;
}
