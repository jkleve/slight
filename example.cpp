#include "slight.h"
#include <iostream>

using slight::Bind;
using slight::Q;

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
    slight::Database db("my.db", slight::Access::kCreateReadWrite);

    auto create = db.run({
        Q("CREATE TABLE IF NOT EXISTS my_table_name ("
          "id PRIMARY KEY,"
          "name VARCHAR(200),"
          "age INTEGER,"
          "notes TEXT)"),
        Q("CREATE TABLE IF NOT EXISTS my_other_table ("
          "id PRIMARY KEY,"
          "name VARCHAR(200))")
    });

    if (create.error())
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
    auto values = db.start(Q("SELECT * FROM my_table_name"));
    while (values.ready())
    {
        std::string name_value = "null";
        std::string notes_value = "null";
        if (values.get<slight::kText>(2))
            name_value = values.get<slight::kText>(2);
        if (values.get<slight::kText>(4))
            notes_value = values.get<slight::kText>(4);
        std::cout << "User: " << name_value <<  " (" << values.get<slight::kInt>(1) << "), age: " <<
            values.get<slight::kInt>(3) << ", notes: " << notes_value << std::endl;
        values.step();
    }

    if (values.error())
    {
        std::cout << "Select error " << values.error_code() << ": " << values.error_msg() << std::endl;
        return -1;
    }

    auto err = db.run(Q("slight MALFORMED query"));
    std::cout << err.error_code() << ": " << err.error_msg();

    auto more_types = db.run(
        Q("INSERT INTO my_table_name VALUES (:id, ?, :id, ?)",
            Bind(":id", 83)));

    return 0;
}
