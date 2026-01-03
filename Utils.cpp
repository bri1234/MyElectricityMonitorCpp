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

#include "Utils.h"

#include <stdexcept>

using namespace std;

namespace Utils
{

    double StrToDouble(const std::string & str)
    {
        size_t idx = 0;
        double d = stod(str, &idx);

        for (; idx < str.size(); idx++)
        {
            if (!isspace(str[idx]))
                throw invalid_argument("Invalid double string: " + str);
        }   
        
        return d;
    }

    std::vector<uint8_t> UInt32ToBytes(uint32_t number, bool bigEndian)
    {
        std::vector<uint8_t> bytes;

        for (int idx = 0; idx < 4; idx++)
        {
            bytes.push_back(static_cast<uint8_t>(number & 0xFF));
            number >>= 8;
        }

        if (bigEndian)
            std::reverse(bytes.begin(), bytes.end());
            
        return bytes;
    }

    uint16_t GetUInt16(const std::vector<uint8_t> & data, int position)
    {
        uint16_t n = data[position];
        n <<= 8;
        n |= data[position + 1];

        return n;
    }

    uint32_t GetUInt32(const std::vector<uint8_t> & data, int position)
    {
        uint32_t n = data[position];
        n <<= 8;
        n |= data[position + 1];
        n <<= 8;
        n |= data[position + 2];
        n <<= 8;
        n |= data[position + 3];

        return n;
    }

}
