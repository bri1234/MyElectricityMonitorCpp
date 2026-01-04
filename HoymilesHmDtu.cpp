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

#include "HoymilesHmDtu.h"

#include "Utils.h"
#include "OnScopeExit.h"
#include "Logger.h"

#include <unistd.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;
using namespace std::chrono;
using namespace Utils;

const std::vector<int> HoymilesHmDtu::TX_CHANNELS = { 3, 23, 40, 61, 75 };

const std::map <int, std::vector <int>> RX_CHANNEL_LISTS = {
    {  3, { 23, 40, 61 }},
    { 23, { 40, 61, 75 }},
    { 40, { 61, 75,  3 }},
    { 61, { 75,  3, 23 }},
    { 75, {  3, 23, 40 }},
};

void HoymilesHmDtu::ChannelReadings::Print(std::ostream & os) const
{
    os << "*** Channel " << ChannelNumber << " ***" << endl;
    os << "    DC Voltage:   " << DcVoltage     << " " << UnitDcVoltage << endl;
    os << "    DC Current:   " << DcCurrent     << " " << UnitDcCurrent << endl;
    os << "    DC Power:     " << DcPower       << " " << UnitDcPower << endl;
    os << "    Energy total: " << DcEnergyTotal << " " << UnitDcEnergyTotal << endl;
    os << "    Energy day:   " << DcEnergyDay   << " " << UnitDcEnergyDay << endl;
}

HoymilesHmDtu::Readings::Readings(int numberOfChannels)
{
    Clear(numberOfChannels);
}

void HoymilesHmDtu::Readings::Clear(int numberOfChannels)
{
    if ((numberOfChannels != 1) && (numberOfChannels != 2) && (numberOfChannels != 4))
        throw Error(format("Invalid number of channels: {}", numberOfChannels));
    
    _channelReadingsList.clear();
    _channelReadingsList.resize(numberOfChannels);

    for (int idx = 0; idx < (int)_channelReadingsList.size(); idx++)
        _channelReadingsList[idx].ChannelNumber = idx + 1;
}

void HoymilesHmDtu::Readings::Print(std::ostream & os) const
{
    for (const auto & channelReadings : _channelReadingsList)
        channelReadings.Print(os);

    os << "    AC Voltage:        " << AcVoltage       << " " << UnitAcVoltage << endl;
    os << "    AC Current:        " << AcCurrent       << " " << UnitAcCurrent << endl;
    os << "    AC Power:          " << AcPower         << " " << UnitAcPower << endl;
    os << "    AC Frequency:      " << AcFrequency     << " " << UnitAcFrequency << endl;
    os << "    AC Power factor:   " << AcPowerFactor   << " " << UnitAcPowerFactor << endl;
    os << "    AC Reactive power: " << AcReactivePower << " " << UnitAcReactivePower << endl;
    os << "    Temperature:       " << Temperature     << " " << UnitTemperature << endl;
    os << "    EVT:               " << EVT             << " " << UnitEVT << endl;
}

HoymilesHmDtu::HoymilesHmDtu(const std::string & inverterSerialNumber, int pinCSn, int pinCE)
    : _inverterSerialNumber(inverterSerialNumber)
    , _pinCSn(pinCSn)
    , _pinCE(pinCE)
    , _randomEngine()
    , _randomTxChannel(0, TX_CHANNELS.size() - 1)
{
    if (_inverterSerialNumber.length() != 12)
        throw Error(format("Inverter serial number has not 12 digits: {}", _inverterSerialNumber));

    _dtuRadioAddress = GenerateDtuRadioAddress();
    _inverterRadioAddress = GetInverterRadioAddress(_inverterSerialNumber);
    _inverterNumberOfChannels = GetInverterNumberOfChannels(_inverterSerialNumber);

    _writingPipeAddress.push_back(0x01);
    AppendRange(_writingPipeAddress, _inverterRadioAddress);

    _readingPipeAddress.push_back(0x01);
    AppendRange(_readingPipeAddress, _dtuRadioAddress);

    
}

