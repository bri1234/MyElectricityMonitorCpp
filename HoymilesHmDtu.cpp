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

HoymilesHmDtu::HoymilesHmDtu(const std::string & inverterSerialNumber, int pinCSn, int pinCE)
    : _inverterSerialNumber(inverterSerialNumber)
    , _pinCSn(pinCSn)
    , _pinCE(pinCE)
{
    if (_inverterSerialNumber.length() != 12)
        throw Error(format("Inverter serial number has not 12 digits: {}", _inverterSerialNumber));

    _dtuRadioAddress = GenerateDtuRadioAddress();
    _inverterRadioAddress = GetInverterRadioAddress(_inverterSerialNumber);
    _inverterNumberOfChannels = GetInverterNumberOfChannels(_inverterSerialNumber);

    // _radio(pinCe, pinCsn, SPI_FREQUENCY_HZ)
}

void HoymilesHmDtu::PrintNrf24l01Info()
{
    AssertCommunicationIsInitialized();
    _radio->printPrettyDetails();
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

    return UInt32ToBytes(id, true);
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

int HoymilesHmDtu::GetInverterNumberOfChannels(const std::string & inverterSerialNumber)
{
    string inverterType = inverterSerialNumber.substr(0, 2);
    if ((inverterType == "10") || (inverterType == "11"))
    {
        string inverterSubType = inverterSerialNumber.substr(2, 2);

        if ((inverterSubType == "21") || (inverterSubType == "22") || (inverterSubType == "24"))
            return 1;

        if ((inverterSubType == "41") || (inverterSubType == "42") || (inverterSubType == "44"))
            return 2;

        if ((inverterSubType == "61") || (inverterSubType == "62") || (inverterSubType == "64"))
            return 4;
    }

    throw Error(format("Inverter type with serial number {} is not supported.", inverterSerialNumber));
}

void HoymilesHmDtu::AssertCommunicationIsInitialized() const
{
    if (!_radio)
    {
        throw Error("Communication is not initialized!");
    }
}

void HoymilesHmDtu::EscapeData(std::vector<uint8_t> & dest, const std::vector<uint8_t> & src)
{
    dest.clear();
    dest.reserve(src.size() * 2);

    // Replaces bytes with special meaning by escape sequences.
    // 0x7D -> 0x7D 0x5D
    // 0x7E -> 0x7D 0x5E
    // 0x7F -> 0x7D 0x5F

    for (uint8_t b : src)
    {
        switch (b)
        {
            case 0x7D:
                dest.push_back(0x7D);
                dest.push_back(0x5D);
                break;

            case 0x7E:
                dest.push_back(0x7D);
                dest.push_back(0x5E);
                break;

            case 0x7F:
                dest.push_back(0x7D);
                dest.push_back(0x5F);
                break;

            default:
                dest.push_back(b);
                break;
        }
    }
}

void HoymilesHmDtu::UnescapeData(std::vector<uint8_t> & dest, const std::vector<uint8_t> & src)
{
    dest.clear();
    dest.reserve(src.size());

    for (size_t idx = 0; idx < src.size(); idx++)
    {
        uint8_t b = src[idx];

        if (b == 0x7D)
        {
            idx++;
            b = src.at(idx);

            switch (b)
            {
                case 0x5D:
                    dest.push_back(0x7D);
                    break;

                case 0x5E:
                    dest.push_back(0x7E);
                    break;

                case 0x5F:
                    dest.push_back(0x7F);
                    break;

                default:
                    throw Error("UnescapeData(): Invalid data, can not decode.");
            }
        }
        else
        {
            dest.push_back(b);
        }

    }
}

uint8_t HoymilesHmDtu::CalculateCrc8(const std::vector<uint8_t> & data, size_t dataLen)
{
    uint32_t crc = 0;

    for (size_t idx = 0; idx < dataLen; idx++)
    {
        crc ^= data[idx];

        for (int p = 0; p < 8; p++)
        {
            crc <<= 1;

            if (crc & 0x0100)
                crc ^= 0x01;

            crc &= 0xFF;
        }
    }

    return static_cast<uint8_t>(crc);
}

uint16_t HoymilesHmDtu::CalculateCrc16(const std::vector<uint8_t> & data, size_t dataLen)
{
    uint32_t crc = 0xFFFF;

    for (size_t idx = 0; idx < dataLen; idx++)
    {
        crc ^= data[idx];

        for (int p = 0; p < 8; p++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return static_cast<uint16_t>(crc);
}

bool HoymilesHmDtu::CheckPacketChecksum(const std::vector<uint8_t> & packet)
{
    uint8_t checksum1 = CalculateCrc8(packet, packet.size() - 1);
    uint8_t checksum2 = packet[packet.size() - 1];

    return checksum1 == checksum2;
}

