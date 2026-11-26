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
#include "ValueChangeDump.h"
#include <array>
#include <sstream>

#include <unistd.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;
using namespace Utils;
using namespace std::chrono;

namespace
{
constexpr array<uint8_t, 5> Channels{3, 23, 40, 61, 75};
constexpr auto Slot = 2596us;
constexpr auto ChannelTime = 2 * Slot;
constexpr auto SearchTime = 10 * Slot;
// Reacquire after a gap of three host rotations.
constexpr auto SyncLifetime = 3 * SearchTime;

constexpr size_t ChannelIndex(size_t start, steady_clock::duration elapsed, microseconds dwell)
{
    return (start + elapsed / dwell) % Channels.size();
}

static_assert(ChannelIndex(0, 0us, ChannelTime) == 0);
static_assert(ChannelIndex(0, 5191us, ChannelTime) == 0);
static_assert(ChannelIndex(0, 5192us, ChannelTime) == 1);
constexpr array<size_t, 10> HostCycle{0, 0, 1, 1, 2, 2, 3, 3, 4, 4};
static_assert([] {
    for (int slot = 0; slot < 10; ++slot)
        if (ChannelIndex(0, slot * Slot, ChannelTime) != HostCycle[slot])
            return false;
    return ChannelIndex(0, 10 * Slot, ChannelTime) == 0;
}());
static_assert(ChannelIndex(0, 9 * Slot, SearchTime) == 0);
static_assert(ChannelIndex(0, 10 * Slot, SearchTime) == 1);
}

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

    const size_t minimumBytes = numberOfChannels == 1 ? 30 : (numberOfChannels == 2 ? 42 : 62);
    if (data.size() < minimumBytes)
        throw Error("Incomplete realtime dataset");

    _channelReadingsList.clear();
    for (int input = 1; input <= numberOfChannels; ++input)
        _channelReadingsList.emplace_back(input);

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
    _acReactivePower = static_cast<int16_t>(GetUInt16(data, idxRP)) / 10.0;   // var (W)
    _acCurrent = GetUInt16(data, idxC) / 100.0;         // A
    _acPowerFactor = GetUInt16(data, idxPF) / 1000.0;   // -
    _temperature = static_cast<int16_t>(GetUInt16(data, idxT)) / 10.0;        // °C
    _EVT = GetUInt16(data, idxEVT) / 1.0;               // -
}

HoymilesHmDtu::HoymilesHmDtu(const std::string & inverterSerialNumber, int pinCSn, int pinCE)
    : _inverterSerialNumber(inverterSerialNumber)
    , _pinCSn(pinCSn)
    , _pinCE(pinCE)
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

    if (!radio->setDataRate(RF24_250KBPS))
        throw Error("RF24 does not support 250 kbit/s");
    radio->setPALevel(RF24_PA_MIN);
    radio->setCRCLength(RF24_CRC_16);
    radio->setAddressWidth(5);

    radio->openWritingPipe(&(_writingPipeAddress[0]));
    radio->openReadingPipe(RX_PIPE_NUM, &(_readingPipeAddress[0]));

    radio->enableDynamicPayloads();
    radio->setRetries(2, 0); // one transmission per software-timed slot
    radio->setAutoAck(true);

    _sync.reset();
    _searchIndex = 0;
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

    // add the header
    CreatePacketHeader(packet, 0x15, receiverAddr, senderAddr, 0x80);

    // add the payload
    size_t payloadStartPos = packet.size();
    CreateRequestInfoPayload(packet, currentTime);

    // add the payload checksum
    uint16_t payloadChecksum = CalculateCrc16(packet, payloadStartPos, packet.size());
    UInt16ToBytes(packet, payloadChecksum, true);

    // add the packet checksum
    uint8_t packetChecksum = CalculateCrc8(packet, 0, packet.size());
    packet.push_back(packetChecksum);

}

struct HoymilesHmDtu::QueryDiagnostics
{
    ostringstream events;
    unsigned repairs = 0;
    unsigned timeouts = 0;
};

struct HoymilesHmDtu::ResponseFragments
{
    array<buffer_type, 16> parts; // HM uses one-based, four-bit fragment numbers
    unsigned last = 0;
    unsigned highest = 0;
    bool conflict = false;

    bool Add(const buffer_type & packet, const buffer_type & inverter)
    {
        if (packet.size() < 12 || packet[0] != 0x95 || !CheckPacketChecksum(packet) ||
            !equal(inverter.begin(), inverter.end(), packet.begin() + 1) ||
            !equal(inverter.begin(), inverter.end(), packet.begin() + 5))
            return false;

        const unsigned number = packet[9] & 0x7f;
        const bool final = (packet[9] & 0x80) != 0;
        if (number == 0 || number >= parts.size() || (!final && packet.size() != 27))
            return false;

        buffer_type payload(packet.begin() + 10, packet.end() - 1);
        if ((last && (number > last || final != (number == last))) ||
            (final && highest > number) ||
            (!parts[number].empty() && (parts[number] != payload || final != (number == last))))
        {
            conflict = true;
            return false;
        }
        parts[number] = move(payload);
        highest = max(highest, number);
        if (final)
            last = number;
        return true;
    }

