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

#include <json-c/json.h>
#include <string>
#include <stdexcept>
#include <format>

/// @brief A class for handling JSON data.
class Json
{
public:

    /// @brief JSON error.
    class Error : public std::runtime_error
    {
    public:
        Error(const std::string & errorMessage) : std::runtime_error(std::format("JSON error: {}", errorMessage)) { }
    };

    /// @brief Constructor.
    Json();

    /// @brief Destructor.
    ~Json();

    /// @brief Loads JSON data from a file.
    /// @param filename The filename.
    void LoadFromFile(const std::string & filename);

    /// @brief Returns the root JSON object.
    /// @return The root JSON object.
    json_object * GetRootObject() const;

private:
    json_object *_jsonRoot;
    
    /// @brief Frees the JSON root object.
    void FreeJsonRoot();
};

