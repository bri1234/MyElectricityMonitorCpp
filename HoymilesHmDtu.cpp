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

#include "HoymilesHmDtu.h"

#include "Utils.h"

#include <unistd.h>

using namespace std;
using namespace Utils;

const std::vector<int> HoymilesHmDtu::TX_CHANNELS = { 3, 23, 40, 61, 75 };

const std::map <int, std::vector <int>> RX_CHANNEL_LISTS = {
    {  3, { 23, 40, 61 }},
    { 23, { 40, 61, 75 }},
    { 40, { 61, 75,  3 }},
    { 61, { 75,  3, 23 }},
    { 75, {  3, 23, 40 }},
};

HoymilesHmDtu::HoymilesHmDtu(const std::string & inverterSerialNumber,
                             int pinCsn, int pinCe)
    : _radio(pinCe, pinCsn, SPI_FREQUENCY_HZ)
    , _inverterSerialNumber(inverterSerialNumber)
{

}

std::vector<uint8_t> HoymilesHmDtu::GenerateDtuRadioAddress()
{
    long uuid = gethostid();
    uint32_t id = 0;

    for (int idx = 0; idx < 7; idx++)
    {
        id |= uuid % 10;
        id <<= 4;
        uuid /= 10;
    }

    id |= 0x80000000;

    return ToBytes(id, 4, true);
}

std::vector<uint8_t> HoymilesHmDtu::GetInverterRadioAddress(const std::string & inverterSerialNumber)
{
    if (inverterSerialNumber.length() != 12)
        throw Error(format("GetInverterRadioAddress: inverter serial number must have 12 digits ({})", inverterSerialNumber));

    vector<uint8_t> bytes;

    for (int idx = 0; idx < 4; idx ++)
    {
        string numberStr = inverterSerialNumber.substr(4 + idx * 2, 2);
        size_t endIdx = 0;
        int number = stoi(numberStr, &endIdx, 16);

        if (endIdx != 2)
            throw Error(format("GetInverterRadioAddress: {} inverter serial number is not a number ({})", numberStr, inverterSerialNumber));

        bytes.push_back(static_cast<uint8_t>(number));
    }

    return bytes;
}

