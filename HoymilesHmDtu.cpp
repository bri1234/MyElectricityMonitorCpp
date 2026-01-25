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
using namespace Utils;
using namespace std::chrono;

const std::vector<int> HoymilesHmDtu::TX_CHANNELS = { 3, 23, 40, 61, 75 };

const std::map <int, std::vector <int>> HoymilesHmDtu::RX_CHANNEL_LISTS = {
    {  3, { 23, 40, 61 }},
    { 23, { 40, 61, 75 }},
    { 40, { 61, 75,  3 }},
    { 61, { 75,  3, 23 }},
    { 75, {  3, 23, 40 }},
};

HoymilesHmDtu::ChannelReadings::ChannelReadings()
: _channelNumber(0)
{
}

HoymilesHmDtu::ChannelReadings::ChannelReadings(int channelNumber)
: _channelNumber(channelNumber)
{
}

void HoymilesHmDtu::ChannelReadings::Print(std::ostream &os) const
{
    os << "*** Channel " << _channelNumber << " ***" << endl;
    os << "    DC Voltage:   " << _dcVoltage     << " " << UnitDcVoltage << endl;
    os << "    DC Current:   " << _dcCurrent     << " " << UnitDcCurrent << endl;
    os << "    DC Power:     " << _dcPower       << " " << UnitDcPower << endl;
    os << "    Energy total: " << _dcEnergyTotal << " " << UnitDcEnergyTotal << endl;
    os << "    Energy day:   " << _dcEnergyDay   << " " << UnitDcEnergyDay << endl;
}

void HoymilesHmDtu::ChannelReadings::ExtractReadings(const buffer_type & data, int idxV, int idxC, int idxP, int idxEtotal, int idxEday)
{
    _dcVoltage = GetUInt16(data, idxV) / 10.0;              // V
    _dcCurrent = GetUInt16(data, idxC) / 100.0;             // A
    _dcPower = GetUInt16(data, idxP) / 10.0;                // W
    _dcEnergyTotal = GetUInt32(data, idxEtotal) / 1000.0;   // kWh
    _dcEnergyDay = GetUInt16(data, idxEday) / 1.0;          // Wh
}

void HoymilesHmDtu::Readings::Print(std::ostream & os) const
{
    for (const auto & channelReadings : _channelReadingsList)
        channelReadings.Print(os);

    os << "    AC Voltage:        " << _acVoltage       << " " << UnitAcVoltage << endl;
    os << "    AC Current:        " << _acCurrent       << " " << UnitAcCurrent << endl;
    os << "    AC Power:          " << _acPower         << " " << UnitAcPower << endl;
    os << "    AC Frequency:      " << _acFrequency     << " " << UnitAcFrequency << endl;
    os << "    AC Power factor:   " << _acPowerFactor   << " " << UnitAcPowerFactor << endl;
    os << "    AC Reactive power: " << _acReactivePower << " " << UnitAcReactivePower << endl;
    os << "    Temperature:       " << _temperature     << " " << UnitTemperature << endl;
    os << "    EVT:               " << _EVT             << " " << UnitEVT << endl;
}

void HoymilesHmDtu::Readings::ExtractReadings(int numberOfChannels, const buffer_type & data)
{
    if ((numberOfChannels != 1) && (numberOfChannels != 2) && (numberOfChannels != 4))
        throw Error(format("ExtractReadings: Invalid number of channels: {} (valid: 1, 2 or 4)", numberOfChannels));

    _channelReadingsList.resize(numberOfChannels);

    switch (numberOfChannels)
    {
        case 1:
            _channelReadingsList.at(0).ExtractReadings(data, 2, 4, 6, 8, 12);
            ExtractReadings(data, 14, 16, 18, 20, 22, 24, 26, 28);
            break;

        case 2:
            _channelReadingsList.at(0).ExtractReadings(data, 2, 4, 6, 14, 22);
            _channelReadingsList.at(1).ExtractReadings(data, 8, 10, 12, 18, 24);
            ExtractReadings(data, 26, 28, 30, 32, 34, 36, 38, 40);
            break;

        case 4:
            _channelReadingsList.at(0).ExtractReadings(data, 2, 4, 8, 12, 20);
            _channelReadingsList.at(1).ExtractReadings(data, 2, 6, 10, 16, 22);
            _channelReadingsList.at(2).ExtractReadings(data, 24, 26, 30, 34, 42);
            _channelReadingsList.at(3).ExtractReadings(data, 24, 28, 32, 38, 44);
            ExtractReadings(data, 46, 48, 50, 52, 54, 56, 58, 60);
            break;

        default:
            throw Error(format("ExtractReadings: Invalid number of channels {}", numberOfChannels));
    }
}

