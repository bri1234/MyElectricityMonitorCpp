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
#include <map>
#include <cstdint>
#include <format>

#include "SerialPort.h"
#include "Gpio.h"
#include "SmlDecoder.h"

/// @brief Class to interface with two EBZ DD3 electricity meter via a serial port and GPIO.
class EbzDd3
{
public:

    /// @brief The electricity meter readings.
    class Readings
    {
    public:
        constexpr static double InvalidValue = -1.0;

        /// @brief meter reading +A, tariff-free in kWh (+A: Active energy, grid supplies to customer)
        double PlusA = InvalidValue;   
        constexpr static const char * UnitPlusA = "kWh";

        /// @brief meter reading +A, tariff 1 in kWh (+A: Active energy, grid supplies to customer)
        double PlusA_T1 = InvalidValue;   
        constexpr static const char * UnitPlusA_T1 = "kWh";

        /// @brief meter reading +A, tariff 2 in kWh (+A: Active energy, grid supplies to customer)
        double PlusA_T2 = InvalidValue;   
        constexpr static const char * UnitPlusA_T2 = "kWh";

        /// @brief meter reading -A, tariff-free in kWh (-A: Active energy, customer supplies to grid)
        double MinusA = InvalidValue;   
        constexpr static const char * UnitMinusA = "kWh";

        /// @brief Sum of instantaneous power in all phases in W
        double Power = InvalidValue;   
        constexpr static const char * UnitPower = "W";

        /// @brief Instantaneous power phase L1 in W
        double PowerL1 = InvalidValue;   
        constexpr static const char * UnitPowerL1 = "W";

        /// @brief Instantaneous power phase L2 in W
        double PowerL2 = InvalidValue;   
        constexpr static const char * UnitPowerL2 = "W";

        /// @brief Instantaneous power phase L3 in W
        double PowerL3 = InvalidValue;   
        constexpr static const char * UnitPowerL3 = "W";

        /// @brief Sets all values to "InvalidValue".
        void Clear();

        /// @brief Prints the readings.
        /// @param os The stream where the readings shall be printed.
        void Print(std::ostream & os);

        /// @brief Returns the readings as a dictionary.
        /// @param readings The readings dictionary.
        void GetReadings(std::map <std::string, double> & readings);
    };

    /// @brief EbzDd3 error.
    class Error : public std::runtime_error
    {
    public:
        Error(const std::string & errorMessage) : std::runtime_error(std::format("EbzDd3 error: {}", errorMessage)) { }
    };

    /// @brief Constructor
    /// @param serialPortName The name of the serial port (e.g. "/dev/ttyS0" on Linux).
    /// @param gpioPinSwitch The GPIO pin number used to switch between electricity meter 1 and 2 (default: 17).
    EbzDd3(const std::string & serialPortName, int gpioPinSwitch = 17);

    ~EbzDd3();

    EbzDd3(const EbzDd3 &) = delete;
    EbzDd3 & operator=(const EbzDd3 &) = delete;
    
    /// @brief Opens the connection to the electricity meter.
    void Open();

    /// @brief Closes the connection to the electricity meter.
    void Close();

    /// @brief Selects the channel (= the electricity meter) to read from.
    /// @param channelNum The channel (= the electricity meter) 0 or 1.
    void SelectChannel(int channelNum);

    /// @brief Receives the information from a electricity meter. (The meter readings.)
    /// @param channelNum The channel (= the electricity meter) 0 or 1.
    /// @param readings The electricity meter readings.
    /// @return True if readings are valid.
    bool ReceiveInfo(int channelNum, Readings & readings);

private:
    std::string _serialPortName;
    int _gpioSwitch;

    SerialPort _serialPort;
    Gpio _gpio;

    bool _isOpen;
    
    /// @brief Reads a data block from the serial port with specified timeouts.
    /// @param serialPort The serial port to read from.
    /// @param data The data buffer to store the read data.
    /// @param timeoutBetweenBytes The timeout between receiving two bytes in seconds.
    /// @param timeoutFirstByte The timeout for receiving the first byte in seconds.
    static void ReadBlock(const SerialPort & serialPort, std::vector <uint8_t> & data, double timeoutBetweenBytes, double timeoutFirstByte);

    /// @brief Receives the data of one full info message.
    /// @param data The data buffer where the received message data is stored.
    /// @param channelNum The channel (= electricity meter 0 or 1).
    void ReceiveInfoData(std::vector <uint8_t> & data, int channelNum);

    /// @brief Extracts meter readings from the received raw data.
    /// @param data The received raw data.
    /// @param readings The meter readinds.
    static void ExtractInfoFromData(const std::vector <uint8_t> & data, Readings & readings);

    /// @brief Extracts meter reading from one dataset.
    /// @param dataSet The data for one reading.
    /// @param readings Where to store the reading.
    /// @return True if a known reading was found-
    static bool ExtractInfoFromDataSet(const SmlData & dataSet, Readings & readings);

    /// @brief Ensures that the electricity meter connection is open.
    void AssertIsOpen();
};

