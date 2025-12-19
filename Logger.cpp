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

#include <iostream>
#include <stdexcept>
#include <chrono>
#include <format>
#include <filesystem>

#include "Logger.h"

using namespace std;

Logger::ptr_type Logger::_instance;

Logger::Logger()
{

}

Logger::~Logger()
{
    try
    {
        CloseLogFile();
    }
    catch(const exception & exc)
    {
        cerr << exc.what() << '\n';
    }
    
}

Logger & Logger::Instance()
{
    if (!_instance)
    {
        _instance = ptr_type(new Logger());
    }

    return *_instance;
}

void Logger::OpenLogFile(const std::string & fileName)
{
    CloseLogFile();

    ostream_ptr_type logFile = make_shared<ofstream>(fileName);

    if (!(*logFile))
        throw runtime_error("Logger: can not open file: " + fileName);

    _logFile = logFile;
}

void Logger::CloseLogFile()
{
    if (!_logFile)
        return;

    _logFile->close();
    _logFile.reset();
}

void Logger::LogInfo(const std::string & fileName, int lineNumber, const std::string & message)
{
    if (!_logFile || !(*_logFile))
    {
        Log(cout, "INFO", fileName, lineNumber, message);
        return;
    }

    Log(*_logFile, "INFO", fileName, lineNumber, message);
}

void Logger::LogError(const std::string & fileName, int lineNumber, const std::string & message)
{
    if (!_logFile || !(*_logFile))
    {
        Log(cerr, "ERROR", fileName, lineNumber, message);
        return;
    }

    Log(*_logFile, "ERROR", fileName, lineNumber, message);
}

void Logger::LogError(const std::string & fileName, int lineNumber, const std::exception & exc)
{
    LogError(fileName, lineNumber, exc.what());
}

void Logger::Log(std::ostream & os, const std::string & messageType, const std::string & fileName, int lineNumber, const std::string & message)
{
    LogCurrentTime(os);
    os << " " << messageType << ": " << message << "(Line: " << lineNumber << " File: " << fileName << ")" << endl;
}

void Logger::LogCurrentTime(std::ostream & os)
{
    os << format("{:%Y-%m-%d %H:%M:%S}", chrono::system_clock::now());
}

std::string Logger::CreateLinuxLogFilename(const std::string & applicationName)
{
    filesystem::path fullPath = "/var/log";
    fullPath /= applicationName + ".log";

    return fullPath;
}
