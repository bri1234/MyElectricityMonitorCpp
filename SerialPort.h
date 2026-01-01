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
#include <cstdint>
#include <stdexcept>

class SerialPort
{
public:
    /// @brief Parity options for serial port configuration
    enum Parity { P_NONE, P_EVEN, P_ODD };

    /// @brief Serial port error.
    class Error : public std::runtime_error
    {
    public:
        Error(const std::string & errorMessage) : std::runtime_error(std::format("Serial port error: {}", errorMessage)) { }
    };

    /// @brief Database error.
    class Timeout : public Error
    {
    public:
        Timeout(const std::string & errorMessage) : Error(std::format("Timeout: {}", errorMessage)) { }
    };

    /// @brief Constructor
    SerialPort();

    /// @brief Destructor
    ~SerialPort();

    /// @brief Opens the serial port.
    /// @param serialPortName The name of the serial port (e.g. "/dev/ttyS0" on Linux).
    void OpenPort(const std::string & serialPortName);

    /// @brief Closes the serial port.
    void ClosePort();

    /// @brief Configures the serial port.
    /// @param baudrate The baud rate (e.g. 9600, 19200, 38400, 57600, 115200).
    /// @param parity The parity setting (P_NONE, P_EVEN, P_ODD).
    /// @param dataBits The number of data bits (e.g. 7, 8).
    /// @param stopBits The number of stop bits (e.g. 1, 2).
    /// @param enableHardwareFlowControl Enable or disable hardware flow control (RTS/CTS).
    /// @param enableSoftwareFlowControl Enable or disable software flow control (XON/XOFF).
    /// @param readTimeoutSeconds Read timeout in seconds.
    void ConfigurePort(int baudrate, Parity parity, int dataBits, int stopBits, bool enableHardwareFlowControl,
        bool enableSoftwareFlowControl, double readTimeoutSeconds) const;
    
    /// @brief Writes data to the serial port.
    /// @param str The string data to write.
    void WriteData(const std::string & str) const { WriteData(str.data(), str.length()); }

    /// @brief Writes data to the serial port.
    /// @param data The vector of bytes to write.
    void WriteData(const std::vector<uint8_t> & data) const { WriteData(data.data(), data.size()); }

    /// @brief Writes data to the serial port.
    /// @param data Pointer to the data to write.
    /// @param length Length of the data to write.
    void WriteData(const void * data, size_t length) const;

    /// @brief Reads data from the serial port into a vector.
    /// @param buffer The vector to store the read data.
    /// @param length The number of bytes to read.
    /// @param blocking If true, the read operation will block until the requested number of bytes is read or a timeout occurs.
    void ReadData(std::vector<uint8_t> & buffer, size_t length, bool blocking) const;

    /// @brief Reads data from the serial port into a buffer.
    /// @param data Pointer to the buffer to store the read data.
    /// @param length The number of bytes to read.
    /// @param blocking If true, the read operation will block until the requested number of bytes is read or a timeout occurs.
    /// @return The number of bytes actually read. (May be less than length or 0 if non blocking and no data is available.)
    size_t ReadData(void * data, size_t length, bool blocking) const;

    /// @brief Reads data from the serial port in blocking mode. Exception is thrown on timeout.
    /// @param data The buffer to read data into.
    /// @param length The number of bytes to read.
    /// @return The number of bytes actually read. (Exactly length bytes or exception on timeout.)
    size_t ReadDataBlocking(void * data, size_t length) const;

    /// @brief Reads data from the serial port in non-blocking mode. No exception is thrown on timeout.
    /// @param data The buffer to read data into.
    /// @param length The number of bytes to read.
    /// @return The number of bytes actually read. (May be less than length or 0 if no data is available.)
    size_t ReadDataNonBlocking(void * data, size_t length) const;
    
    /// @brief Queries the number of bytes available in the receive buffer.
    /// @return The number of bytes available in the receive buffer.
    int GetNumberOfBytesAvailable() const;
    
    /// @brief Discards all data that is in the input buffer.
    void ClearInputBuffer() const;

private:
    int _fileDescriptor = -1;
    std::string _serialPortName;

    /// @brief Asserts that the serial port is open.
    void AssertPortIsOpen() const;
};

