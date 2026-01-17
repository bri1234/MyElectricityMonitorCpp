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
#include <iostream>

using namespace std;

using std::chrono::steady_clock;
using std::chrono::duration;

constexpr static const string ID_PLUS_A     = string("\x01\x00\x01\x08\x00\xFF", 6);
constexpr static const string ID_PLUS_A_T1  = string("\x01\x00\x01\x08\x01\xFF", 6);
constexpr static const string ID_PLUS_A_T2  = string("\x01\x00\x01\x08\x02\xFF", 6);
constexpr static const string ID_MINUS_A    = string("\x01\x00\x02\x08\x00\xFF", 6);
constexpr static const string ID_POWER      = string("\x01\x00\x10\x07\x00\xFF", 6);
constexpr static const string ID_POWER_L1   = string("\x01\x00\x24\x07\x00\xFF", 6);
constexpr static const string ID_POWER_L2   = string("\x01\x00\x38\x07\x00\xFF", 6);
constexpr static const string ID_POWER_L3   = string("\x01\x00\x4C\x07\x00\xFF", 6);

EbzDd3::EbzDd3(const std::string & serialPortName, int gpioPinSwitch)
: _serialPortName(serialPortName)
, _gpioSwitch(gpioPinSwitch)
, _gpio("EbzDd3")
, _isOpen(false)
{

}

EbzDd3::~EbzDd3()
{   
    try
    {
        Close();
    }
    catch (const exception & exc)
    {
        LOG_ERROR(exc);
    }
}

void EbzDd3::Open()
{
    Close();

    _serialPort.OpenPort(_serialPortName);
    _serialPort.ConfigurePort(9600, SerialPort::P_NONE, 8, 1, false, false, 0.1);
    
    _gpio.InitializeGpioLine(_gpioSwitch, Gpio::GD_OUTPUT);

    _isOpen = true;

    SelectChannel(0);
}

void EbzDd3::Close()
{
    _isOpen = false;
    _serialPort.ClosePort();
}

void EbzDd3::SelectChannel(int channelNum)
{
    AssertIsOpen();

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

    char buffer[8];
    auto tm = steady_clock::now();

    // extra timeout for the first byte?
    if (timeoutFirstByte > 0.0)
    {
        while (true)
        {
            try
            {
                serialPort.ReadData(buffer, 1, true);
                data.push_back(buffer[0]);

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
            serialPort.ReadData(buffer, 1, true);
            data.push_back(buffer[0]);

            tm = steady_clock::now();
        }
        catch(const SerialPort::Timeout &)
        {
            // ignore
        }
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

bool EbzDd3::ExtractInfoFromDataSet(const SmlData & dataSet, Readings & readings)
{
    const std::string & id = dataSet.GetListItem(0).GetString();

    // +A: Active energy, grid supplies to customer.
    // -A: Active energy, customer supplies to grid

    if (id == ID_PLUS_A)
    {
        // meter reading +A, tariff-free in kWh
        double value = dataSet.GetListItem(5).GetUnsigned();
        readings.PlusA = value / 1E8;
        return true;
    }

    if (id == ID_PLUS_A_T1)
    {
        // meter reading +A, tariff 1 in kWh
        double value = dataSet.GetListItem(5).GetUnsigned();
        readings.PlusA_T1 = value / 1E8;
        return true;
    }

    if (id == ID_PLUS_A_T2)
    {
        // meter reading +A, tariff 2 in kWh
        double value = dataSet.GetListItem(5).GetUnsigned();
        readings.PlusA_T2 = value / 1E8;
        return true;
    }

    if (id == ID_MINUS_A)
    {
        // meter reading -A, tariff-free in kWh
        double value = dataSet.GetListItem(5).GetUnsigned();
        readings.MinusA = value / 1E8;
        return true;
    }

    if (id == ID_POWER)
    {
        // Sum of instantaneous power in all phases in W
        double value = dataSet.GetListItem(5).GetInteger();
        readings.Power = value / 1E2;
        return true;
    }

    if (id == ID_POWER_L1)
    {
        // Instantaneous power phase L1 in W
        double value = dataSet.GetListItem(5).GetInteger();
        readings.PowerL1 = value / 1E2;
        return true;
    }

    if (id == ID_POWER_L2)
    {
        // Instantaneous power phase L2 in W
        double value = dataSet.GetListItem(5).GetInteger();
        readings.PowerL2 = value / 1E2;
        return true;
    }

    if (id == ID_POWER_L3)
    {
        // Instantaneous power phase L3 in W
        double value = dataSet.GetListItem(5).GetInteger();
        readings.PowerL3 = value / 1E2;
        return true;
    }

    return false;
}

void EbzDd3::ExtractInfoFromData(const std::vector<uint8_t> & data, Readings & readings)
{
    auto messageList = DecodeSmlMessages(data);

    // get the useful data sets
    const SmlData & message = messageList.at(1);
    const SmlData & dataSetList = message.GetListItem(3).GetListItem(1).GetListItem(4);

    // message.PrintValue(cout);

    for (const auto & dataSet : dataSetList.GetList())
    {
        ExtractInfoFromDataSet(dataSet, readings);
    }
}

bool EbzDd3::ReceiveInfo(int channelNum, Readings & readings)
{
    AssertIsOpen();
    
    readings.Clear();

    try
    {
        vector <uint8_t> data;

        ReceiveInfoData(data, channelNum);
        if (data.size() == 0)
            return false;

        ExtractInfoFromData(data, readings);

        return true;
    }
    catch (const exception & exc)
    {
        LOG_ERROR(exc);
        return false;
    }
}

void EbzDd3::AssertIsOpen()
{
    if (!_isOpen)
        throw Error("electricity meter connection is not open!");
}

void EbzDd3::Readings::Clear()
{
    PlusA = InvalidValue;
    PlusA_T1 = InvalidValue;
    PlusA_T2 = InvalidValue;
    MinusA = InvalidValue;
    Power = InvalidValue;
    PowerL1 = InvalidValue;
    PowerL2 = InvalidValue;
    PowerL3 = InvalidValue;
}

void EbzDd3::Readings::Print(std::ostream & os)
{
    os << "+A    = " << PlusA << " " << UnitPlusA << endl;
    os << "+A T1 = " << PlusA_T1 << " " << UnitPlusA_T1 << endl;
    os << "+A T2 = " << PlusA_T2 << " " << UnitPlusA_T2 << endl;
    os << "-A    = " << MinusA << " " << UnitMinusA << endl;
    os << "P     = " << Power << " " << UnitPower << endl;
    os << "P L1  = " << PowerL1 << " " << UnitPowerL1 << endl;
    os << "P L2  = " << PowerL2 << " " << UnitPowerL2 << endl;
    os << "P L3  = " << PowerL3 << " " << UnitPowerL3 << endl;
}

void EbzDd3::Readings::GetReadings(std::map <std::string, double> & readings)
{
    readings.clear();

    readings["+A"] = PlusA;
    readings["+A T1"] = PlusA_T1;
    readings["+A T2"] = PlusA_T2;
    readings["-A"] = MinusA;
    readings["P"] = Power;
    readings["P L1"] = PowerL1;
    readings["P L2"] = PowerL2;
    readings["P L3"] = PowerL3;
}
