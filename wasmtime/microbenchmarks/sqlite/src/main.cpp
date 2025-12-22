#include <iostream>
#include <string>
#include <sqlite3.h>
#include <chrono>

void check_sqlite_result(const int result, const std::string &message) {
    if (result != SQLITE_OK) {
        std::cerr << "SQLite error (" << result << "): " << message << "\n";
        exit(1);
    }
}

void create_schema(sqlite3 *db, const int num_columns, const std::string &column_type) {
    char *err_msg = nullptr;
    const std::string drop_table_sql = "DROP TABLE benchmark_table;";
    sqlite3_exec(db, drop_table_sql.c_str(), nullptr, nullptr, &err_msg);

    std::string create_table_sql = "CREATE TABLE benchmark_table (id INTEGER PRIMARY KEY";

    for (int i = 1; i <= num_columns; ++i) {
        create_table_sql += ", col" + std::to_string(i) + " " + column_type;
    }
    create_table_sql += ");";


    sqlite3_exec(db, create_table_sql.c_str(), nullptr, nullptr, &err_msg);
}

double insert_rows(sqlite3 *db, const int num_columns, const int num_rows, const bool transaction = false) {
    std::string insert_sql = "INSERT INTO benchmark_table (";

    for (int i = 1; i <= num_columns; ++i) {
        insert_sql += "col" + std::to_string(i);
        if (i != num_columns) insert_sql += ", ";
    }
    insert_sql += ") VALUES (";
    for (int i = 1; i <= num_columns; ++i) {
        insert_sql += std::to_string(rand());
        if (i != num_columns) insert_sql += ", ";
    }
    insert_sql += ");";

    sqlite3_stmt *stmt;
    check_sqlite_result(sqlite3_prepare_v2(db, insert_sql.c_str(), -1, &stmt, nullptr), "Failed to prepare statement");

    auto const start = std::chrono::high_resolution_clock::now();

    if (transaction) {
        check_sqlite_result(sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr),
                            "Failed to begin transaction");
    }

    for (int i = 0; i < num_rows; ++i) {
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    if (transaction) {
        check_sqlite_result(sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr), "Failed to commit transaction");
    }

    sqlite3_finalize(stmt);

    auto const end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> duration = end - start;
    return duration.count();
}

double select_rows(sqlite3 *db, const int num_columns, const int num_selections) {
    const std::string select_sql = "SELECT * FROM benchmark_table ORDER BY RANDOM() LIMIT 1;";

    sqlite3_stmt *stmt;
    check_sqlite_result(sqlite3_prepare_v2(db, select_sql.c_str(), -1, &stmt, nullptr),
                        "Failed to prepare select statement");

    auto const start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_selections; ++i) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            for (int j = 0; j < num_columns; ++j) {
                sqlite3_column_int(stmt, j + 1);
            }
        }
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);

    auto const end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> duration = end - start;
    return duration.count();
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <db_file> <num_columns> <column_type> <num_rows> <num_selections>\n";
        return EXIT_FAILURE;
    }

    const std::string db_file = argv[1];
    const int num_columns = std::stoi(argv[2]);
    const std::string column_type = argv[3];
    const int num_rows = std::stoi(argv[4]);
    const int num_selections = std::stoi(argv[5]);

    std::cout << "---------------[SQLite microbenchmark]---------------\n";
    std::cout << "[Database file]: " << db_file << "\n";
    std::cout << "[Number of columns]: " << num_columns << "\n";
    std::cout << "[Column type]: " << column_type << "\n";
    std::cout << "[Number of rows (inserts)]: " << num_rows << "\n";
    std::cout << "[Number of selections]: " << num_selections << "\n";
    std::cout << "----------------------------------------------------\n";

    sqlite3 *db;
    check_sqlite_result(sqlite3_open(db_file.c_str(), &db), "Cannot open database");

    create_schema(db, num_columns, column_type);

    std::cout << "Performing INSERT microbenchmark...\n";
    const double insert_duration = insert_rows(db, num_columns, num_rows);
    std::cout << "Insertions completed in: " << insert_duration / num_rows << " ms\n";

    std::cout << "Performing SELECT microbenchmark...\n";
    const double select_duration = select_rows(db, num_columns, num_selections);
    std::cout << "Selections completed in: " << select_duration / num_selections << " ms\n";

    sqlite3_close(db);

    return EXIT_SUCCESS;
}