void HoymilesHmDtu::Readings::ExtractReadings(const buffer_type & data,
    int idxV, int idxF, int idxP, int idxRP, int idxC, int idxPF, int idxT, int idxEVT)
{
    _acVoltage = GetUInt16(data, idxV) / 10.0;          // V
    _acFrequency = GetUInt16(data, idxF) / 100.0;       // Hz
    _acPower = GetUInt16(data, idxP) / 10.0;            // W
    _acReactivePower = GetUInt16(data, idxRP) / 10.0;   // var (W)
    _acCurrent = GetUInt16(data, idxC) / 100.0;         // A
    _acPowerFactor = GetUInt16(data, idxPF) / 1000.0;   // -
    _temperature = GetUInt16(data, idxT) / 10.0;        // °C
    _EVT = GetUInt16(data, idxEVT) / 1.0;               // -
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

std::string HoymilesHmDtu::PrintNrf24l01Info()
{
    AssertCommunicationIsInitialized();

    vector <char> buffer(1024, '\0');
    _radio->sprintfPrettyDetails(&(buffer[0]));

    return string(&(buffer[0]));
}

HoymilesHmDtu::buffer_type HoymilesHmDtu::GenerateDtuRadioAddress()
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

    buffer_type bytes;
    UInt32ToBytes(bytes, id, true);

    return bytes;
}

HoymilesHmDtu::buffer_type HoymilesHmDtu::GetInverterRadioAddress(const std::string & inverterSerialNumber)
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

void HoymilesHmDtu::EscapeData(buffer_type & dest, const buffer_type & src)
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

void HoymilesHmDtu::UnescapeData(buffer_type & dest, const buffer_type & src)
{
    dest.clear();
    dest.reserve(src.size() + 16);

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

uint8_t HoymilesHmDtu::CalculateCrc8(const buffer_type & data, size_t startPos, size_t endPos)
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

uint16_t HoymilesHmDtu::CalculateCrc16(const buffer_type & data, size_t startPos, size_t endPos)
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

bool HoymilesHmDtu::CheckPacketChecksum(const buffer_type & packet)
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

void HoymilesHmDtu::CreatePacketHeader(buffer_type & packetHeader, uint8_t command, const buffer_type & receiverAddr,
    const buffer_type & senderAddr, uint8_t frame)
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

void HoymilesHmDtu::CreateRequestInfoPayload(buffer_type & payload, uint32_t currentTime)
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

    payload.push_back(0x05);
    
    payload.push_back(0x00);
    payload.push_back(0x00);
    payload.push_back(0x00);
    payload.push_back(0x00);
    
    if (payload.size() - sz != 14)
        throw Error(format("Internal error CreateRequestInfoPayload: size {} != 14", payload.size()));
}

void HoymilesHmDtu::CreateRequestInfoPacket(buffer_type & packet, const buffer_type & receiverAddr,
    const buffer_type & senderAddr, uint32_t currentTime)
{
    packet.clear();
    packet.reserve(MAX_PACKET_SIZE);

    vector<uint8_t> tmpPacket;
    tmpPacket.reserve(MAX_PACKET_SIZE);
    
    // add the header
    CreatePacketHeader(tmpPacket, 0x15, receiverAddr, senderAddr, 0x80);

    // add the payload
    size_t payloadStartPos = tmpPacket.size();
    CreateRequestInfoPayload(tmpPacket, currentTime);

    // add the payload checksum
    uint16_t payloadChecksum = CalculateCrc16(tmpPacket, payloadStartPos, tmpPacket.size());
    UInt16ToBytes(tmpPacket, payloadChecksum, true);

    // add the packet checksum
    uint8_t packetChecksum = CalculateCrc8(tmpPacket, 0, tmpPacket.size());
    tmpPacket.push_back(packetChecksum);

    // check the length
    if (tmpPacket.size() != 27)
        throw Error(format("Internal error CreateRequestInfoPacket: packet size {} != 27", packet.size()));

    // replace special characters
    EscapeData(packet, tmpPacket);

    if (packet.size() > MAX_PACKET_SIZE)
        throw Error(format("Internal error CreateRequestInfoPacket: packet size {} > MAX_PACKET_SIZE {}", packet.size(), MAX_PACKET_SIZE));
}

