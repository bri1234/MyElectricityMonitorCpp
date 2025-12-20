#pragma once

/*
Copyright (C) 2025  Torsten Brischalle
email: torsten@brischalle.de
web: http://www.aaabbb.de

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/

#include <sqlite3.h>
#include <string>
#include <stdexcept>
#include <vector>

/// @brief Class to store the readings in a SQLite database.
class Database
{
public:

    /// @brief Database error.
    class DatabaseError : public std::runtime_error
    {
    public:
        DatabaseError(const std::string & errorMessage) : std::runtime_error(errorMessage) { }
    };

    /// @brief Creates a new instance of the database object.
    /// @param fileName The filename of the SQLite database. If the database does not exists a new one will be created.
    /// @param numberOfInverterChannels Number of inverter channels = number of solar panels.
    Database(const std::string & fileName, int numberOfInverterChannels);

    virtual ~Database();

private:

    const std::vector <std::string> _COLUMNS_ELECTRICITY_METER { "+A", "+A T1", "+A T2", "-A", "P", "P L1", "P L2", "P L3" };
    const std::vector <std::string> _READINGS_INVERTER_CHANNEL { "DC V", "DC I", "DC P", "DC E day", "DC E total" };
    const std::vector <std::string> _READINGS_INVERTER { "AC V", "AC I", "AC F", "AC P", "AC Q", "AC PF", "T" };
    std::vector <std::string> _columnsInverter;

    int _numberOfInverterChannels;

    sqlite3 *_database;

    /// @brief Opens the database.
    void OpenDatabase(const std::string fileName);

    /// @brief Closes the database.
    void CloseDatabase();

    /// @brief Creates all missing tables in the database.
    void CreateTablesIfNotExists();

    /// @brief Checks the result code and throws an error if it is an error code.
    /// @param resultCode The result code to be checked.
    void CheckResult(int resultCode);

    /// @brief Executes an SQL command.
    /// @param sql The SQL command to be executed.
    void SqlExecute(const std::string & sql);
};

