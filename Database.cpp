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

#include <format>

#include "Database.h"
#include "Utils.h"

using namespace std;
using namespace Utils;

Database::Database(const std::string & fileName, int numberOfInverterChannels)
: _database(nullptr)
{
    _numberOfInverterChannels = numberOfInverterChannels;

    for (int channel = 0; channel < numberOfInverterChannels; channel++)
    {
        for (auto & reading : _READINGS_INVERTER_CHANNEL)
        {
            _columnsInverter.push_back(format("CH{} {}", channel, reading));
        }
    }

    append_range(_columnsInverter, _READINGS_INVERTER);

    OpenDatabase(fileName);
    CreateTablesIfNotExists();
}

Database::~Database()
{
    try
    {
        CloseDatabase();
    }
    catch(const exception & e)
    {
        // TODO: log in file
        // std::cerr << e.what() << '\n';
    }
}

void Database::OpenDatabase(const std::string fileName)
{
    CloseDatabase();

    CheckResult(sqlite3_open(fileName.c_str(), &_database));
}

void Database::CloseDatabase()
{
    if (!_database)
        return;

    CheckResult(sqlite3_close(_database));
    _database = nullptr;
}

void Database::CheckResult(int resultCode)
{
    if ((resultCode == SQLITE_OK) || (resultCode == SQLITE_ROW) || (resultCode == SQLITE_DONE))
        return;
    
    throw DatabaseError(format("{} (error code {})", sqlite3_errstr(resultCode), resultCode));
}

void Database::CreateTablesIfNotExists()
{

}
