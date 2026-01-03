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

#include <RF24/RF24.h>

#include <vector>
#include <map>
#include <cstdint>

/// @brief Class for communication with HM300, HM350, HM400, HM600, HM700, HM800, HM1200 & HM1500 inverter. (DTU means 'data transfer unit'.)
class HoymilesHmDtu
{
public:

    /// @brief Hoymiles HM DTU error.
    class Error : public std::runtime_error
    {
    public:
        Error(const std::string & errorMessage) : std::runtime_error(std::format("Hoymiles HM DTU error: {}", errorMessage)) { }
    };

    HoymilesHmDtu(const std::string & inverterSerialNumber,
                  int pinCsn = 0, int pinCe = 24);


                
private:
    // the nRF24L01 receive pipeline
    constexpr static int RX_PIPE_NUM = 1;

    // timeout (in ms) for channel scan
    constexpr static int RECEIVE_TIMEOUT_MS = 5;

    // the SPI communication frequency (in Hz)
    constexpr static int SPI_FREQUENCY_HZ = 1000000;

    // the power level to send the request to the receiver
    constexpr static rf24_pa_dbm_e RADIO_POWER_LEVEL = RF24_PA_MAX;

    // maximum size of packets that can be sent with the nRF24L01 module
    constexpr static int MAX_PACKET_SIZE = 32;

    // list of channels where the inverter is listening for requests
    static const std::vector <int> TX_CHANNELS;

    // list of channels where the inverter sends the responses depending on the channel, where the request was received
    static const std::map <int, std::vector <int>> RX_CHANNEL_LISTS;

    RF24 _radio;

    std::string _inverterSerialNumber;
    
    /// @brief Generates a 4 byte DTU radio ID (data transfer unit, this device) from the system UUID. The radio ID is used to send and receive packets.
    /// @return The 4 bytes DTU radio ID.
    static std::vector <uint8_t> GenerateDtuRadioAddress();

    /// @brief Returns the inverter radio ID (4 byte) from the serial number. The radio ID is used to send and receive packets.
    /// @param inverterSerialNumber The 12 digits inverter serial number.
    /// @return The 4 bytes inverter radio ID.
    static std::vector <uint8_t> GetInverterRadioAddress(const std::string & inverterSerialNumber);
};

