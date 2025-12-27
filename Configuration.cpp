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

#include "Configuration.h"

#include "UnixUtils.h"
#include "Utils.h"
#include "Logger.h"
#include "Json.h"

#include <filesystem>
#include <format>

using namespace Utils;
using namespace std;

Configuration::Configuration()
: _inverterSerialNumber("00000000")
, _inverterNumberOfChannels(2)
, _electricityMeterSerialPort("/dev/ttyAMA0")
, _databaseFilepath("electricity_monitor_readings.db")
, _dataAcquisitionPeriod(30.0)
{
}

void Configuration::Load(const std::string & configurationFilename)
{
    if (!filesystem::exists(configurationFilename))
        throw runtime_error(format("Configuration file not found: {}", configurationFilename));

    Json json;
    json.LoadFromFile(configurationFilename);

    _inverterSerialNumber = GetStringValue(json, "Inverter", "SerialNumber", _inverterSerialNumber);
    _inverterNumberOfChannels = GetIntValue(json, "Inverter", "NumberOfChannels", _inverterNumberOfChannels);

    _databaseFilepath = GetStringValue(json, "Database", "Filepath", _databaseFilepath);
    _dataAcquisitionPeriod = GetDoubleValue(json, "Database", "DataAcquisitionPeriod", _dataAcquisitionPeriod);

    _electricityMeterSerialPort = GetStringValue(json, "ElectricityMeter", "SerialPort", _electricityMeterSerialPort);
    
    LOG_INFO(std::string("Loaded configuration from: ") + configurationFilename);
}

double Configuration::GetDoubleValue(const Json & json, const std::string & topic, const std::string & key)
{
    json_object * objTopic = nullptr;
    json_object * objKey = nullptr;
    auto root = json.GetRootObject();

    if (!json_object_object_get_ex(root, topic.c_str(), &objTopic))
        throw runtime_error("Topic not found in JSON: " + topic);

    if (!json_object_object_get_ex(objTopic, key.c_str(), &objKey))
        throw runtime_error("Key not found in JSON topic '" + topic + "': " + key);

    if (json_object_get_type(objKey) == json_type_int)
        return json_object_get_int(objKey);

    if (json_object_get_type(objKey) == json_type_double)
        return json_object_get_double(objKey);

    throw runtime_error("Key is not a number in JSON topic '" + topic + "': " + key);
}

double Configuration::GetDoubleValue(const Json & json, const std::string & topic, const std::string & key, double defaultValue)
{
    try
    {
        return GetDoubleValue(json, topic, key);
    }
    catch(const exception& e)
    {
        return defaultValue;
    }
}

int Configuration::GetIntValue(const Json & json, const std::string & topic, const std::string & key)
{
    json_object * objTopic = nullptr;
    json_object * objKey = nullptr;
    auto root = json.GetRootObject();

    if (!json_object_object_get_ex(root, topic.c_str(), &objTopic))
        throw runtime_error("Topic not found in JSON: " + topic);

    if (!json_object_object_get_ex(objTopic, key.c_str(), &objKey))
        throw runtime_error("Key not found in JSON topic '" + topic + "': " + key);

    if (json_object_get_type(objKey) == json_type_int)
        return json_object_get_int(objKey);

    throw runtime_error("Key is not a number in JSON topic '" + topic + "': " + key);
}

int Configuration::GetIntValue(const Json & json, const std::string & topic, const std::string & key, int defaultValue)
{
    try
    {
        return GetIntValue(json, topic, key);
    }
    catch(const exception& e)
    {
        return defaultValue;
    }
}

std::string Configuration::GetStringValue(const Json & json, const std::string & topic, const std::string & key)
{
    json_object * objTopic = nullptr;
    json_object * objKey = nullptr;
    auto root = json.GetRootObject();

    if (!json_object_object_get_ex(root, topic.c_str(), &objTopic))
        throw runtime_error("Topic not found in JSON: " + topic);

    if (!json_object_object_get_ex(objTopic, key.c_str(), &objKey))
        throw runtime_error("Key not found in JSON topic '" + topic + "': " + key);

    if (json_object_get_type(objKey) == json_type_string)
        return string(json_object_get_string(objKey));

    throw runtime_error("Key is not a string in JSON topic '" + topic + "': " + key);
}

std::string Configuration::GetStringValue(const Json & json, const std::string & topic, const std::string & key, const std::string & defaultValue)
{
    try
    {
        return GetStringValue(json, topic, key);
    }
    catch(const exception& e)
    {
        return defaultValue;
    }
}

