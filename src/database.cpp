#include "database.h"

using namespace CipherSafe;

Database::Database(const std::string& path): path(path) {
  init_db();
  create_tables();
}

bool Database::Add(std::unique_ptr<Database::Entry> entry) {
    const char* insert_sql = "INSERT INTO secrets (title, url, username, password, category, notes) VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(this->db, insert_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout << "Failed to prepare statement: " << sqlite3_errmsg(this->db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, entry->title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, entry->url.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, entry->username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, entry->password.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, entry->category.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, entry->notes.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cout << "Execution failed: " << sqlite3_errmsg(this->db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}


bool Database::Update(Database::Entry* entry) {
    const char* update_sql = "UPDATE secrets SET title = ?, url = ?, username = ?, password = ?, category = ?, notes = ? WHERE id = ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(this->db, update_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout << "Failed to prepare statement: " << sqlite3_errmsg(this->db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, entry->title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, entry->url.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, entry->username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, entry->password.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, entry->category.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, entry->notes.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, entry->id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cout << "Execution failed: " << sqlite3_errmsg(this->db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

void Database::init_db() {
  sqlite3* database;
  int exit_status;
  exit_status = sqlite3_open(this->path.c_str(), &database);

  if (exit_status) {
    throw std::runtime_error("An error occured while attempting to open the database:" + std::string(sqlite3_errmsg(database)));
  } else {
    this->db = database;
  }
}

int Database::create_tables() {
    char* db_error_msg = nullptr;
    std::string create_sql = "CREATE TABLE IF NOT EXISTS secrets (id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT, url TEXT, username TEXT, password TEXT, category TEXT, notes TEXT);";

    int exit_status = sqlite3_exec(this->db, create_sql.c_str(), 0, 0, &db_error_msg);

    if (exit_status != SQLITE_OK) {
        std::cout << "Error creating table: " << sqlite3_errmsg(this->db) << std::endl;
        sqlite3_free(db_error_msg);
    }

    return exit_status;
}


int Database::Close() {
    int rc; 

    if (this->db != nullptr) {
        rc = sqlite3_close(this->db);
    }

    if (rc != SQLITE_OK) {
        std::cerr << "Error closing database: " << sqlite3_errmsg(this->db) << std::endl;
        return rc;
    }

    return rc;
}

std::vector<std::unique_ptr<Database::Entry>> Database::GetAll() {
    std::vector<std::unique_ptr<Database::Entry>> entries;
    sqlite3_stmt *stmt = nullptr;
    std::string sql = "SELECT * FROM secrets;";

    int rc = sqlite3_prepare_v2(this->db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Database error: " + std::string(sqlite3_errmsg(this->db)));
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::unique_ptr<Database::Entry> entry(new Database::Entry());

        entry->id       = sqlite3_column_int(stmt, 0);
        entry->title    = (const char*)sqlite3_column_text(stmt, 1);
        entry->url      = (const char*)sqlite3_column_text(stmt, 2);
        entry->username = (const char*)sqlite3_column_text(stmt, 3);
        entry->password = (const char*)sqlite3_column_text(stmt, 4);
        entry->category = (const char*)sqlite3_column_text(stmt, 5);
        entry->notes    = (const char*)sqlite3_column_text(stmt, 6);

        entries.push_back(std::move(entry));
    }

    if (rc != SQLITE_DONE) {
        throw std::runtime_error("Database error: " + std::string(sqlite3_errmsg(this->db)));
    }

    sqlite3_finalize(stmt);
    return entries;
}


std::vector<std::unique_ptr<Database::Entry>> Database::Filter(const std::string& query) {
    std::vector<std::unique_ptr<Database::Entry>> entries;
    sqlite3_stmt *stmt = nullptr;
    std::string sql = "SELECT * FROM secrets WHERE url LIKE ? OR title LIKE ? OR category LIKE ?";
    std::string wildcard_query = "%" + query + "%";

    int rc = sqlite3_prepare_v2(this->db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(this->db)));
    }

    // Bind the query parameter
    rc = sqlite3_bind_text(stmt, 1, wildcard_query.c_str(), -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(stmt);
        throw std::runtime_error("Failed to bind parameter: " + std::string(sqlite3_errmsg(this->db)));
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::unique_ptr<Database::Entry> entry(new Database::Entry());

        entry->id       = sqlite3_column_int(stmt, 0);
        entry->title    = sqlite3_column_text(stmt, 1) ? (const char*)sqlite3_column_text(stmt, 1) : "";
        entry->url      = sqlite3_column_text(stmt, 2) ? (const char*)sqlite3_column_text(stmt, 2) : "";
        entry->username = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        entry->password = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        entry->category = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        entry->notes    = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";

        entries.push_back(std::move(entry));
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error("Execution failed: " + std::string(sqlite3_errmsg(this->db)));
    }

    sqlite3_finalize(stmt);
    return entries;
}


bool Database::ResetDB() {
    std::string sql = "DELETE FROM secrets;";
    char* errMsg = nullptr;

    int rc = sqlite3_exec(this->db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Database error resetting db: " + std::string(sqlite3_errmsg(this->db)) << std::endl;
        return false;
    }

    return true;
}

std::unique_ptr<Database::Entry> Database::GetEntryById(int id) {
    sqlite3_stmt *stmt = nullptr;
    int rc;

    const char *sql = "SELECT id, title, username, password, url, category, notes FROM secrets WHERE id = ?";

    rc = sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << sqlite3_errmsg(this->db) << std::endl;
        return nullptr;
    }

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        int entry_id                  = sqlite3_column_int(stmt, 0);
        const unsigned char *title    = sqlite3_column_text(stmt, 1);
        const unsigned char *username = sqlite3_column_text(stmt, 2);
        const unsigned char *password = sqlite3_column_text(stmt, 3);
        const unsigned char *url      = sqlite3_column_text(stmt, 4);
        const unsigned char *category = sqlite3_column_text(stmt, 5);
        const unsigned char *notes    = sqlite3_column_text(stmt, 6);

        std::unique_ptr<Database::Entry> entry(new Database::Entry{
            entry_id,
            std::string(reinterpret_cast<const char*>(title)),
            std::string(reinterpret_cast<const char*>(url)),
            std::string(reinterpret_cast<const char*>(username)),
            std::string(reinterpret_cast<const char*>(password)),
            std::string(reinterpret_cast<const char*>(category)),
            std::string(reinterpret_cast<const char*>(notes))
            });

        sqlite3_finalize(stmt);
        return entry;
    } else if (rc == SQLITE_DONE) {
        std::cout << "No entry found with ID " << id << std::endl;
    } else {
        std::cerr << "Execution failed: " << sqlite3_errmsg(this->db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return nullptr;
}

bool Database::RemoveEntryById(int id) {
    bool didRemove = false;
    sqlite3_stmt *stmt = nullptr;
    int rc;
    const char *sql = "DELETE FROM secrets WHERE id = ?";

    rc = sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Database error preparing SQL query: " + std::string(sqlite3_errmsg(this->db)));
    }

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error("Error executing SQL statement: " + std::string(sqlite3_errmsg(this->db)));
    } else {
        didRemove = true;
    }

    sqlite3_finalize(stmt);

    return didRemove;
}