    // 0xff requests the last fragment when its number is still unknown.
    uint8_t MissingFrame() const
    {
        if (!last)
            return 0xff;
        for (unsigned number = 1; number <= last; ++number)
            if (parts[number].empty())
                return 0x80 | number;
        return 0;
    }

    buffer_type Assemble() const
    {
        buffer_type data;
        for (unsigned number = 1; number <= last; ++number)
            AppendRange(data, parts[number]);
        return data;
    }
};

HoymilesHmDtu::ChannelPhase HoymilesHmDtu::SendPacket(const buffer_type & packet,
    Clock::time_point deadline, ostream * trace)
{
    _radio->stopListening();
    const auto now = Clock::now();
    if (_sync && now - _sync->slotStart >= SyncLifetime)
        _sync.reset();

    auto firstSlot = now;
    if (_sync)
    {
        const auto elapsed = now - _sync->slotStart;
        firstSlot = _sync->slotStart + (elapsed / ChannelTime + 1) * ChannelTime;
    }

    ChannelPhase phase{_searchIndex, now};
    auto nextSlot = firstSlot;
    auto searchStart = firstSlot;
    unsigned attempts = 0;
    bool ack = false;
    while (nextSlot < deadline && nextSlot < firstSlot + 16 * Slot)
    {
        this_thread::sleep_until(nextSlot);
        const auto start = Clock::now();
        if (start >= deadline || start >= firstSlot + 16 * Slot)
            break;

        if (_sync && start - _sync->slotStart >= SyncLifetime)
        {
            _sync.reset();
            searchStart = start;
        }
        phase.index = _sync ? ChannelIndex(_sync->index, start - _sync->slotStart, ChannelTime)
                            : ChannelIndex(_searchIndex, start - searchStart, SearchTime);
        _radio->setChannel(Channels[phase.index]);
        phase.slotStart = Clock::now();
        ++attempts;
        ack = _radio->write(packet.data(), static_cast<uint8_t>(packet.size()));
        if (ack)
            break;
        // Skip elapsed slots after a scheduling delay instead of sending a burst.
        nextSlot = firstSlot + ((Clock::now() - firstSlot) / Slot + 1) * Slot;
    }
    if (ack)
    {
        _sync = phase;
        _searchIndex = phase.index;
    }
    else
    {
        _sync.reset();
        _searchIndex = (phase.index + 1) % Channels.size();
    }
    if (!attempts)
        _radio->setChannel(Channels[phase.index]);
    _radio->startListening(); // receive even if the request's ACK was lost
    if (trace)
        *trace << " TX(last)=" << +Channels[phase.index] << " attempts=" << attempts << " ACK=" << ack;
    return phase;
}

void HoymilesHmDtu::ReceiveFragments(ResponseFragments & fragments, ChannelPhase phase,
    Clock::time_point deadline, ostream * trace)
{
    size_t listening = phase.index;
    while (Clock::now() < deadline)
    {
        const auto index = ChannelIndex(phase.index, Clock::now() - phase.slotStart, ChannelTime);
        if (index != listening)
        {
            _radio->stopListening();
            _radio->setChannel(Channels[index]);
            _radio->startListening();
            listening = index;
        }
        while (_radio->available() && Clock::now() < deadline)
        {
            const uint8_t length = _radio->getDynamicPayloadSize();
            if (!length || length > MAX_PACKET_SIZE)
            {
                // RF24 discards the corrupt FIFO on an invalid payload width.
                break;
            }
            buffer_type packet(length);
            _radio->read(packet.data(), length);
            if (fragments.Add(packet, _inverterRadioAddress) && trace)
                *trace << " RX=" << +packet[9];
            if (fragments.conflict || fragments.MissingFrame() == 0)
                return;
        }
        this_thread::sleep_for(100us);
    }
}

bool HoymilesHmDtu::ExtractInverterReadings(Readings & readings,
    const buffer_type & responseData, int numberOfChannels)
{
    if (responseData.size() < 2)
        return false;
    const size_t length = responseData.size() - 2;
    if (CalculateCrc16(responseData, 0, length) != GetUInt16(responseData, length))
        return false;

    Readings decoded;
    try
    {
        decoded.ExtractReadings(numberOfChannels, {responseData.begin(), responseData.end() - 2});
    }
    catch (const Error &)
    {
        return false;
    }
    readings = move(decoded);
    return true;
}

bool HoymilesHmDtu::QueryInverterInfo(Readings & readings, int numberOfRetries, double waitBeforeRetry)
{
    return QueryInverterInfo(readings, numberOfRetries, waitBeforeRetry, nullptr);
}

