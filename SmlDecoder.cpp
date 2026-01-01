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

#include "SmlDecoder.h"

#include <format>
#include <stdexcept>
#include <algorithm>

using namespace std;

static const uint16_t CRC16_X25_TABLE[] = {
    0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD,	0x6536, 0x74BF,
    0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7,
    0x1081, 0x0108,	0x3393, 0x221A, 0x56A5, 0x472C, 0x75B7, 0x643E,
    0x9CC9, 0x8D40, 0xBFDB, 0xAE52, 0xDAED, 0xCB64,	0xF9FF, 0xE876,
    0x2102, 0x308B, 0x0210, 0x1399,	0x6726, 0x76AF, 0x4434, 0x55BD,
    0xAD4A, 0xBCC3,	0x8E58, 0x9FD1, 0xEB6E, 0xFAE7, 0xC87C, 0xD9F5,
    0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E,	0x54B5, 0x453C,
    0xBDCB, 0xAC42, 0x9ED9, 0x8F50,	0xFBEF, 0xEA66, 0xD8FD, 0xC974,
    0x4204, 0x538D,	0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB,
    0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1,	0xAB7A, 0xBAF3,
    0x5285, 0x430C, 0x7197, 0x601E,	0x14A1, 0x0528, 0x37B3, 0x263A,
    0xDECD, 0xCF44,	0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72,
    0x6306, 0x728F, 0x4014, 0x519D, 0x2522, 0x34AB,	0x0630, 0x17B9,
    0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5,	0xA96A, 0xB8E3, 0x8A78, 0x9BF1,
    0x7387, 0x620E,	0x5095, 0x411C, 0x35A3, 0x242A, 0x16B1, 0x0738,
    0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862,	0x9AF9, 0x8B70,
    0x8408, 0x9581, 0xA71A, 0xB693,	0xC22C, 0xD3A5, 0xE13E, 0xF0B7,
    0x0840, 0x19C9,	0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF,
    0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324,	0xF1BF, 0xE036,
    0x18C1, 0x0948, 0x3BD3, 0x2A5A,	0x5EE5, 0x4F6C, 0x7DF7, 0x6C7E,
    0xA50A, 0xB483,	0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5,
    0x2942, 0x38CB, 0x0A50, 0x1BD9, 0x6F66, 0x7EEF,	0x4C74, 0x5DFD,
    0xB58B, 0xA402, 0x9699, 0x8710,	0xF3AF, 0xE226, 0xD0BD, 0xC134,
    0x39C3, 0x284A,	0x1AD1, 0x0B58, 0x7FE7, 0x6E6E, 0x5CF5, 0x4D7C,
    0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1,	0xA33A, 0xB2B3,
    0x4A44, 0x5BCD, 0x6956, 0x78DF,	0x0C60, 0x1DE9, 0x2F72, 0x3EFB,
    0xD68D, 0xC704,	0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232,
    0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68,	0x3FF3, 0x2E7A,
    0xE70E, 0xF687, 0xC41C, 0xD595,	0xA12A, 0xB0A3, 0x8238, 0x93B1,
    0x6B46, 0x7ACF,	0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9,
    0xF78F, 0xE606, 0xD49D, 0xC514, 0xB1AB, 0xA022,	0x92B9, 0x8330,
    0x7BC7, 0x6A4E, 0x58D5, 0x495C,	0x3DE3, 0x2C6A, 0x1EF1, 0x0F78 };

static const SmlData::byte_array_type ESCAPE_SEQUENCE = { 0x1B, 0x1B, 0x1B, 0x1B };

static const SmlData::byte_array_type SML_START = { 0x01, 0x01, 0x01, 0x01 };

/// @brief Calculates the CRC16 checksum for Smart Message Language of a byte array.
/// @param data The byte array on which the ckecksum is to be calculated.
/// @param dataLen Number of bytes to be used to calculate the checksum.
/// @return The CRC16 checksum of the byte array.
static uint16_t CalculateSmlCrc16(const SmlData::byte_array_type & data, int dataLen)
{
    uint16_t crcsum = 0xFFFF;

    for (int idx = 0; idx < dataLen; idx++)
    {
        crcsum = CRC16_X25_TABLE[(data[idx] ^ crcsum) & 0xff] ^ (crcsum >> 8 & 0xff);
    }

    crcsum ^= 0xffff;
    return crcsum;
}

