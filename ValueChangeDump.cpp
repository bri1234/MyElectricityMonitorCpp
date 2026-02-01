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

#include "ValueChangeDump.h"

using namespace std;
using namespace std::chrono;

ValueChangeDump::ValueChangeDump()
{
}

ValueChangeDump::~ValueChangeDump()
{
    CloseFile();
}

unsigned int ValueChangeDump::DefineVariable(const std::string & name, const std::string & typeStr,
    unsigned int bitWidth, unsigned int initialValue)
{
    if (bitWidth > 31)
        throw Error("DefineVariable: bit width greater than 31 is not supported.");

    if (name.empty())
        throw Error("DefineVariable: variable name cannot be empty.");

    if (typeStr.empty())
        throw Error("DefineVariable: variable type string cannot be empty.");

    unsigned int variableIndex = (unsigned int)_variableList.size();
    string shortVariableName = CreateShortVariableName(variableIndex);

    Variable var {name, shortVariableName, typeStr, bitWidth, initialValue};

    _variableList.push_back(var);

    return variableIndex;
}

void ValueChangeDump::ClearVariables()
{
    _variableList.clear();
}

std::string ValueChangeDump::CreateShortVariableName(unsigned int variableIndex)
{
    static const string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned int base = (unsigned int)chars.size();
    string name;

    do
    {
        name += chars.at(variableIndex % base);
        variableIndex /= base;
    }
    while (variableIndex > 0);

    return name;
}

void ValueChangeDump::CloseFile()
{
    if (_outputFile.is_open())
    {
        _outputFile.close();
    }
}

void ValueChangeDump::OpenFile(const std::string & fileName,
    const std::string & versionStr, const std::string & comment, const std::string & dateStr)
{
    if (_outputFile.is_open())
        throw Error("File is already open.");

    _outputFile.open(fileName);
    if (!_outputFile.is_open())
        throw Error("Cannot open file " + fileName);

    WriteFileHeader("1us", versionStr, comment, dateStr);

    _stopwatchStartTime = high_resolution_clock::now();
}

void ValueChangeDump::WriteFileHeader(const std::string & timescaleStr,
    const std::string & versionStr, const std::string & comment, const std::string & dateStr)
{
    _outputFile << "$date" << endl << "   ";

    if (!dateStr.empty())
        _outputFile << dateStr;
    else
        _outputFile << system_clock::now();

    _outputFile << endl << "$end" << endl;

    _outputFile << "$version" << endl;
    _outputFile << "   " << versionStr << endl;
    _outputFile << "$end" << endl;

    _outputFile << "$comment" << endl;
    _outputFile << "   " << comment << endl;
    _outputFile << "$end" << endl;
    
    _outputFile << "$timescale " << timescaleStr << " $end" << endl;

    for (const auto & variable : _variableList)
    {
        variable.WriteVariableDefinition(_outputFile);
    }

    _outputFile << "$enddefinitions $end" << endl;

    _outputFile << "$dumpvars" << endl;
    for (const auto & variable : _variableList)
    {
        variable.WriteVariableValue(_outputFile);
    }
    _outputFile << "$end" << endl;
}

void ValueChangeDump::StartTheStopwatch()
{
    _stopwatchStartTime = high_resolution_clock::now();
}

void ValueChangeDump::LogVariableValue(unsigned int variableIndex, unsigned int value)
{
    if (variableIndex >= _variableList.size())
        throw Error("LogVariableValue: invalid variable index " + to_string(variableIndex));

    auto timestamp = duration_cast<microseconds>(high_resolution_clock::now() - _stopwatchStartTime).count();
    _outputFile << "#" << timestamp << endl;

    Variable & var = _variableList.at(variableIndex);

    if (value == var.CurrentValue)
        return;

    var.CurrentValue = value;
    var.WriteVariableValue(_outputFile);
}

void ValueChangeDump::LogVariableValue(const std::vector <unsigned int> & variableIndices,
    const std::vector <unsigned int> & values)
{
    if (variableIndices.size() != values.size())
        throw Error("LogVariableValue: variableIndices size does not match values size.");

    auto timestamp = duration_cast<microseconds>(high_resolution_clock::now() - _stopwatchStartTime).count();
    _outputFile << "#" << timestamp << endl;

    for (size_t i = 0; i < variableIndices.size(); i++)
    {
        unsigned int variableIndex = variableIndices.at(i);
        unsigned int value = values.at(i);

        if (variableIndex >= _variableList.size())
            throw Error("LogVariableValue: invalid variable index " + to_string(variableIndex));

        Variable & var = _variableList.at(variableIndex);

        if (value == var.CurrentValue)
            return;

        var.CurrentValue = value;
        var.WriteVariableValue(_outputFile);
    }
}

void ValueChangeDump::Variable::WriteVariableDefinition(std::ofstream & os) const
{
    os << "$var " << TypeStr << " " << BitWidth << " " << ShortName << " " << Name << " $end" << endl;
}

void ValueChangeDump::Variable::WriteVariableValue(std::ofstream & os) const
{
    if (BitWidth == 1)
    {
        // write single bit value
        os << (CurrentValue ? '1' : '0') << ShortName << endl;
    }
    else
    {
        // write binary value
        os << "b";
        for (int bitIndex = BitWidth - 1; bitIndex >= 0; bitIndex--)
        {
            os << ((CurrentValue & (1 << bitIndex)) ? '1' : '0');
        }
        os << " " << ShortName << endl;
    }
}

