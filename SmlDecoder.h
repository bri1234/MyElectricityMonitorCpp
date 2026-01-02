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
#include <memory>
#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <format>

/// @brief Represents the decoded SML data.
class SmlData
{
public:

    typedef std::vector<SmlData> list_type;
    typedef std::vector<uint8_t> byte_array_type;

    enum DataType
    {
        DT_STRING = 0,
        DT_BOOL = 4,
        DT_INTEGER = 5,
        DT_UNSIGNED = 6,
        DT_LIST = 7
    };

    /// @brief EbzDd3 error.
    class Error : public std::runtime_error
    {
    public:
        Error(const std::string & errorMessage) : std::runtime_error(std::format("SML data error: {}", errorMessage)) { }
    };

    /// @brief Constructor.
    SmlData();

    /// @brief Returns the data type.
    /// @return The data type.
    DataType GetDataType() const { return _dataType; }

    bool IsBool() const { return _dataType == DT_BOOL; }
    bool IsInteger() const { return _dataType == DT_INTEGER; }
    bool IsUnsigned() const { return _dataType == DT_UNSIGNED; }
    bool IsString() const { return _dataType == DT_STRING; }
    bool IsList() const { return _dataType == DT_LIST; }

    const std::string & GetString() const { AssertIsDataType(DT_STRING); return _string; }
    bool GetBool() const { AssertIsDataType(DT_BOOL); return _boolean; }
    int64_t GetInteger() const { AssertIsDataType(DT_INTEGER); return _integer; }
    uint64_t GetUnsigned() const { AssertIsDataType(DT_UNSIGNED); return _unsigned; }
    const list_type & GetList() const { AssertIsDataType(DT_LIST); return _list; }
    const SmlData & GetListItem(int index) const { AssertIsDataType(DT_LIST); return _list.at(index); }

    /// @brief Prints the SML data structure.
    /// @param os The output stream.
    void PrintValue(std::ostream & os) const { PrintValue(os, 0); }

    /// @brief Decodes a SML value from the byte array.
    /// @param data The raw SML binary data.
    /// @param position The position to start decoding.
    /// @param valueEndPos The position after the decoded value.
    /// @param endOfMsg True if the end of the SML message is reached.
    /// @return The decoded SML data.
    static SmlData DecodeValue(const byte_array_type & data, int position,
        int & valueEndPos, bool & endOfMsg);

private:
    DataType _dataType;
    std::string _string;
    bool _boolean;
    int64_t _integer;
    uint64_t _unsigned;
    list_type _list;

    void AssertIsDataType(DataType expectedDataType) const;
    static const char * DataTypeToString(DataType dataType);

    /// @brief Prints the SML data structure.
    /// @param os The output stream.
    /// @param indent The number of spaces to indent.
    void PrintValue(std::ostream & os, int indent) const;

    /// @brief Decodes the type length field.
    /// @param data The raw SML binary data.
    /// @param position The position of the type length field.
    /// @param tlFieldSize The number of type length field read.
    /// @param dataType The decoded data type.
    /// @param dataLen The decoded data length.
    static void DecodeTypeLengthField(const byte_array_type & data, int position,
        int & tlFieldSize, DataType & dataType, int & dataLen);
};

/// @brief Checks if the check sum of the SML message is valid.
/// @param data The raw byte data of the SML message.
/// @return True if the ckeck sum is valid.
bool CheckIfSmlIsValid(const SmlData::byte_array_type & data);

/// @brief Decodes the SML messages.
/// @param data The raw byte data of the SML messages.
/// @return The list of decoded messages.
SmlData::list_type DecodeSmlMessages(const SmlData::byte_array_type & data);
