#include "database.hpp"
#include "weather.hpp"

#include <iostream>
#include <cstdlib>

Database::Database(const char* filename)
    : db_(nullptr)
{
    int rc = sqlite3_open(filename, &db_);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Datenbank konnte nicht geöffnet werden: "
                  << sqlite3_errmsg(db_) << '\n';
    }
    else
    {
        std::cout << "Datenbank geöffnet\n";
    }

    if (!createTables())
    {
        std::cerr
            << "Datenbanktabellen konnten nicht erstellt werden.\n";

        sqlite3_close(db_);
        std::exit(EXIT_FAILURE);
    }
}

Database::~Database()
{
    if (db_ != nullptr)
    {
        sqlite3_close(db_);
        std::cout << "Datenbank geschlossen\n";
    }
}

bool Database::createTables()
{
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS weather (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT UNIQUE NOT NULL,
            temperature REAL NOT NULL,
            humidity REAL NOT NULL,
            pressure REAL NOT NULL
        );

        CREATE TABLE IF NOT EXISTS sync_checked (
            minute TEXT PRIMARY KEY
        );
    )";

    char* errorMessage = nullptr;

    int rc = sqlite3_exec(
        db_,
        sql,
        nullptr,
        nullptr,
        &errorMessage
    );

    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL Fehler: "
                  << errorMessage << '\n';

        sqlite3_free(errorMessage);
        return false;
    }

    std::cout << "Tabellen weather und sync_checked bereit\n";

    return true;
}

bool Database::insertWeatherData(const WeatherData& weather)
{
    const char* sql = R"(
        INSERT OR IGNORE INTO weather
        (timestamp, temperature, humidity, pressure)
        VALUES (?, ?, ?, ?);
    )";

    sqlite3_stmt* statement = nullptr;

    int rc = sqlite3_prepare_v2(
        db_,
        sql,
        -1,
        &statement,
        nullptr
    );
    
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL Fehler beim Vorbereiten:"
                  << sqlite3_errmsg(db_) << '\n';

        return false;
    }

    sqlite3_bind_text(statement, 1, weather.timestamp().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(statement, 2, weather.temperature());
    sqlite3_bind_double(statement, 3, weather.humidity());
    sqlite3_bind_double(statement, 4, weather.pressure());

    rc = sqlite3_step(statement);

    if (rc != SQLITE_DONE)
    {
        std::cerr << "SQL Fehler beim Einfügen: "
                 << sqlite3_errmsg(db_) << '\n';

        sqlite3_finalize(statement);

        return false;
    }

    bool inserted = (sqlite3_changes(db_) > 0);

    sqlite3_finalize(statement);

    if (inserted)
    {
        std::cout << "Wetterdaten gespeichert\n";
    }
    else
    {
        std::cout
            << "Wetterdaten bereits vorhanden - "
               "Einfügen uebersprungen.\n";
    }

    std::cout << "insertWeatherData: return true\n";

    return true;
}

SyncInfo Database::getSyncInfo()
{
    const char* sql = R"(
        SELECT MIN(timestamp), MAX(timestamp)
        FROM weather
        WHERE timestamp != '0000-00-00 00:00:00';
    )";

    sqlite3_stmt* statement = nullptr;

    int rc = sqlite3_prepare_v2(
        db_,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL Fehler beim Vorbereiten: "
                  << sqlite3_errmsg(db_) << '\n';

        return {};
    }

    SyncInfo info;

    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        const unsigned char* first = sqlite3_column_text(statement, 0);
        const unsigned char* last = sqlite3_column_text(statement, 1);

        if (first != nullptr)
        {
            info.first = reinterpret_cast<const char*>(first);
        }

        if (last != nullptr)
        {
            info.last = reinterpret_cast<const char*>(last);
        }
    }

    sqlite3_finalize(statement);

    return info;
}

std::vector<std::string> Database::getTimestamps(
    const std::string& first,
    const std::string& last)
{
    std::vector<std::string> timestamps;

    const char* sql = R"(
        SELECT timestamp
        FROM weather
        WHERE timestamp >= ?
          AND timestamp <= ?
        ORDER BY timestamp;
    )";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        db_,
        sql,
        -1,
        &stmt,
        nullptr
    );

    if (rc != SQLITE_OK)
    {
        std::cerr
            << "SQL prepare Fehler: "
            << sqlite3_errmsg(db_)
            << '\n';

        return timestamps;
    }

    sqlite3_bind_text(
        stmt,
        1,
        first.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        stmt,
        2,
        last.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const unsigned char* text =
            sqlite3_column_text(stmt, 0);

        if (text != nullptr)
        {
            timestamps.push_back(
                reinterpret_cast<const char*>(text)
            );
        }
    }

    sqlite3_finalize(stmt);

    return timestamps;
}

bool Database::isMinuteChecked(const std::string& minute)
{
    const char* sql = R"(
        SELECT 1
        FROM sync_checked
        WHERE minute = ?
        LIMIT 1;
    )";

    sqlite3_stmt* statement = nullptr;

    int rc = sqlite3_prepare_v2(
        db_,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (rc != SQLITE_OK)
    {
        std::cerr
            << "SQL Fehler beim Vorbereiten: "
            << sqlite3_errmsg(db_)
            << '\n';

        return false;
    }

    sqlite3_bind_text(
        statement,
        1,
        minute.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    rc = sqlite3_step(statement);

    bool checked = (rc == SQLITE_ROW);

    sqlite3_finalize(statement);

    return checked;
}

bool Database::markMinuteChecked(const std::string& minute)
{
    const char* sql = R"(
        INSERT OR IGNORE INTO sync_checked (minute)
        VALUES (?);
    )";

    sqlite3_stmt* statement = nullptr;

    int rc = sqlite3_prepare_v2(
        db_,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (rc != SQLITE_OK)
    {
        std::cerr
            << "SQL Fehler beim Vorbereiten: "
            << sqlite3_errmsg(db_)
            << '\n';

        return false;
    }

    sqlite3_bind_text(
        statement,
        1,
        minute.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    rc = sqlite3_step(statement);

    if (rc != SQLITE_DONE)
    {
        std::cerr
            << "SQL Fehler beim Markieren der Minute: "
            << sqlite3_errmsg(db_)
            << '\n';

        sqlite3_finalize(statement);
        return false;
    }

    sqlite3_finalize(statement);

    return true;
}