HoymilesHmDtu::~HoymilesHmDtu()
{
    try
    {
        TerminateCommunication();
    }
    catch(const exception & exc)
    {
        cerr << exc.what() << endl;
    }
}

void HoymilesHmDtu::PrintNrf24l01Info()
{
    AssertCommunicationIsInitialized();
    _radio->printPrettyDetails();
}

std::vector<uint8_t> HoymilesHmDtu::GenerateDtuRadioAddress()
{
    long uuid = gethostid();
    uint32_t id = 0;

    for (int idx = 0; idx < 7; idx++)
    {
        id |= uuid % 10;
        id <<= 4;
        uuid /= 10;
    }

    id |= 0x80000000;

    std::vector<uint8_t> bytes;
    UInt32ToBytes(bytes, id, true);

    return bytes;
}

std::vector<uint8_t> HoymilesHmDtu::GetInverterRadioAddress(const std::string & inverterSerialNumber)
{
    if (inverterSerialNumber.length() != 12)
        throw Error(format("GetInverterRadioAddress: inverter serial number must have 12 digits ({})", inverterSerialNumber));

    vector<uint8_t> bytes;

    for (int idx = 0; idx < 4; idx ++)
    {
        string numberStr = inverterSerialNumber.substr(4 + idx * 2, 2);
        size_t endIdx = 0;
        int number = stoi(numberStr, &endIdx, 16);

        if (endIdx != 2)
            throw Error(format("GetInverterRadioAddress: {} inverter serial number is not a number ({})", numberStr, inverterSerialNumber));

        bytes.push_back(static_cast<uint8_t>(number));
    }

    return bytes;
}

int HoymilesHmDtu::GetInverterNumberOfChannels(const std::string & inverterSerialNumber)
{
    string inverterType = inverterSerialNumber.substr(0, 2);
    if ((inverterType == "10") || (inverterType == "11"))
    {
        string inverterSubType = inverterSerialNumber.substr(2, 2);

        if ((inverterSubType == "21") || (inverterSubType == "22") || (inverterSubType == "24"))
            return 1;

        if ((inverterSubType == "41") || (inverterSubType == "42") || (inverterSubType == "44"))
            return 2;

        if ((inverterSubType == "61") || (inverterSubType == "62") || (inverterSubType == "64"))
            return 4;
    }

    throw Error(format("Inverter type with serial number {} is not supported.", inverterSerialNumber));
}

void HoymilesHmDtu::AssertCommunicationIsInitialized() const
{
    if (!_radio)
    {
        throw Error("Communication is not initialized!");
    }
}

void HoymilesHmDtu::EscapeData(std::vector<uint8_t> & dest, const std::vector<uint8_t> & src)
{
    dest.clear();
    dest.reserve(src.size() * 2);

    // Replaces bytes with special meaning by escape sequences.
    // 0x7D -> 0x7D 0x5D
    // 0x7E -> 0x7D 0x5E
    // 0x7F -> 0x7D 0x5F

    for (uint8_t b : src)
    {
        switch (b)
        {
            case 0x7D:
                dest.push_back(0x7D);
                dest.push_back(0x5D);
                break;

            case 0x7E:
                dest.push_back(0x7D);
                dest.push_back(0x5E);
                break;

            case 0x7F:
                dest.push_back(0x7D);
                dest.push_back(0x5F);
                break;

            default:
                dest.push_back(b);
                break;
        }
    }
}

void HoymilesHmDtu::UnescapeData(std::vector<uint8_t> & dest, const std::vector<uint8_t> & src)
{
    dest.clear();
    dest.reserve(src.size());

    for (size_t idx = 0; idx < src.size(); idx++)
    {
        uint8_t b = src[idx];

        if (b == 0x7D)
        {
            idx++;
            b = src.at(idx);

            switch (b)
            {
                case 0x5D:
                    dest.push_back(0x7D);
                    break;

                case 0x5E:
                    dest.push_back(0x7E);
                    break;

                case 0x5F:
                    dest.push_back(0x7F);
                    break;

                default:
                    throw Error("UnescapeData(): Invalid data, can not decode.");
            }
        }
        else
        {
            dest.push_back(b);
        }

    }
}

