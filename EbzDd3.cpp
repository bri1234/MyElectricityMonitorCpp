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

#include "EbzDd3.h"

#include "Logger.h"

#include <format>
#include <stdexcept>
#include <thread>
#include <chrono>

using namespace std;

using std::chrono::steady_clock;
using std::chrono::duration;

EbzDd3::EbzDd3(const std::string & serialPortName, int gpioSwitch)
: _serialPortName(serialPortName)
, _gpioSwitch(gpioSwitch)
, _gpio("EbzDd3")
{

}

EbzDd3::~EbzDd3()
{   
    try
    {
        Close();
    }
    catch (const exception & e)
    {
        LOG_ERROR(e);
    }
}

void EbzDd3::Open()
{
    Close();

    _serialPort.OpenPort(_serialPortName);
    _serialPort.ConfigurePort(9600, SerialPort::P_NONE, 8, 1, false, false, 0.1);
    
    _gpio.InitializeGpioLine(_gpioSwitch, Gpio::GD_OUTPUT);

    SelectChannel(0);
}

void EbzDd3::Close()
{
    _serialPort.ClosePort();
}

void EbzDd3::SelectChannel(int channelNum)
{
    switch (channelNum)
    {
    case 0:
        _gpio.SetPinLevel(_gpioSwitch, 0);
        break;
    
    case 1:
        _gpio.SetPinLevel(_gpioSwitch, 1);
        break;
    
    default:
        throw Error(format("SelectChannel(): Invalid channel number {}. Must be 0 or 1.", channelNum));
    }

    this_thread::sleep_for(chrono::milliseconds(100));
}

void EbzDd3::ReadBlock(const SerialPort & serialPort, std::vector <uint8_t> & data, double timeoutBetweenBytes, double timeoutFirstByte)
{
    data.clear();

    auto tm = steady_clock::now();

    // extra timeout for the first byte?
    if (timeoutFirstByte > 0.0)
    {
        while (true)
        {
            try
            {
                serialPort.ReadData(data, 1, true);

                // first byte received
                break;
            }
            catch(const SerialPort::Timeout &) { }

            if (duration<double>(steady_clock::now() - tm).count() > timeoutFirstByte)
                return;
        }
    }

    // receive bytes and consider the timeout between the bytes
    tm = steady_clock::now();
    
    while (duration<double>(steady_clock::now() - tm).count() < timeoutBetweenBytes)
    {
        try
        {
            serialPort.ReadData(data, 1, true);
            tm = steady_clock::now();
        }
        catch(const SerialPort::Timeout &) { }
    }
}

void EbzDd3::ReceiveInfoData(std::vector <uint8_t> & data, int channelNum)
{
    SelectChannel(channelNum);

    // discard old data
    _serialPort.ClearInputBuffer();

    // wait until time gap before start of the info message
    ReadBlock(_serialPort, data, 0.3, 0.0);
    
    // now receive the info message
    ReadBlock(_serialPort, data, 0.3, 1.0);
}