void HoymilesHmDtu::SendRequestAndScanForResponses(std::vector <buffer_type> & responsePacketList,
    int txChannel, const std::vector <int> & rxChannelList, const buffer_type & txPacket)
{
    responsePacketList.clear();

    AssertCommunicationIsInitialized();

    if (txPacket.size() > MAX_PACKET_SIZE)
        throw Error(format("SendRequestAndScanForResponses: packet size {} > MAX_PACKET_SIZE {}", txPacket.size(), MAX_PACKET_SIZE));

    uint32_t rxChannelIndex = 0;

    buffer_type packet;
    packet.reserve(MAX_PACKET_SIZE);

    // send request to the inverter
    _radio->stopListening();

    _radio->flush_rx();
    _radio->flush_tx();

    _radio->setChannel(txChannel);
     this_thread::sleep_for(microseconds(150));

    _radio->write(&(txPacket[0]), (uint8_t)txPacket.size());
    
    // scan channels for response from the inverter
    _radio->startListening();
    
    // all inverter responses should be received within 500 ms
    constexpr int maxScanTimeMs = 500;

    auto startTime1 = steady_clock::now();
    auto endTime1 = startTime1 + milliseconds(maxScanTimeMs);
    while (steady_clock::now() < endTime1)
    {
        int rxChannel = rxChannelList[rxChannelIndex];
        rxChannelIndex++;
        if (rxChannelIndex >= rxChannelList.size())
            rxChannelIndex = 0;

        // set new receive channel
        _radio->setChannel(rxChannel);

        // wait until the channel is set
        _radio->getChannel();

        // wait for signal
        bool signalDetected = false;

        for (int i = 0; i < 10; i++)
        {
            if (_radio->testRPD() || _radio->available()) // is "_radio->available()" wise???
            {
                signalDetected = true;
                break;
            }
        }

        if (!signalDetected)
            continue;

        // read packets on this channel
        constexpr int maxScanTimePerPacketMs = 10;

        auto startTime2 = steady_clock::now();
        auto endTime2 = startTime2 + milliseconds(maxScanTimePerPacketMs);
        while (steady_clock::now() < endTime2)
        {
            if (!_radio->available())
                continue;
            
            // auto rxtm = duration_cast<milliseconds>(steady_clock::now() - startTime1).count();
            // cout << "      rxtm " << rxtm << " ms";

            // read packet data
            uint8_t packetLen = _radio->getDynamicPayloadSize();
            // cout << "~~~ radio available rx channel " << rxChannel << " packet len " << (int)packetLen << endl;

            packet.resize(packetLen);
            _radio->read(&(packet[0]), packetLen);

            _radio->flush_rx();

            // store raw packet data
            responsePacketList.push_back(packet);
        }
    }
}

bool HoymilesHmDtu::EvaluateInverterInfoResponse(buffer_type & responseData, const std::vector<buffer_type> & responsePacketList,
    const buffer_type & inverterRadioAddress, int inverterNumberOfChannels)
{
    responseData.clear();
    int numberOfResponses = inverterNumberOfChannels + 1;

    // did we get the right number of responses?
    if ((int)responsePacketList.size() != numberOfResponses)
        return false;

    for (int idx = 0; idx < numberOfResponses; idx++)
    {
        const auto & response = responsePacketList[idx];

        if (response.size() < 12)
            return false;

        // are the frame numbers valid?
        int frameNumberResponse = response[9];
        int frameNumberExpected = idx + 1;
        if (frameNumberExpected == numberOfResponses) // is it the last frame?
            frameNumberExpected |= 0x80;

        if (frameNumberResponse != frameNumberExpected)
            return false;

        // are the receiver addresses valid?
        if (!equal(inverterRadioAddress.begin(),  inverterRadioAddress.end(), response.begin() + 1))
            return false;

        if (!equal(inverterRadioAddress.begin(),  inverterRadioAddress.end(), response.begin() + 5))
            return false;

        // is the checksum valid?
        if (!CheckPacketChecksum(response))
            return false;
        
        // header is 10 bytes and last byte is the checksum
        responseData.insert(responseData.end(), response.begin() + 10, response.end() - 1);
    }

    return true;
}

bool HoymilesHmDtu::ExtractInverterReadings(Readings & readings, const buffer_type & responseData, int numberOfChannels)
{
    // check the checksum
    uint16_t crc1 = GetUInt16(responseData, responseData.size() - 2);
    uint16_t crc2 = CalculateCrc16(responseData, 0, responseData.size() - 2);
    if (crc1 != crc2)
        return false;
    
    try
    {
        readings.ExtractReadings(numberOfChannels, responseData);
    }
    catch (const exception &)
    {
        return false;
    }

    return true;
}

void HoymilesHmDtu::UnescapedPacketList(std::vector <buffer_type> & dest, const std::vector <buffer_type> & src)
{
    dest.clear();
    dest.reserve(src.size());

    buffer_type unescapedPacket;

    for (const auto & packet : src)
    {
        UnescapeData(unescapedPacket, packet);
        dest.push_back(unescapedPacket);
    }
}

