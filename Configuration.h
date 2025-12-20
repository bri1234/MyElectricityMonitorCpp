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

#include "Json.h"

/// @brief The program configuration.
class Configuration
{
public:

    /// @brief Constructor.
    Configuration();

    /// @brief Loads the configuration from file.
    /// @param configurationFilename The filename of the configuration file.
    void Load(const std::string & configurationFilename);

    /// @brief Returns the database filepath.
    /// @return The database filepath.
    const std::string & GetDatabaseFilepath() const { return _databaseFilepath; }
    
    /// @brief Returns the data acquisition period in seconds.
    /// @return The data acquisition period in seconds.
    double GetDataAcquisitionPeriod() const { return _dataAcquisitionPeriod; }

    /// @brief Returns the inverter serial number.
    /// @return The inverter serial number.
    const std::string & GetInverterSerialNumber() const { return _inverterSerialNumber; }

    /// @brief Returns the number of channels of the inverter.
    /// @return The number of channels of the inverter.
    int GetInverterNumberOfChannels() const { return _inverterNumberOfChannels; }

private:
    std::string _inverterSerialNumber;
    int _inverterNumberOfChannels;

    std::string _databaseFilepath;

    double _dataAcquisitionPeriod;

    static std::string GetStringValue(const Json & json, const std::string & topic, const std::string & key);
    static std::string GetStringValue(const Json & json, const std::string & topic, const std::string & key, const std::string & defaultValue);

    static double GetDoubleValue(const Json & json, const std::string & topic, const std::string & key);
    static double GetDoubleValue(const Json & json, const std::string & topic, const std::string & key, double defaultValue);
    
    static int GetIntValue(const Json & json, const std::string & topic, const std::string & key);
    static int GetIntValue(const Json & json, const std::string & topic, const std::string & key, int defaultValue);
    
};