/// @brief Decodes an unsigned integer from the byte array (big endian).
/// @param data The raw SML binary data.
/// @param position The position to start decoding.
/// @param length The number of bytes to decode (1 .. 8).
/// @return The decoded unsigned integer.
static uint64_t DecodeUnsignedBigEndian(const SmlData::byte_array_type & data, int position, int length)
{
    if ((length < 1) || (length > 8))
        throw SmlData::Error(format("DecodeUnsigned: length {} is out of range (1 .. 8).", length));

    uint64_t value = 0;

    for (int idx = 0; idx < length; idx++)
    {
        value <<= 8;
        value |= data[position + idx];
    }

    return value;
}

/// @brief Decodes an signed integer from the byte array (big endian).
/// @param data The raw SML binary data.
/// @param position The position to start decoding.
/// @param length The number of bytes to decode (1 .. 8).
/// @return The decoded signed integer.
static int64_t DecodeIntegerBigEndian(const SmlData::byte_array_type & data, int position, int length)
{
    if ((length < 1) || (length > 8))
        throw SmlData::Error(format("DecodeInteger: length {} is out of range (1 .. 8).", length));

    bool isNegative = (data[position] & 0x80) != 0;
    int64_t value = isNegative ? -1 : 0;

    for (int idx = 0; idx < length; idx++)
    {
        value <<= 8;
        value |= data[position + idx];
    }

    return value;
}

/// @brief Decodes an 16 bit unsigned integer from the byte array (little endian).
/// @param data The raw SML binary data.
/// @param position The position to start decoding.
/// @param length The number of bytes to decode (1 .. 8).
/// @return The decoded unsigned integer.
static uint16_t DecodeUnsigned16LittleEndian(const SmlData::byte_array_type & data, int position)
{
    uint16_t value = data[position + 1];
    value <<= 8;
    value |= data[position];

    return value;
}

bool CheckIfSmlIsValid(const SmlData::byte_array_type & data)
{
    int count = (int)data.size();
    if (count < 2)
        return false;

    uint16_t checkSum1 = DecodeUnsigned16LittleEndian(data, count - 2);
    uint16_t checkSum2 = CalculateSmlCrc16(data, count - 2);

    return checkSum1 == checkSum2;
}

SmlData::SmlData()
: _dataType(DT_INTEGER)
, _boolean(false)
, _integer(0)
, _unsigned(0)
{
}

void SmlData::AssertIsDataType(DataType expectedDataType) const
{
    if (_dataType != expectedDataType)
    {
        throw Error(format("SmlData: data type expexted {} but is {}.",
            DataTypeToString(expectedDataType), DataTypeToString(_dataType)));
    }
}

const char * SmlData::DataTypeToString(DataType dataType)
{
    switch (dataType)
    {
    case DT_STRING:
        return "String";
    case DT_BOOL:
        return "Boolean";
    case DT_INTEGER:
        return "Integer";
    case DT_UNSIGNED:
        return "Unsigned";
    case DT_LIST:
        return "List";
    default:
        return "???";
    }
}

void SmlData::PrintValue(std::ostream & os, int indent) const
{
    for (int idx = 0; idx < indent; idx++)
        os << " ";
    
    switch (_dataType)
    {
    case DT_LIST:
        os << "List (" << _list.size() << "):" << endl;
        for (const SmlData & listItem : _list)
            listItem.PrintValue(os, indent + 4);
        break;
    
    case DT_STRING:
        os << "String (" << _string.length() << "): " << _string << endl;
        break;
    
    case DT_BOOL:
        os << "Bool: " << (_boolean ? "true" : "false") << endl;
        break;

    case DT_INTEGER:
        os << "Integer: " << _integer << endl;
        break;

    case DT_UNSIGNED:
        os << "Unsigned: " << _unsigned << endl;
        break;

    default:
        break;
    }
}