uint8_t HoymilesHmDtu::CalculateCrc8(const std::vector<uint8_t> & data, size_t startPos, size_t endPos)
{
    uint32_t crc = 0;

    for (size_t idx = startPos; idx < endPos; idx++)
    {
        crc ^= data[idx];

        for (int p = 0; p < 8; p++)
        {
            crc <<= 1;

            if (crc & 0x0100)
                crc ^= 0x01;

            crc &= 0xFF;
        }
    }

    return static_cast<uint8_t>(crc);
}

uint16_t HoymilesHmDtu::CalculateCrc16(const std::vector<uint8_t> & data, size_t startPos, size_t endPos)
{
    uint32_t crc = 0xFFFF;

    for (size_t idx = startPos; idx < endPos; idx++)
    {
        crc ^= data[idx];

        for (int p = 0; p < 8; p++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return static_cast<uint16_t>(crc);
}

bool HoymilesHmDtu::CheckPacketChecksum(const std::vector<uint8_t> & packet)
{
    uint8_t checksum1 = CalculateCrc8(packet, 0, packet.size() - 1);
    uint8_t checksum2 = packet[packet.size() - 1];

    return checksum1 == checksum2;
}

void HoymilesHmDtu::InitializeCommunication()
{
    TerminateCommunication();

    auto radio = make_shared<RF24>(_pinCE, _pinCSn, SPI_FREQUENCY_HZ);

    if (!radio->begin())
        throw Error("Can not initialize RF24!");
    
    if (!radio->isChipConnected())
        throw Error("Error chip is not connected!");

    radio->stopListening();

    radio->setDataRate(RF24_250KBPS);
    radio->setPALevel(RF24_PA_MIN);
    radio->setCRCLength(RF24_CRC_16);
    radio->setAddressWidth(5);

    radio->openWritingPipe(&(_writingPipeAddress[0]));
    radio->openReadingPipe(RX_PIPE_NUM, &(_readingPipeAddress[0]));

    radio->enableDynamicPayloads();
    radio->setRetries(3, 10);
    radio->setAutoAck(true);

    _radio = radio;
}

void HoymilesHmDtu::TerminateCommunication()
{
    if (!_radio)
        return;

    // recommended idle behavior is TX mode
    _radio->stopListening();

    _radio.reset();
}

void HoymilesHmDtu::CreatePacketHeader(std::vector<uint8_t> & packetHeader, uint8_t command, const std::vector<uint8_t> & receiverAddr,
    const std::vector<uint8_t> & senderAddr, uint8_t frame)
{
    if (receiverAddr.size() != 4)
        throw Error(format("Invalid length of receiver address: {}. (must be 4 bytes)", receiverAddr.size()));
    
    if (senderAddr.size() != 4)
        throw Error(format("Invalid length of sender address: {}. (must be 4 bytes)", senderAddr.size()));
    
    size_t sz = packetHeader.size();

    packetHeader.push_back(command);
    AppendRange(packetHeader, receiverAddr);
    AppendRange(packetHeader, senderAddr);
    packetHeader.push_back(frame);
    
    if (packetHeader.size() - sz != 10)
        throw Error(format("Internal error __CreatePacketHeader: size {} != 10", packetHeader.size()));
}

void HoymilesHmDtu::CreateRequestInfoPayload(std::vector<uint8_t> & payload, uint32_t currentTime)
{
    size_t sz = payload.size();

    payload.push_back(0x0B); // sub command
    payload.push_back(0x00); // revision

    payload.push_back((uint8_t)(currentTime >> 24));
    payload.push_back((uint8_t)(currentTime >> 16));
    payload.push_back((uint8_t)(currentTime >>  8));
    payload.push_back((uint8_t)(currentTime >>  0));

    payload.push_back(0x00);
    payload.push_back(0x00);
    payload.push_back(0x00);

    payload[9] = 0x05;
    
    payload.push_back(0x00);
    payload.push_back(0x00);
    payload.push_back(0x00);
    payload.push_back(0x00);
    
    if (payload.size() - sz != 14)
        throw Error(format("Internal error CreateRequestInfoPayload: size {} != 14", payload.size()));
}

void HoymilesHmDtu::CreateRequestInfoPacket(std::vector<uint8_t> & packet, const std::vector<uint8_t> & receiverAddr,
    const std::vector<uint8_t> & senderAddr, uint32_t currentTime)
{
    packet.clear();
    packet.reserve(MAX_PACKET_SIZE);

    vector<uint8_t> tmp;
    tmp.reserve(MAX_PACKET_SIZE);
    
    // add the header
    CreatePacketHeader(tmp, 0x15, receiverAddr, senderAddr, 0x80);

    // add the payload
    size_t payloadStartPos = tmp.size();
    CreateRequestInfoPayload(tmp, currentTime);

    // add the payload checksum
    uint16_t payloadChecksum = CalculateCrc16(tmp, payloadStartPos, tmp.size());
    UInt16ToBytes(tmp, payloadChecksum, true);

    // add the packet checksum
    uint8_t packetChecksum = CalculateCrc8(tmp, 0, tmp.size());
    tmp.push_back(packetChecksum);

    // check the length
    if (tmp.size() != 27)
        throw Error(format("Internal error CreateRequestInfoPacket: packet size {} != 27", packet.size()));

    // replace special characters
    EscapeData(packet, tmp);

    if (packet.size() > MAX_PACKET_SIZE)
        throw Error(format("Internal error CreateRequestInfoPacket: packet size {} > MAX_PACKET_SIZE {}", packet.size(), MAX_PACKET_SIZE));
}

bool HoymilesHmDtu::QueryInverterInfo(Readings & readings, int numberOfRetries, double waitBeforeRetry)
{
    readings.Clear(_inverterNumberOfChannels);

    if (!_radio)
        throw Error("Communication is not initialized!");

    _radio->flush_tx();
    _radio->flush_rx();

    // set power level to minimum at the end of the function
    OnScopeExit onScopeExit( [&] { _radio->setPALevel(RF24_PA_MIN); } );

    // increase power level
    _radio->setPALevel(RADIO_POWER_LEVEL);

    vector <uint8_t> txPacket;

    for (int retryIndex = 0; retryIndex < numberOfRetries; retryIndex++)
    {
        if (retryIndex > 0)
            this_thread::sleep_for(chrono::milliseconds((int)(waitBeforeRetry * 1000.0)));

        // select a random channel for the request
        int txChannelIndex = _randomTxChannel(_randomEngine);
        int txChannel = TX_CHANNELS.at(txChannelIndex);

        auto rxChannelList = RX_CHANNEL_LISTS.find(txChannel);
        if (rxChannelList == RX_CHANNEL_LISTS.end())
            throw Error(format("Internal error: no RX channels for tx channel {}", txChannel));

        // create packet to send to the inverter
        uint32_t tm = static_cast<uint32_t>(duration_cast<seconds>(system_clock::now().time_since_epoch()).count());

        CreateRequestInfoPacket(txPacket, _inverterRadioAddress, _dtuRadioAddress, tm);
        
        try
        {
            // send request and scan for responses
            responseList = SendRequestAndScanForResponses(radio, txChannel, rxChannelList, txPacket)

            // did we get a valid response?
            success, responseData = EvaluateInverterInfoResponse(responseList, self.__inverterRadioAddress, self.__inverterNumberOfChannels)
            if success:
                success, info = ExtractInverterInfo(responseData, self.__inverterNumberOfChannels)
                if success:
                    return True, info
        }
        catch (const exception & exc)
        {
            // not successful, try again
            LOG_ERROR(exc);
        }       
    }

    return false;
}

