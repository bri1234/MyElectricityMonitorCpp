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

#include "SerialPort.h"

#include <stdexcept>
#include <format>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

using namespace std;

SerialPort::SerialPort()
{
}

SerialPort::~SerialPort()
{
    ClosePort();
}

void SerialPort::OpenPort(const std::string & serialPortName)
{
    ClosePort();

    _fileDescriptor = open(serialPortName.c_str(), O_RDWR);
    if (_fileDescriptor < 0)
        throw runtime_error(format("SerialPort: can not open port {}: error {} {}", serialPortName, errno, strerror(errno)));

    _serialPortName = serialPortName;
}

void SerialPort::ClosePort()
{
    if (_fileDescriptor < 0)
        return;

    close(_fileDescriptor);
    _fileDescriptor = -1;
    _serialPortName.clear();
}

void SerialPort::ConfigurePort(int baudrate, Parity parity, int dataBits, int stopBits,
    bool enableHardwareFlowControl, bool enableSoftwareFlowControl, double readTimeoutSeconds) const
{
    AssertPortIsOpen();

    struct termios tty;

    if (tcgetattr(_fileDescriptor, &tty) != 0)
        throw runtime_error(format("SerialPort: can not get configuration for port {}: error {} {}", _serialPortName, errno, strerror(errno)));
    
    switch (baudrate)
    {
        case 9600:
            cfsetispeed(&tty, B9600);
            cfsetospeed(&tty, B9600);
            break;
        case 19200:
            cfsetispeed(&tty, B19200);
            cfsetospeed(&tty, B19200);
            break;
        case 38400:
            cfsetispeed(&tty, B38400);
            cfsetospeed(&tty, B38400);
            break;
        case 57600:
            cfsetispeed(&tty, B57600);
            cfsetospeed(&tty, B57600);
            break;
        case 115200:
            cfsetispeed(&tty, B115200);
            cfsetospeed(&tty, B115200);
            break;
        default:
            throw runtime_error(format("SerialPort: unsupported baudrate {} for port {}", baudrate, _serialPortName));
    }

    switch (parity)
    {
        case P_NONE:
            tty.c_cflag &= ~PARENB;
            break;
        case P_EVEN:
            tty.c_cflag |= PARENB;
            tty.c_cflag &= ~PARODD;
            break;
        case P_ODD:
            tty.c_cflag |= PARENB;
            tty.c_cflag |= PARODD;
            break;
        default:
            throw runtime_error(format("SerialPort: unsupported parity {} for port {}", (int)parity, _serialPortName));
    }

    tty.c_cflag &= ~CSIZE;
    switch (dataBits)
    {
        case 7:
            tty.c_cflag |= CS7;
            break;
        case 8:
            tty.c_cflag |= CS8;
            break;
        default:
            throw runtime_error(format("SerialPort: unsupported data bits {} for port {}", dataBits, _serialPortName));
    }

    switch (stopBits)
    {
        case 1:
            tty.c_cflag &= ~CSTOPB;
            break;
        case 2:
            tty.c_cflag |= CSTOPB;
            break;
        default:
            throw runtime_error(format("SerialPort: unsupported stop bits {} for port {}", stopBits, _serialPortName));
    }

    if (enableHardwareFlowControl)
        tty.c_cflag |= CRTSCTS;
    else
        tty.c_cflag &= ~CRTSCTS;

    // turn on READ & ignore modem control lines
    tty.c_cflag |= CREAD | CLOCAL;

    // disable canonical mode
    tty.c_lflag &= ~ICANON;

    // disable echo, erasure, new-line echo
    tty.c_lflag &= ~(ECHO | ECHOE | ECHONL);

    // disable interpretation of INTR, QUIT and SUSP
    tty.c_lflag &= ~ISIG;
    
    if (enableSoftwareFlowControl)
        tty.c_iflag |= (IXON | IXOFF | IXANY);
    else
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    
    // disable any special handling of received bytes
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    // disable any special handling of output bytes
    tty.c_oflag &= ~(OPOST | ONLCR);
    
    if (readTimeoutSeconds < 0.0)
        throw runtime_error(format("SerialPort: invalid read timeout {} for port {}", readTimeoutSeconds, _serialPortName));

    tty.c_cc[VTIME] = (int)(readTimeoutSeconds * 10.0); // in tenths of a second
    tty.c_cc[VMIN] = 0;

    if (tcsetattr(_fileDescriptor, TCSANOW, &tty) != 0)
        throw runtime_error(format("SerialPort: can not change configuration for port {}: error {} {}", _serialPortName, errno, strerror(errno)));
}

void SerialPort::AssertPortIsOpen() const
{
    if (_fileDescriptor < 0)
        throw runtime_error("SerialPort: port is not open!");
}

void SerialPort::WriteData(const void * data, size_t length) const
{
    AssertPortIsOpen();

    size_t bytesWritten = write(_fileDescriptor, data, length);
    if (bytesWritten != length)
    {
        throw runtime_error(format("SerialPort: can not write data to port {}: error {} {} ({} of {} bytes written)",
            _serialPortName, errno, strerror(errno), bytesWritten, length));
    }
}

void SerialPort::ReadData(std::vector<uint8_t> & buffer, size_t length, bool blocking) const
{
    buffer.resize(length);
    auto bytesRead = ReadData(buffer.data(), length, blocking);
    buffer.resize(bytesRead);
}

size_t SerialPort::ReadData(void * data, size_t length, bool blocking) const
{
    AssertPortIsOpen();

    if (blocking)
    {
        return ReadDataBlocking(data, length);
    }

    return ReadDataNonBlocking(data, length);
}

size_t SerialPort::ReadDataBlocking(void * data, size_t length) const
{
    AssertPortIsOpen();

    size_t totalBytesRead = 0;
    uint8_t * dataPtr = static_cast<uint8_t *>(data);

    while (totalBytesRead < length)
    {
        ssize_t bytesRead = read(_fileDescriptor, dataPtr + totalBytesRead, length - totalBytesRead);
        if (bytesRead < 0)
        {
            throw runtime_error(format("SerialPort: can not read data from port {}: error {} {} ({} of {} bytes read)",
                _serialPortName, errno, strerror(errno), totalBytesRead, length));
        }
        else if (bytesRead == 0)
        {
            // Timeout reached
            break;
        }

        totalBytesRead += static_cast<size_t>(bytesRead);
    }

    if (totalBytesRead < length)
    {
        throw runtime_error(format("SerialPort: timeout reading data from port {}: only {} of {} bytes read",
            _serialPortName, totalBytesRead, length));
    }

    return totalBytesRead;
}

size_t SerialPort::ReadDataNonBlocking(void * data, size_t length) const
{
    AssertPortIsOpen();

    size_t bytesRead = read(_fileDescriptor, data, length);
    if (bytesRead < 0)
    {
        throw runtime_error(format("SerialPort: can not read data from port {}: error {} {}",
            _serialPortName, errno, strerror(errno)));
    }

    return bytesRead;
}
