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
#include <format>
#include <memory>
#include <random>

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
    
    class ChannelReadings
    {
    public:
        typedef std::vector<ChannelReadings> list_type;

        double DcVoltage = 0.0;
        constexpr static const char * UnitDcVoltage = "V";

        double DcCurrent = 0.0;
        constexpr static const char * UnitDcCurrent = "A";

        double DcPower = 0.0;
        constexpr static const char * UnitDcPower = "W";

        double DcEnergyTotal = 0.0;
        constexpr static const char * UnitDcEnergyTotal = "kWh";

        double DcEnergyDay = 0.0;
        constexpr static const char * UnitDcEnergyDay = "Wh";
    };

    class Readings
    {
    public:
        
        double AcVoltage = 0.0;
        constexpr static const char * UnitAcVoltage = "V";

        double AcCurrent = 0.0;
        constexpr static const char * UnitAcCurrent = "A";

        double AcFrequency = 0.0;
        constexpr static const char * UnitAcFrequency = "Hz";

        double AcPower = 0.0;
        constexpr static const char * UnitAcPower = "W";

        double AcPowerFactor = 0.0;
        constexpr static const char * UnitAcPowerFactor = "";

        double AcReactivePower = 0.0; // Q = Blindleistung
        constexpr static const char * UnitAcReactivePower = "var";

        double Temperature = 0.0; 
        constexpr static const char * UnitTemperature = "°C";

        double EVT = 0.0; 
        constexpr static const char * UnitEVT = "";

        Readings(int numberOfChannels = 1);

        /// @brief Returns the number of channels.
        /// @return The number of channels.
        int NumberOfChannels() const { return (int)_channelReadingsList.size(); }

        /// @brief The channel readings.
        /// @param channelNumber The channel number. (starts at 0)
        /// @return The channel readings.
        const ChannelReadings & GetChannelReadings(int channelNumber) const { return _channelReadingsList.at(channelNumber); }

        /// @brief Stores the readings of one channel.
        /// @param channelNumber The channel number. (starts at 0)
        /// @param newChannelReadings The readings to be stored.
        void SetChannelReadings(int channelNumber, const ChannelReadings & newChannelReadings) { _channelReadingsList.at(channelNumber) = newChannelReadings; }

        /// @brief Clears the readings.
        /// @param numberOfChannels The number of channels.
        void Clear(int numberOfChannels);

    private:
        ChannelReadings::list_type _channelReadingsList;
    };

    /// @brief Creates a new Hoymiles HM communication object.
    /// @param inverterSerialNumber The 12 digits inverter serial number. (As printed on the sticker on the inverter case.)
    /// @param pinCSn The CSN pin as SPI device number (0 or 1), usually 0. Defaults to 0.
    /// @param pinCE The GPIO pin connected to the NRF24L01 CE signal. Defaults to 24.
    HoymilesHmDtu(const std::string & inverterSerialNumber, int pinCSn = 0, int pinCE = 24);
    virtual ~HoymilesHmDtu();

    /// @brief Prints NRF24L01 module information on standard output.
    void PrintNrf24l01Info();

    /// @brief Initializes the communication.
    void InitializeCommunication();

    /// @brief Terminates the communication.
    void TerminateCommunication();

    /// @brief Requests info data from the inverter and returns the inverter response.
    /// @param readings The readings from the inverter.
    /// @param numberOfRetries Number of requests before giving up.
    /// @param waitBeforeRetry Time (in s) to wait before a new request is sent to the inverter if the previous request was not successful.
    /// @return Success (true or false)
    bool QueryInverterInfo(Readings & readings, int numberOfRetries = 20, double waitBeforeRetry = 1.0);

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

    std::shared_ptr<RF24> _radio;

    std::string _inverterSerialNumber;
    int _pinCSn;
    int _pinCE;

    std::vector<uint8_t> _dtuRadioAddress;
    std::vector<uint8_t> _inverterRadioAddress;

    std::vector<uint8_t> _writingPipeAddress;
    std::vector<uint8_t> _readingPipeAddress;

    int _inverterNumberOfChannels;

    // needed for random numbers
    std::minstd_rand _randomEngine;
    std::uniform_int_distribution<int> _randomTxChannel;

    /// @brief Generates a 4 byte DTU radio ID (data transfer unit, this device) from the system UUID. The radio ID is used to send and receive packets.
    /// @return The 4 bytes DTU radio ID.
    static std::vector <uint8_t> GenerateDtuRadioAddress();

    /// @brief Returns the inverter radio ID (4 byte) from the serial number. The radio ID is used to send and receive packets.
    /// @param inverterSerialNumber The 12 digits inverter serial number.
    /// @return The 4 bytes inverter radio ID.
    static std::vector <uint8_t> GetInverterRadioAddress(const std::string & inverterSerialNumber);

    /// @brief Determines the number of inberter channels from serial number.
    /// @param inverterSerialNumber The inverter serial number as printed on the sticker on the inverter case.
    /// @return Number of inverter channels: 1 = HM300, HM350, HM400; 2 = HM600, HM700, HM800; 4 = HM1200, HM1500
    static int GetInverterNumberOfChannels(const std::string & inverterSerialNumber);

    /// @brief Throws an error if the communication is not intialized.
    void AssertCommunicationIsInitialized() const;

    /// @brief Replaces bytes with special meaning by escape sequences.
    /// @param dest The destination buffer with the escaped data.
    /// @param src The source buffer.
    static void EscapeData(std::vector <uint8_t> & dest, const std::vector <uint8_t> & src);

    /// @brief Remove escape sequences for bytes with special meanings.
    /// @param dest The destination buffer with the unescaped data.
    /// @param src The source buffer.
    static void UnescapeData(std::vector <uint8_t> & dest, const std::vector <uint8_t> & src);
    
    /// @brief Calculates CRC8 checksum for communication with hoymiles inverters. poly = 0x101; reversed = False; init-value = 0x00; XOR-out = 0x00; Check = 0x31
    /// @param data The byte array on which the ckecksum is to be calculated.
    /// @param dataLen Number of bytes to be used to calculate the checksum.
    /// @return The crc checksum.
    static uint8_t CalculateCrc8(const std::vector <uint8_t> & data, size_t dataLen);

    /// @brief Calculates CRC16 checksum for communication with hoymiles inverters. poly = 0x8005; reversed = True; init-value = 0xFFFF; XOR-out = 0x0000; Check = 0x4B37
    /// @param data The byte array on which the ckecksum is to be calculated.
    /// @param dataLen Number of bytes to be used to calculate the checksum.
    /// @return The crc checksum.
    static uint16_t CalculateCrc16(const std::vector <uint8_t> & data, size_t dataLen);

    /// @brief Checks the checksum of a packet.
    /// @param packet The packet to be checked.
    /// @return True if the checksum is valid.
    static bool CheckPacketChecksum(const std::vector <uint8_t> & packet);

    /// @brief Creates the packet header.
    /// @param packetHeader The created packet header.
    /// @param command The packet command.
    /// @param receiverAddr The address of the receiver generated from the receiver (inverter) serial number. (4 bytes)
    /// @param senderAddr The address of the sender generated from the sender (DTU) serial number. (4 bytes)
    /// @param frame The frame number for message data.
    static void CreatePacketHeader(std::vector <uint8_t> & packetHeader, uint8_t command,
        const std::vector <uint8_t> & receiverAddr, const std::vector <uint8_t> & senderAddr, uint8_t frame);
    
    /// @brief Creates payload data for info request filled with the time.
    /// @param payload The payload data.
    /// @param currentTime The current time in seconds since the epoch.
    static void CreateRequestInfoPayload(std::vector <uint8_t> & payload, uint32_t currentTime);
};

