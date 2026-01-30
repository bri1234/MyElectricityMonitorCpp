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

#include <fstream>
#include <stdexcept>
#include <string>
#include <format>
#include <vector>

/// @brief Class for creating value change dump files. (see https://en.wikipedia.org/wiki/Value_change_dump)
///        Use "GTKWave" to view the created value change dump files.
class ValueChangeDump
{
public:

    /// @brief Value change dump file error.
    class Error : public std::runtime_error
    {
    public:
        Error(const std::string & errorMessage) : std::runtime_error(std::format("Value change dump file error: {}", errorMessage)) { }
    };

    /// @brief Creates a new value change dump file.
    ValueChangeDump();
    ~ValueChangeDump();
    
    /// @brief Defines a variable in the value change dump file.
    /// @param name The variable name.
    /// @param typeStr The variable type string. (e.g. wire, reg, ...)
    /// @param bitWidth The variable bit width. (1, 8, ...)
    /// @param initialValue The initial value of the variable.
    /// @return The variable index.
    unsigned int DefineVariable(const std::string & name, const std::string & typeStr,
        unsigned int bitWidth, unsigned int initialValue = 0);

    /// @brief Clears all defined variables.
    void ClearVariables();

    /// @brief Opens a value change dump file.
    /// @param fileName The file name.
    /// @param timescaleStr The timescale string. (e.g. 10us, time number = 1 | 10 | 100, time unit = s | ms | us | ns | ps | fs)
    /// @param dateStr The date string.
    /// @param versionStr The version string.
    /// @param comment Additional comment.
    void OpenFile(const std::string & fileName, const std::string & timescaleStr,
        const std::string & versionStr = "", const std::string & comment = "",
        const std::string & dateStr = "");

    /// @brief Closes the value change dump file.
    void CloseFile();
    
    /// @brief Begins a log entry with the given timestamp. Call this function at every time step before logging variable values.
    /// @param timestamp The timestamp in time units specified by the timescale.
    void BeginLog(unsigned int timestamp);

    /// @brief Logs the value of a variable.
    /// @param variableIndex The variable index.
    /// @param value The variable value.
    void LogVariableValue(unsigned int variableIndex, unsigned int value);

private:
    class Variable
    {
    public:
        typedef std::vector<Variable> list_type;

        std::string Name;
        std::string ShortName;
        std::string TypeStr;
        unsigned int BitWidth = 0;
        unsigned int CurrentValue = 0;

        /// @brief Writes the variable definition to the output stream.
        /// @param os The output stream.
        void WriteVariableDefinition(std::ofstream & os) const;

        /// @brief Writes the variable value to the output stream.
        /// @param os The output stream.
        void WriteVariableValue(std::ofstream & os) const;
    };

    std::ofstream _outputFile;

    Variable::list_type _variableList;
    
    bool _newLogEntry = false;
    unsigned int _currentLogTimestamp = 0;

    /// @brief Writes the file header.
    /// @param timescaleStr The timescale string. (e.g. 1s, 10ms, ...)
    /// @param dateStr The date string.
    /// @param versionStr The version string.
    /// @param comment Additional comment.
    void WriteFileHeader(const std::string & timescaleStr, const std::string & versionStr,
        const std::string & comment, const std::string & dateStr = "");

    /// @brief Creates a short variable name from the variable index that consists of letters a-z and A-Z.
    static std::string CreateShortVariableName(unsigned int variableIndex);
};