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

#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <cstdint>
#include <algorithm>

namespace Utils
{
    /// @brief Appends the contents of one vector to another.
    /// @tparam T The type of the vector elements.
    /// @param dest The destination vector.
    /// @param source The source vector to append.
    template <typename T>
    void AppendRange(std::vector <T> & dest, const std::vector <T> & source)
    {
        dest.insert(dest.end(), source.begin(), source.end());
    }

    /// @brief Joins the elements of a vector into a single string with a specified separator.
    /// @tparam T The type of the vector elements.
    /// @param itemList The list of items to join.
    /// @param separator The separator string.
    /// @return The joined string.
    template <typename T>
    void Join(std::ostream & os, std::vector <T> & itemList, const std::string & separator)
    {
        bool first = true;

        for (const auto & item : itemList)
        {
            if (first)
                first = false;
            else
                os << separator;

            os << item;
        }
    }
    
    /// @brief Joins the elements of a vector into a single string with a specified separator.
    /// @tparam T The type of the vector elements.
    /// @param itemList The list of items to join.
    /// @param separator The separator string.
    /// @return The joined string.
    template <typename T>
    std::string Join(std::vector <T> & itemList, const std::string & separator)
    {
        std::ostringstream os;
        Join(os, itemList, separator);
        return os.str();
    }
    
    /// @brief Converts a string to a double.
    /// @param str The string to convert.
    /// @return The converted double.
    double StrToDouble(const std::string & str);

    /// @brief Converts a number to an array of bytes.
    /// @tparam T Number type.
    /// @param number The number to convert to bytes.
    /// @return The bytes.
    template <typename T>
    std::vector<uint8_t> ToBytes(T number, int numberOfBytes, bool bigEndian)
    {
        std::vector<uint8_t> bytes;

        for (int idx = 0; idx < numberOfBytes; idx++)
        {
            bytes.push_back(static_cast<uint8_t>(number & 0xFF));
            number >>= 8;
        }

        if (bigEndian)
            std::reverse(bytes.begin(), bytes.end());
            
        return bytes;
    }
}
