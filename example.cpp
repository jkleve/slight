#include "slight.h"
#include <iostream>

int main()
{
    //
    // user input
    //
    uint32_t id = 0;
    uint32_t age = 0;
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
        "name VARCHAR(200),"
        "age INTEGER,"
        "notes TEXT)");
    auto create_other_table_stmt = db.prepare(
        "CREATE TABLE IF NOT EXISTS my_other_table "
        "(id PRIMARY KEY, name VARCHAR(200))");
    auto insert_stmt = db.prepare(
        "INSERT INTO my_table_name VALUES (?, ?, ?, ?)",
        {Bind(id), Bind(name.c_str()), Bind(age), Bind(notes.c_str())}
    );
    auto more_types = db.prepare(
            "INSERT INTO my_table_name VALUES (:id, ?, :id, ?)",
            Bind(":id", 83));
    auto select_stmt = db.prepare("SELECT * FROM my_table_name");
    auto select_one_stmt = db.prepare("SELECT name FROM my_table_name");
    auto err = db.prepare("slight MALFORMED query");

    if (did_error(*create_my_table_stmt))
    {
        std::cout << "Create table error " << create.error_code() << ": " << create.error_msg() << std::endl;
        return -1;
    }

    auto insert = db.run(
        Q("INSERT INTO my_table_name VALUES (?, ?, ?, ?)",
          {Bind(id), Bind(name.c_str()), Bind(age), Bind(notes.c_str())})
    );

    if (insert.error())
    {
        std::cout << "Insert error " << insert.error_code() <<  ": " << insert.error_msg() << std::endl;
        return -1;
    }

    std::cout << "Contents are:" << std::endl;
    while (slight::is_ready(*select_stmt))
    {
        std::string name_value = "null";
        std::string notes_value = "null";
        if (slight::get<slight::text>(*select_stmt, 2))
            name_value = slight::get<slight::text>(*select_stmt, 2);
        if (slight::get<slight::text>(*select_stmt, 4))
            notes_value = slight::get<slight::text>(*select_stmt, 4);
        std::cout << "User: " << name_value <<  " (" << slight::get<slight::i32>(*select_stmt, 1) << "), age: " <<
            slight::get<slight::i32>(*select_stmt, 3) << ", notes: " << notes_value << std::endl;
        slight::step(*select_stmt);
    }

    while (slight::is_ready(*select_one_stmt))
    {
        slight::get<slight::text>(*select_stmt, 1);
        slight::step(*select_stmt);
    }

    if (values.error())
    {
        std::cout << "Select error " << values.error_code() << ": " << values.error_msg() << std::endl;
        return -1;
    }

    std::cout << slight::error_code(*err) << ": " << slight::error_msg(*err);

    auto more_types = db.run(
        Q("INSERT INTO my_table_name VALUES (:id, ?, :id, ?)",
            Bind(":id", 83)));

    return 0;
}
