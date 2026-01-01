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
#include <memory>
#include <stdexcept>

/// @brief A class for logging.
class Logger
{
public:
    typedef std::shared_ptr <Logger> ptr_type;
    typedef std::shared_ptr <std::ofstream> ostream_ptr_type;
    
    /// @brief Logger error.
    class Error : public std::runtime_error
    {
    public:
        Error(const std::string & errorMessage) : std::runtime_error(std::format("Logger error: {}", errorMessage)) { }
    };

    /// @brief Destructor.
    ~Logger();

    /// @brief Returns the logger instance.
    /// @return Pointer to instance.
    static Logger & Instance();

    /// @brief Opens a new log file.
    /// @param fileName The log file name.
    void OpenLogFile(const std::string & fileName);

    /// @brief Closes the log file.
    void CloseLogFile();

    /// @brief Logs an information message.
    /// @param fileName The source code filename.
    /// @param lineNumber The source code line number.
    /// @param message The log message.
    void LogInfo(const std::string & fileName, int lineNumber, const std::string & message);

    /// @brief Logs an error message.
    /// @param fileName The source code filename.
    /// @param lineNumber The source code line number.
    /// @param message The error message.
    void LogError(const std::string & fileName, int lineNumber, const std::string & message);

    /// @brief Logs an error message.
    /// @param fileName The source code filename.
    /// @param lineNumber The source code line number.
    /// @param exc The error message.
    void LogError(const std::string & fileName, int lineNumber, const std::exception & exc);

private:
    static ptr_type _instance;

    ostream_ptr_type _logFile;

    Logger();

    Logger(const Logger &) = delete;
    Logger & operator=(const Logger &) = delete;

    void LogCurrentTime(std::ostream & os);
    void Log(std::ostream & os, const std::string & messageType, const std::string & fileName, int lineNumber, const std::string & message);
};

#define LOG_INFO(MESSAGE) Logger::Instance().LogInfo(__FILE__, __LINE__, MESSAGE)

#define LOG_ERROR(ERROR_MESSAGE) Logger::Instance().LogError(__FILE__, __LINE__, ERROR_MESSAGE)