bool HoymilesHmDtu::QueryInverterInfo(Readings & readings, int numberOfRetries, double waitBeforeRetry)
{
    AssertCommunicationIsInitialized();

    _radio->flush_tx();
    _radio->flush_rx();

    // set power level to minimum at the end of the function
    OnScopeExit onScopeExit( [&] { _radio->setPALevel(RF24_PA_MIN); } );

    // increase power level
    _radio->setPALevel(RADIO_POWER_LEVEL);

    vector <uint8_t> txPacket;
    vector <buffer_type> responsePacketList, unescapedPacketList;
    buffer_type responseData;

    for (int retryIndex = 0; retryIndex < numberOfRetries; retryIndex++)
    {
        if (retryIndex > 0)
            this_thread::sleep_for(milliseconds((int)(waitBeforeRetry * 1000.0)));

        // create packet to send to the inverter
        uint32_t tm = static_cast<uint32_t>(duration_cast<seconds>(system_clock::now().time_since_epoch()).count());

        CreateRequestInfoPacket(txPacket, _inverterRadioAddress, _dtuRadioAddress, tm);
        
        // select a random channel for the request
        int txChannelIndex = _randomTxChannel(_randomEngine);
        int txChannel = TX_CHANNELS.at(txChannelIndex);

        auto rxChannelList = RX_CHANNEL_LISTS.find(txChannel);
        if (rxChannelList == RX_CHANNEL_LISTS.end())
            throw Error(format("Internal error: no RX channels for tx channel {}", txChannel));

        try
        {
            // send request and scan for responses
            SendRequestAndScanForResponses(responsePacketList, txChannel, rxChannelList->second, txPacket);

            // undo replace of special characters
            UnescapedPacketList(unescapedPacketList, responsePacketList);

            // did we get a valid response?
            bool success = EvaluateInverterInfoResponse(responseData, unescapedPacketList, _inverterRadioAddress, _inverterNumberOfChannels);
            if (success)
            {
                success = ExtractInverterReadings(readings, responseData, _inverterNumberOfChannels);
                if (success)
                    return true;
            }
        }
        catch (const exception & exc)
        {
            // not successful, try again
            LOG_ERROR(exc);
        }       
    }

    return false;
}

void HoymilesHmDtu::TestInverterCommunication()
{
    AssertCommunicationIsInitialized();

    _radio->flush_tx();
    _radio->flush_rx();

    // set power level to minimum at the end of the function
    OnScopeExit onScopeExit( [&] { _radio->setPALevel(RF24_PA_MIN); } );

    // increase power level
    _radio->setPALevel(RADIO_POWER_LEVEL);

    vector <uint8_t> txPacket;
    vector <buffer_type> responsePacketList, unescapedPacketList;
    buffer_type responseData;

    // create packet to send to the inverter
    uint32_t tm = static_cast<uint32_t>(duration_cast<seconds>(system_clock::now().time_since_epoch()).count());

    CreateRequestInfoPacket(txPacket, _inverterRadioAddress, _dtuRadioAddress, tm);
    
    try
    {
        for (int txChannel : TX_CHANNELS)
        {
            // *** select a TX channel for the request from channels: 3, 23, 40, 61, 75 ***
            // int txChannel = 3;

            cout << "***** Using TX channel: " << txChannel << " *****" << endl;

            auto rxChannelList = RX_CHANNEL_LISTS.find(txChannel);
            if (rxChannelList == RX_CHANNEL_LISTS.end())
                throw Error(format("Internal error: no RX channels for tx channel {}", txChannel));

            vector <size_t> rxPacketsCounts;
            
            for (int retries = 0; retries < 20; retries++)
            {
                SendRequestAndScanForResponses(responsePacketList, txChannel, rxChannelList->second, txPacket);
                rxPacketsCounts.push_back(responsePacketList.size());

                if (responsePacketList.size() > 0)
                {
                    cout << "      retry " << retries << "\t";

                    // which packets were received?
                    try
                    {
                        UnescapedPacketList(unescapedPacketList, responsePacketList);

                        cout << " Frames: ";
                        for (const auto & packet : unescapedPacketList)
                        {
                            cout << (int)(packet[9] & 0x7F) << " ";
                        }
                        cout << endl;
                    }
                    catch (const exception & exc)
                    {
                        cout << "         " << exc.what() << endl;
                    }
                }
            }

            double sum = 0.0;
            int count1 = 0;
            int count2 = 0;
            int count3 = 0;

            for (auto packetCount : rxPacketsCounts)
            {
                sum += packetCount;
                if (packetCount == 1)
                    count1++;
                if (packetCount == 2)
                    count2++;
                if (packetCount == 3)
                    count3++;
            }
            double avg = sum / rxPacketsCounts.size();

            cout << "      Avg rx packets count: " << avg << "\tRcv #1: " << count1 << "\tRcv #2: " << count2 << "\tRcv #3: " << count3 << endl;
        }
    }
    catch (const exception & exc)
    {
        // not successful, try again
        LOG_ERROR(exc);
    }       
}