bool HoymilesHmDtu::QueryInverterInfo(Readings & readings, int numberOfRetries,
    double waitBeforeRetry, QueryDiagnostics * diagnostics)
{
    AssertCommunicationIsInitialized();
    ostream * trace = diagnostics ? &diagnostics->events : nullptr;
    OnScopeExit restore([&] {
        _radio->stopListening();
        _radio->setPALevel(RF24_PA_MIN);
    });
    _radio->setPALevel(RADIO_POWER_LEVEL);
    for (int attempt = 0; attempt < numberOfRetries; ++attempt)
    {
        if (attempt)
            this_thread::sleep_for(duration<double>(waitBeforeRetry));
        _radio->stopListening();
        _radio->flush_tx();
        _radio->flush_rx();
        ResponseFragments fragments;
        buffer_type request;
        const auto timestamp = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
        CreateRequestInfoPacket(request, _inverterRadioAddress, _dtuRadioAddress, timestamp);
        const auto deadline = Clock::now() + 1s;
        while (Clock::now() < deadline)
        {
            auto phase = SendPacket(request, deadline, trace);
            ReceiveFragments(fragments, phase, min(deadline, Clock::now() + 150ms), trace);
            if (fragments.conflict)
            {
                if (trace)
                    *trace << " conflicting fragments";
                break;
            }
            if (!fragments.highest)
                break;
            const uint8_t missing = fragments.MissingFrame();
            if (!missing)
            {
                if (ExtractInverterReadings(readings, fragments.Assemble(), _inverterNumberOfChannels))
                {
                    if (trace)
                        *trace << " CRC16=OK";
                    return true;
                }
                if (trace)
                    *trace << " invalid CRC16 or dataset length";
                break;
            }
            if (Clock::now() >= deadline)
                break;
            request.clear();
            CreatePacketHeader(request, 0x15, _inverterRadioAddress, _dtuRadioAddress, missing);
            request.push_back(CalculateCrc8(request, 0, request.size()));
            if (diagnostics)
            {
                ++diagnostics->repairs;
                if (missing == 0xff)
                    *trace << " request-last-fragment";
                else
                    *trace << " request-fragment=" << (missing & 0x7f);
            }
        }
        if (diagnostics && !fragments.conflict && fragments.MissingFrame())
        {
            ++diagnostics->timeouts;
            *trace << " RX timeout";
        }
    }
    return false;
}

int HoymilesHmDtu::TestInverterCommunication()
{
    constexpr int queries = 100;
    int successes = 0;
    unsigned repairs = 0;
    unsigned timeouts = 0;
    for (int query = 1; query <= queries; ++query)
    {
        Readings readings;
        QueryDiagnostics diagnostics;
        const auto start = Clock::now();
        const bool success = QueryInverterInfo(readings, 1, 0, &diagnostics);
        repairs += diagnostics.repairs;
        timeouts += diagnostics.timeouts;
        cout << query << "/" << queries << diagnostics.events.str() << " elapsed="
             << duration_cast<milliseconds>(Clock::now() - start).count() << "ms"
             << (success ? " OK" : " FAILED") << endl;
        if (success)
        {
            ++successes;
            readings.Print(cout);
        }
        if (query < queries)
            this_thread::sleep_for(1s);
    }
    cout << "Complete CRC-verified datasets: " << successes << "/" << queries
         << ", fragment requests: " << repairs << ", RX timeouts: " << timeouts << endl;
    return successes;
}

void HoymilesHmDtu::CaptureRadioActivity(const string & fileName, milliseconds duration)
{
    AssertCommunicationIsInitialized();
    if (duration <= 0ms)
        throw Error("Radio capture duration must be positive");

    ValueChangeDump trace;
    const vector<unsigned int> signals{
        trace.DefineVariable("RX_Channel", "wire", 7),
        trace.DefineVariable("RPD", "wire", 1)
    };
    vector<unsigned int> values(signals.size());
    trace.OpenFile(fileName, "1.0", "Passive nRF24 RPD samples");

    _radio->stopListening();
    _radio->setAutoAck(false);
    OnScopeExit restore([&] {
        _radio->stopListening();
        _radio->flush_rx();
        _radio->setAutoAck(true);
    });
    _sync.reset();

    constexpr auto sampleInterval = 250us;
    const auto deadline = Clock::now() + duration;
    for (size_t index = 0; Clock::now() < deadline; index = (index + 1) % Channels.size())
    {
        const auto channel = Channels[index];
        _radio->stopListening();
        _radio->setChannel(channel);
        _radio->startListening();
        const auto channelDeadline = min(deadline, Clock::now() + ChannelTime);
        values[0] = channel;

        // Allow the receiver to settle before recording the first RPD sample.
        while (Clock::now() < channelDeadline)
        {
            this_thread::sleep_for(sampleInterval);
            if (Clock::now() >= channelDeadline)
                break;
            values[1] = _radio->testRPD();
            trace.LogVariableValue(signals, values);
        }
    }
    trace.CloseFile();
}
