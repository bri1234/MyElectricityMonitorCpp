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
    typedef std::vector <uint8_t> buffer_type;
    
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

        constexpr static const char * UnitDcVoltage = "V";
        constexpr static const char * UnitDcCurrent = "A";
        constexpr static const char * UnitDcPower = "W";
        constexpr static const char * UnitDcEnergyTotal = "kWh";
        constexpr static const char * UnitDcEnergyDay = "Wh";

        ChannelReadings();
        ChannelReadings(int channelNumber);

        /// @brief Returns the channel number.
        /// @return The channel number: 1, 2 or 4.
        int GetChannelNumber() const { return _channelNumber; }

        /// @brief Returns the voltage.
        /// @return The voltage in V.
        double GetDcVoltage() const { return _dcVoltage; }

        /// @brief Returns the current.
        /// @return The current in A.
        double GetDcCurrent() const { return _dcCurrent; }

        /// @brief Returns the power.
        /// @return The power in W.
        double GetDcPower() const { return _dcPower; }

        /// @brief Returns the total energy.
        /// @return The energy in kWh.
        double GetDcEnergyTotal() const { return _dcEnergyTotal; }

        /// @brief Returns the day energy.
        /// @return The energy in Wh.
        double GetDcEnergyDay() const { return _dcEnergyDay; }

        /// @brief Prints the data.
        /// @param os The output stream.
        void Print(std::ostream & os) const;

        /// @brief Extracts the readings from the raw data.
        /// @param data The raw data.
        /// @param idxV The index of the voltage.
        /// @param idxC The index of the current.
        /// @param idxP The index of the power.
        /// @param idxEtotal The index of the energy (total).
        /// @param idxEday The index of the energy (day).
        void ExtractReadings(const buffer_type & data,
            int idxV, int idxC, int idxP, int idxEtotal, int idxEday);

    private:
        int    _channelNumber;
        double _dcVoltage = 0.0;
        double _dcCurrent = 0.0;
        double _dcPower = 0.0;
        double _dcEnergyTotal = 0.0;
        double _dcEnergyDay = 0.0;
    };

    class Readings
    {
    public:
        
        constexpr static const char * UnitAcVoltage = "V";
        constexpr static const char * UnitAcCurrent = "A";
        constexpr static const char * UnitAcFrequency = "Hz";
        constexpr static const char * UnitAcPower = "W";
        constexpr static const char * UnitAcPowerFactor = "";
        constexpr static const char * UnitAcReactivePower = "var";
        constexpr static const char * UnitTemperature = "°C";
        constexpr static const char * UnitEVT = "";

        /// @brief Returns the voltage.
        /// @return The voltage in V.
        double GetAcVoltage() const { return _acVoltage; }

        /// @brief Returns the current.
        /// @return The current in A.
        double GetAcCurrent() const { return _acCurrent; }

        /// @brief Returns the frequency.
        /// @return The frequency in Hz.
        double GetAcFrequency() const { return _acFrequency; }

        /// @brief Returns the power.
        /// @return The power in W.
        double GetAcPower() const { return _acPower; }

        /// @brief Returns the power factor.
        /// @return The power factor.
        double GetAcPowerFactor() const { return _acPowerFactor; }

        /// @brief Returns the reactive power.
        /// @return The reactive power in var.
        double GetAcReactivePower() const { return _acReactivePower; }

        /// @brief Returns the temperature.
        /// @return The temperature in °C.
        double GetTemperature() const { return _temperature; }

        /// @brief Returns the EVT.
        /// @return The EVT.
        double GetEVT() const { return _EVT; }

        /// @brief Returns the number of channels.
        /// @return The number of channels.
        int NumberOfChannels() const { return (int)_channelReadingsList.size(); }

        /// @brief The channel readings.
        /// @param channelNumber The channel number. (starts at 0)
        /// @return The channel readings.
        const ChannelReadings & GetChannelReadings(int channelNumber) const { return _channelReadingsList.at(channelNumber); }

        /// @brief Prints the data.
        /// @param os The output stream.
        void Print(std::ostream & os) const;

        /// @brief Extracts the readings from the raw data.
        /// @param numberOfChannels The number of channels: 1, 2 or 4.
        /// @param data The raw data.
        void ExtractReadings(int numberOfChannels, const buffer_type & data);

    private:
        ChannelReadings::list_type _channelReadingsList;

        double _acVoltage = 0.0;
        double _acCurrent = 0.0;
        double _acFrequency = 0.0;
        double _acPower = 0.0;
        double _acPowerFactor = 0.0;
        double _acReactivePower = 0.0; // Q = Blindleistung
        double _temperature = 0.0; 
        double _EVT = 0.0; 

        /// @brief Extracts the readings from the raw data.
        /// @param data The raw data.
        /// @param idxV The index of the voltage.
        /// @param idxC The index of the current.
        /// @param idxF The index of the freqeuency.
        /// @param idxP The index of the power.
        /// @param idxPF The index of the power factor.
        /// @param idxRP The index of the reactive power.
        /// @param idxT The index of the temperature.
        /// @param idxEVT The index of the EVT.
        void ExtractReadings(const buffer_type & data,
            int idxV, int idxF, int idxP, int idxRP, int idxC, int idxPF, int idxT, int idxEVT);
    };

    /// @brief Creates a new Hoymiles HM communication object.
    /// @param inverterSerialNumber The 12 digits inverter serial number. (As printed on the sticker on the inverter case.)
    /// @param pinCSn The CSN pin as SPI device number (0 or 1), usually 0. Defaults to 0.
    /// @param pinCE The GPIO pin connected to the NRF24L01 CE signal. Defaults to 24.
    HoymilesHmDtu(const std::string & inverterSerialNumber, int pinCSn = 0, int pinCE = 24);
    virtual ~HoymilesHmDtu();

    HoymilesHmDtu(const HoymilesHmDtu &) = delete;
    HoymilesHmDtu & operator=(const HoymilesHmDtu &) = delete;
    
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
    static buffer_type GenerateDtuRadioAddress();

    /// @brief Returns the inverter radio ID (4 byte) from the serial number. The radio ID is used to send and receive packets.
    /// @param inverterSerialNumber The 12 digits inverter serial number.
    /// @return The 4 bytes inverter radio ID.
    static buffer_type GetInverterRadioAddress(const std::string & inverterSerialNumber);

    /// @brief Determines the number of inberter channels from serial number.
    /// @param inverterSerialNumber The inverter serial number as printed on the sticker on the inverter case.
    /// @return Number of inverter channels: 1 = HM300, HM350, HM400; 2 = HM600, HM700, HM800; 4 = HM1200, HM1500
    static int GetInverterNumberOfChannels(const std::string & inverterSerialNumber);

    /// @brief Throws an error if the communication is not intialized.
    void AssertCommunicationIsInitialized() const;

    /// @brief Replaces bytes with special meaning by escape sequences.
    /// @param dest The destination buffer with the escaped data.
    /// @param src The source buffer.
    static void EscapeData(buffer_type & dest, const buffer_type & src);

    /// @brief Remove escape sequences for bytes with special meanings.
    /// @param dest The destination buffer with the unescaped data.
    /// @param src The source buffer.
    static void UnescapeData(buffer_type & dest, const buffer_type & src);
    
    /// @brief Calculates CRC8 checksum for communication with hoymiles inverters. poly = 0x101; reversed = False; init-value = 0x00; XOR-out = 0x00; Check = 0x31
    /// @param data The byte array on which the ckecksum is to be calculated.
    /// @param startPos The start position. (index starts at 0)
    /// @param endPos The end position. (The position after the last byte.)
    /// @return The crc checksum.
    static uint8_t CalculateCrc8(const buffer_type & data, size_t startPos, size_t endPos);

    /// @brief Calculates CRC16 checksum for communication with hoymiles inverters. poly = 0x8005; reversed = True; init-value = 0xFFFF; XOR-out = 0x0000; Check = 0x4B37
    /// @param data The byte array on which the ckecksum is to be calculated.
    /// @param startPos The start position. (index starts at 0)
    /// @param endPos The end position. (The position after the last byte.)
    /// @return The crc checksum.
    static uint16_t CalculateCrc16(const buffer_type & data, size_t startPos, size_t endPos);

    /// @brief Checks the checksum of a packet.
    /// @param packet The packet to be checked.
    /// @return True if the checksum is valid.
    static bool CheckPacketChecksum(const buffer_type & packet);

    /// @brief Creates the packet header.
    /// @param packetHeader The created packet header. (This function does not clear the buffer.)
    /// @param command The packet command.
    /// @param receiverAddr The address of the receiver generated from the receiver (inverter) serial number. (4 bytes)
    /// @param senderAddr The address of the sender generated from the sender (DTU) serial number. (4 bytes)
    /// @param frame The frame number for message data.
    static void CreatePacketHeader(buffer_type & packetHeader, uint8_t command,
        const buffer_type & receiverAddr, const buffer_type & senderAddr, uint8_t frame);
    
    /// @brief Creates payload data for info request filled with the time.
    /// @param payload The payload data. (This function does not clear the buffer.)
    /// @param currentTime The current time in seconds since the epoch.
    static void CreateRequestInfoPayload(buffer_type & payload, uint32_t currentTime);

    /// @brief Creates the packet that can be sent to the inverter to request information.
    /// @param packet The packet to be sent to the inverter. (This function clears the buffer first.)
    /// @param receiverAddr The address of the receiver generated from the receiver (inverter) serial number. (4 bytes)
    /// @param senderAddr The address of the sender generated from the sender (DTU) serial number. (4 bytes)
    /// @param currentTime The current time in seconds since the start of the epoch.
    static void CreateRequestInfoPacket(buffer_type & packet,
        const buffer_type & receiverAddr, const buffer_type & senderAddr, uint32_t currentTime);
    
    /// @brief Send a request to the inverter and scan receive channels for the response.
    /// @param responsePacketList List of reponse packets.
    /// @param txChannel The channel where the request shall be sent.
    /// @param rxChannelList The channel list to scan for responses.
    /// @param txPacket The request packet that shall be sent.
    void SendRequestAndScanForResponses(std::vector <buffer_type> & responsePacketList,
        int txChannel, const std::vector <int> & rxChannelList, const buffer_type & txPacket);
    
    /// @brief Checks if the responses are valid and returns the assembled data.
    /// @param responseData The assembled response data.
    /// @param responsePacketList List of received responses packets.
    /// @param inverterRadioAddress The inverter radio address (4 bytes).
    /// @param inverterNumberOfChannels The number of inverter channels.
    /// @return True if all responses are valid.
    static bool EvaluateInverterInfoResponse(buffer_type & responseData,
        const std::vector <buffer_type> & responsePacketList,
        const buffer_type & inverterRadioAddress, int inverterNumberOfChannels);

    /// @brief Extracts the inverter infos from the reponse data.
    /// @param readings The inverter readings.
    /// @param responseData The response data.
    /// @param numberOfChannels Number of inverter channels.
    /// @return True if successfull
    static bool ExtractInverterReadings(Readings & readings, const buffer_type & responseData, int numberOfChannels);
};