void SmlData::DecodeTypeLengthField(const byte_array_type & data, int position,
        int & tlFieldSize, DataType & dataType, int & dataLen)
{
    tlFieldSize = 1;
    auto tlField = data[position];

    dataType = (DataType)((tlField & 0x70) >> 4);
    dataLen = tlField & 0x0F;

    while ((tlField & 0x80) != 0)
    {
        position += 1;
        tlFieldSize += 1;
        tlField = data[position];

        dataLen <<= 4;
        dataLen |= tlField & 0x0F;
    }
}

SmlData SmlData::DecodeValue(const byte_array_type & data, int position,
    int & valueEndPos, bool & endOfMsg)
{
    int tlFieldSize, dataLen;
    DataType dataType;

    DecodeTypeLengthField(data, position, tlFieldSize, dataType, dataLen);

    int valueStartPos = position + tlFieldSize;
    valueEndPos = position + dataLen;
    endOfMsg = false;
    SmlData value;

    if (data[position] == 0)
    {
        endOfMsg = true;
        valueEndPos = position + 1;
    }
    else if ((dataType == DT_STRING) && (dataLen >= 1))
    {
        value._dataType = DT_STRING;
        value._string.assign(data.begin() + valueStartPos, data.begin() + valueEndPos);
    }
    else if ((dataType == DT_BOOL) && (dataLen == 2))
    {
        value._dataType = DT_BOOL;
        value._boolean = (data[valueStartPos] != 0);
    }
    else if ((dataType == DT_INTEGER) && (dataLen >= 2) && (dataLen <= 9))
    {
        value._dataType = DT_INTEGER;
        value._integer = DecodeIntegerBigEndian(data, valueStartPos, valueEndPos - valueStartPos);
    }
    else if ((dataType == DT_UNSIGNED) && (dataLen >= 2) && (dataLen <= 9))
    {
        value._dataType = DT_UNSIGNED;
        value._unsigned = DecodeUnsignedBigEndian(data, valueStartPos, valueEndPos - valueStartPos);
    }
    else if (dataType == DT_LIST)
    {
        value._dataType = DT_LIST;
        
        for (int idx = 0; idx < dataLen; idx++)
        {
            SmlData listItem = DecodeValue(data, valueStartPos, valueEndPos, endOfMsg);
            valueStartPos = valueEndPos;

            if (!endOfMsg)
                value._list.push_back(listItem);
        }
    }
    else
    {
        throw Error(format("Unknown data type {} at position {}", (int)dataType, position));
    }

    return value;
}

SmlData::list_type DecodeSmlMessages(const SmlData::byte_array_type & data)
{
    // check for escape sequence
    if (!equal(ESCAPE_SEQUENCE.begin(), ESCAPE_SEQUENCE.end(), data.begin()))
        throw SmlData::Error("DecodeSmlMessages: missing escape sequence at position 0");

    // check version
    if (!equal(SML_START.begin(), SML_START.end(), data.begin() + 4))
        throw SmlData::Error("DecodeSmlMessages: missing SML start sequence at position 4");

    // check for second escape sequence
    size_t count = data.size();

    if (!equal(ESCAPE_SEQUENCE.begin(), ESCAPE_SEQUENCE.end(), data.begin() + count - 8))
        throw SmlData::Error(format("DecodeSmlMessages: missing second escape sequence at position {}", count - 8));

    if (data[count - 4] != 0x1A)
        throw SmlData::Error(format("DecodeSmlMessages: missing 0x1A at position {}", count - 4));

    // get number of fill bytes
    int numberOfFillBytes = data[count - 3];
    int lastMsgBodyIndex = count - 8 - numberOfFillBytes;

    // check checksum
    uint16_t checkSum1 = DecodeUnsigned16LittleEndian(data, count - 2);
    uint16_t checkSum2 = CalculateSmlCrc16(data, count - 2);

    if (checkSum1 != checkSum2)
        throw SmlData::Error(format("DecodeSmlMessages: Checksum error: found {:04X} calculated {:04X}", checkSum1, checkSum2));
    
    SmlData::list_type messageList;
    int position = 8;

    while (position < lastMsgBodyIndex)
    {
        bool endOfMsg;
        SmlData message = SmlData::DecodeValue(data, position, position, endOfMsg);

        if (!endOfMsg)
            throw SmlData::Error(format("DecodeSmlMessages: missing end of message at position {}", position));

        messageList.push_back(message);
    }

    return messageList;
}


