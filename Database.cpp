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
#include <ctime>

#include "Database.h"
#include "Utils.h"
#include "Logger.h"

using namespace std;
using namespace Utils;

Database::Database(const std::string & fileName, int numberOfInverterChannels)
: _database(nullptr)
{
    _numberOfInverterChannels = numberOfInverterChannels;

    for (int channel = 0; channel < numberOfInverterChannels; channel++)
    {
        for (auto & reading : _READINGS_INVERTER_CHANNEL)
            _columnsInverter.push_back(format("CH{} {}", channel, reading));
    }

    AppendRange(_columnsInverter, _READINGS_INVERTER);

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
        LOG_ERROR(e);
    }
}

void Database::OpenDatabase(const std::string fileName)
{
    CloseDatabase();

    int resultCode = sqlite3_open(fileName.c_str(), &_database);
    CheckResult(resultCode, "Can not open database");
}

void Database::CloseDatabase()
{
    if (!_database)
        return;

    int resultCode = sqlite3_close(_database);
    CheckResult(resultCode, "Can not close database");

    _database = nullptr;
}

void Database::CheckResult(int resultCode, const std::string & message)
{
    if ((resultCode == SQLITE_OK) || (resultCode == SQLITE_ROW) || (resultCode == SQLITE_DONE))
        return;
    
    throw Error(format("{}: {} (error code {})", message, sqlite3_errstr(resultCode), resultCode));
}

void Database::SqlExecute(const std::string & sql)
{
    char * errMsg = nullptr;
    int resultCode = sqlite3_exec(_database, sql.c_str(), nullptr, nullptr, &errMsg);
    CheckResult(resultCode, "Can not execute SQL command");

    if (errMsg)
    {
        string errorMessage(errMsg);
        sqlite3_free(errMsg);
        throw Error(errorMessage);
    }
}

void Database::CreateTablesIfNotExists()
{
    vector <string> columns;

    // create inverter data table
    for (const auto & column : _columnsInverter)
        columns.push_back(format("\"{}\" REAL", column));

    string columnsStr = Join(columns, ",");

    string sql = format("CREATE TABLE IF NOT EXISTS Inverter (\"time\" INT NOT NULL PRIMARY KEY,{});", columnsStr);
    SqlExecute(sql);

    // create electricity meter 1 data table
    columns.clear();
    for (const auto & column : _COLUMNS_ELECTRICITY_METER)
        columns.push_back(format("\"{}\" REAL", column));

    columnsStr = Join(columns, ",");

    sql = format("CREATE TABLE IF NOT EXISTS ElectricityMeter0 (\"time\" INT NOT NULL PRIMARY KEY,{});", columnsStr);
    SqlExecute(sql);
    
    // create electricity meter 2 data table
    sql = format("CREATE TABLE IF NOT EXISTS ElectricityMeter1 (\"time\" INT NOT NULL PRIMARY KEY,{});", columnsStr);
    SqlExecute(sql);
}

void Database::InsertReadingsElectricityMeter(int electricityMeterNum, const readings_type & readings)
{
    if ((electricityMeterNum < 0) || (electricityMeterNum > 1))
        throw Error(format("Invalid electricity meter number: {}", electricityMeterNum));
    
    ostringstream os;

    os << "INSERT INTO ElectricityMeter";
    os << electricityMeterNum;
    os << " VALUES (" << time(nullptr);

    for (const auto & key : _COLUMNS_ELECTRICITY_METER)
    {
        os << ",";

        auto it = readings.find(key);
        if (it != readings.end())
            os << it->second;
        else
            os << 0.0;
    }

    os << ");";

    SqlExecute(os.str());
}

void Database::InsertReadingsInverter(const readings_type & readings)
{
    ostringstream os;

    os << "INSERT INTO Inverter VALUES (" << time(nullptr);

    for (const auto & key : _columnsInverter)
    {
        os << ",";

        auto it = readings.find(key);
        if (it != readings.end())
            os << it->second;
        else
            os << 0.0;
    }

    os << ");";

    SqlExecute(os.str());
}

