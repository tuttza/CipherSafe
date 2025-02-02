#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "../database.h"
#include <memory>
#include <vector>

TEST_CASE("CipherSafe::Database Close()") { 
    std::unique_ptr<CipherSafe::Database> db(new CipherSafe::Database("./test.db"));

    SUBCASE("ensure the db was closed successfully") {		
		CHECK(db->Close() == 0);
    }
}

TEST_CASE("CipherSafe::Database Add()") {
    std::unique_ptr<CipherSafe::Database> db(new CipherSafe::Database("./test.db"));

    SUBCASE("Adding a valid entry") {
		std::unique_ptr<CipherSafe::Database::Entry> entry(new CipherSafe::Database::Entry());
		entry->title = "test title";
		entry->url = "https://test.com";
		entry->username = "test@test.com";
		entry->password = "easy123";
		entry->category = "test";

		CHECK(db->Add(std::move(entry)) == true);
    }

    SUBCASE("Adding an empty valid entry") {
		std::unique_ptr<CipherSafe::Database::Entry> entry(new CipherSafe::Database::Entry());
		CHECK(db->Add(std::move(entry)) == true);
    }

    SUBCASE("After adding an entry, it should no longer be accessible") {
		std::unique_ptr<CipherSafe::Database::Entry> entry(new CipherSafe::Database::Entry());
		db->Add(std::move(entry));
		CHECK(entry == nullptr);
    }

    db->Close();
}
