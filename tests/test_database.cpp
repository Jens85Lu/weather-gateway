#include "database.hpp"

#include <iostream>
#include <string>
#include <sqlite3.h>
#include <cstdio>

int main()
{
    const char* filename = "test_database.db";
    std::remove(filename);

    {
        Database database(filename);

        // Test 1: getLastSync() bei leerer Tabelle
        std::cout << "Test 1: getLastSync() bei leerer Tabelle\n";

        std::string actual = database.getLastSync();
        std::string expected = "";

        if (actual == expected)
        {
            std::cout << "[PASS] Leere Tabelle liefert leeren String\n";
        }
        else
        {
            std::cerr << "[FAIL] Erwartet: '" << expected
                      << "', erhalten: '" << actual << "'\n";
            return 1;
        }

        // Test 2: Zeitpunkt speichern und auslesen
        std::cout << "\nTest 2: markLastSync() und getLastSync()\n";

        expected = "2026-10-12 18:30";

        if (!database.markLastSync(expected))
        {
            std::cerr << "[FAIL] markLastSync() fehlgeschlagen\n";
            return 1;
        }

        actual = database.getLastSync();

        if (actual == expected)
        {
            std::cout << "[PASS] Synchronisationszeitpunkt korrekt gespeichert\n";
        }
        else
        {
            std::cerr << "[FAIL] Erwartet: '" << expected
                      << "', erhalten: '" << actual << "'\n";
            return 1;
        }

        // Test 3: Derselbe Zeitpunkt darf kein Duplikat erzeugen
        std::cout << "\nTest 3: Doppeltes Einfügen\n";

        if (!database.markLastSync(expected))
        {
            std::cerr << "[FAIL] Zweiter Aufruf von markLastSync() fehlgeschlagen\n";
            return 1;
        }

        std::cout << "[PASS] Zweiter Aufruf erfolgreich\n";
    }

    // Direkte Kontrolle der Tabellenzeilen mit SQLite
    sqlite3* db = nullptr;

    if (sqlite3_open(filename, &db) != SQLITE_OK)
    {
        std::cerr << "[FAIL] Testdatenbank konnte nicht geöffnet werden\n";
        if (db != nullptr)
        {
            sqlite3_close(db);
        }
        return 1;
    }

    sqlite3_stmt* statement = nullptr;

    const char* sql = "SELECT COUNT(*) FROM lastSync;";

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        std::cerr << "[FAIL] SQL-Abfrage fehlgeschlagen: "
                  << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return 1;
    }

    int rc = sqlite3_step(statement);

    if (rc != SQLITE_ROW)
    {
        std::cerr << "[FAIL] Anzahl der Einträge konnte nicht gelesen werden\n";
        sqlite3_finalize(statement);
        sqlite3_close(db);
        return 1;
    }

    int count = sqlite3_column_int(statement, 0);

    sqlite3_finalize(statement);
    sqlite3_close(db);
    std::remove(filename);

    if (count == 1)
    {
        std::cout << "[PASS] lastSync enthält genau einen Eintrag\n";
    }
    else
    {
        std::cerr << "[FAIL] Erwartet: 1 Eintrag, erhalten: "
                  << count << '\n';
        return 1;
    }

    std::cout << "\nAlle Tests bestanden.\n";
    return 0;
}