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
        "INSERT INTO my_table_name VALUES (?, ?, ?, ?)"
        //{Bind(id), Bind(name.c_str()), Bind(age), Bind(notes.c_str())}
    );
    auto more_types = db.prepare(
            "INSERT INTO my_table_name VALUES (:id, ?, :id, ?)");
            //Bind(":id", 83));
    auto select_stmt = db.prepare("SELECT * FROM my_table_name");
    auto select_name_stmt = db.prepare("SELECT name FROM my_table_name");
    auto err = db.prepare("slight MALFORMED query");

    slight::step(*create_my_table_stmt);
    assert(!did_error(*create_my_table_stmt));

    slight::step(*create_other_table_stmt);
    assert(!did_error(*create_other_table_stmt));

    if (slight::is_ready(*insert_stmt))
    {
        slight::bind(*insert_stmt, {Bind(id), Bind(name.c_str()), Bind(age), Bind(notes.c_str())});
        slight::step(*insert_stmt);
    }
    else
    {
        std::cout << slight::error_msg(*insert_stmt) << std::endl;
    }
    assert(!slight::did_error(*insert_stmt));

    std::cout << "Contents are:" << std::endl;
    slight::step(*select_stmt);
    while (slight::has_row(*select_stmt))
    {
        auto user_id = slight::get<slight::i32>(*select_stmt, 1);
        auto user_name = slight::get<slight::text>(*select_stmt, 2);
        auto user_age = slight::get<slight::i32>(*select_stmt, 3);
        auto user_notes = slight::get<slight::text>(*select_stmt, 4);

        std::cout <<
            "User: id (" << user_id << "), " <<
            "name: " << user_name << ", " <<
            "age: " << user_age << ", " <<
            "notes: " << user_notes << std::endl;
        slight::step(*select_stmt);
    }

    std::cout << "All names:" << std::endl;
    slight::step(*select_name_stmt);
    while (slight::has_row(*select_name_stmt))
    {
        std::cout << slight::get<slight::text>(*select_name_stmt, 1) << std::endl;
        slight::step(*select_name_stmt);
    }

    assert(!slight::did_error(*select_name_stmt));
    slight::reset(*select_name_stmt);

    std::cout << "All names again:" << std::endl;
    slight::step(*select_name_stmt);
    while (slight::has_row(*select_name_stmt))
    {
        std::cout << slight::get<slight::text>(*select_name_stmt, 1) << std::endl;
        slight::step(*select_name_stmt);
    }

    assert(!slight::did_error(*select_name_stmt));

    slight::step(*err);
    std::cout << slight::error_code(*err) << ": " << slight::error_msg(*err);

    slight::step(*more_types);
    assert(!slight::did_error(*more_types));

    return 0;
}
