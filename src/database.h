#ifndef _DATABASE_H
#define _DATABASE_H

#include <sqlite3.h>
#include <memory>
#include <iostream>
#include <fstream>
#include <vector>

namespace CipherSafe
{
  class Database
  {
  public:
    struct Entry
    {
      int id;
      std::string title, url, username, password, category, notes;
    };

    Database(const std::string& path);
    bool Add(std::unique_ptr<Database::Entry> entry);
    bool Update(Database::Entry* entry);
    bool ResetDB();
    bool RemoveEntryById(int id);
    int Close();
    std::vector<std::unique_ptr<Database::Entry>> GetAll();
    std::vector<std::unique_ptr<Database::Entry>> Filter(const std::string& query);
    std::unique_ptr<Database::Entry> GetEntryById(int id);

  private:
    const std::string path;
    sqlite3* db;
    int create_tables();
    void init_db();
  };
}
#endif
