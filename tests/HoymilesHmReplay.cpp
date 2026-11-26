#include "HoymilesHmDtu.h"
#include <iostream>
#include <numeric>
#include <sstream>

using Packet = RF24::Packet;

static void Check(bool condition, const char * message)
{
    if (!condition)
        throw std::runtime_error(message);
}

static Packet Hex(const char * text)
{
    std::istringstream input(text);
    Packet result;
    unsigned value;
    while (input >> std::hex >> value)
        result.push_back(value);
    return result;
}

static uint8_t Checksum(const Packet & packet)
{
    return std::accumulate(packet.begin(), packet.end() - 1, uint8_t{}, std::bit_xor<>());
}

static uint16_t Crc16(const Packet & data, size_t start, size_t end)
{
    uint16_t crc = 0xffff;
    for (size_t i = start; i < end; ++i)
    {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xa001 : 0);
    }
    return crc;
}

int main()
{
    // First transaction in lumapu/ahoy, tools/rpi/example-logs/example.log.
    Packet a = Hex("95 72 22 01 43 72 22 01 43 01 00 01 01 38 00 03 00 0a 01 39 00 04 00 0d 00 00 94");
    Packet b = Hex("95 72 22 01 43 72 22 01 43 02 8f ad 00 00 86 a6 06 29 06 37 08 dd 13 88 00 00 c5");
    Packet c = Hex("95 72 22 01 43 72 22 01 43 83 00 00 00 00 00 00 00 b8 0a b5 5f ff b1");
    HoymilesHmDtu monitor("114172220143");
    monitor.InitializeCommunication();
    HoymilesHmDtu::Readings readings;
    auto query = [&] { return monitor.QueryInverterInfo(readings, 1, 0); };

    auto replay = [&](std::deque<Packet> packets) {
        RF24::onWrite = [packets](const Packet & tx, uint8_t) {
            Check(tx.size() == 27 && tx[0] == 0x15 && tx[9] == 0x80 && tx[10] == 0x0b, "request header");
            Check(Checksum(tx) == tx.back() && Crc16(tx, 10, 24) == (tx[24] << 8 | tx[25]), "TX checksums");
            RF24::fifo = packets;
            return true;
        };
    };

    for (auto packets : {std::deque<Packet>{a,b,c}, {c,a,b}, {a,a,b,c}})
    {
        replay(packets);
        Check(query() && readings.GetAcVoltage() == 226.9 && readings.GetTemperature() == 18.4, "capture");
        Check(readings.GetChannelReadings(0).GetChannelNumber() == 1 &&
              readings.GetChannelReadings(1).GetChannelNumber() == 2, "PV numbering");
    }

    // A fresh request gets only a partial reply. The second TX must target the gap.
    for (uint8_t requested : {0x82, 0xff})
    {
        int calls = 0;
        RF24::onWrite = [&](const Packet & tx, uint8_t) {
            if (++calls == 1)
                RF24::fifo = requested == 0x82 ? std::deque<Packet>{a,c} : std::deque<Packet>{a,b};
            else
            {
                Check(tx.size() == 11 && tx[9] == requested && Checksum(tx) == tx.back(), "repair request");
                RF24::fifo = {requested == 0x82 ? b : c};
            }
            return true;
        };
        Check(query() && calls == 2, "repair failed");
    }

    // End fragment contains reactive power, temperature, event count and CRC16.
    Packet signedEnd = c;
    signedEnd[10] = 0xff; signedEnd[11] = 0x7d; // -13.1 var, including a literal escape byte
    signedEnd[16] = 0xff; signedEnd[17] = 0xf6; // -1.0 C
    Packet data;
    for (const Packet * part : {&a, &b, &signedEnd})
        data.insert(data.end(), part->begin() + 10, part->end() - 1);
    auto crc = Crc16(data, 0, data.size() - 2);
    signedEnd[20] = crc >> 8; signedEnd[21] = crc & 0xff;
    signedEnd.back() = Checksum(signedEnd);
    replay({a,b,signedEnd});
    Check(query() && readings.GetTemperature() == -1.0 && readings.GetAcReactivePower() == -13.1, "signed fields");

    Packet damaged = c;
    damaged[20] ^= 1;
    damaged.back() = Checksum(damaged);
    replay({a,b,damaged});
    Check(!query() && readings.GetTemperature() == -1.0, "failed CRC changed readings");

    // CRC-valid but too short to contain measurements.
    Packet shortEnd = Hex("95 72 22 01 43 72 22 01 43 81 ff ff 00");
    shortEnd.back() = Checksum(shortEnd);
    replay({shortEnd});
    Check(!query() && readings.GetTemperature() == -1.0, "truncated dataset");

    Packet wrongCommand = a, foreign = b, badChecksum = c;
    wrongCommand[0] = 0x96; wrongCommand.back() = Checksum(wrongCommand);
    foreign[1] ^= 1; foreign.back() = Checksum(foreign);
    badChecksum.back() ^= 1;
    replay({wrongCommand, foreign, badChecksum, a, b, c});
    Check(query(), "noise before valid fragments");

    int fullRequests = 0;
    bool requestedFirstAgain = false;
    RF24::onWrite = [&](const Packet & tx, uint8_t) {
        if (tx[9] == 0x80)
        {
            if (++fullRequests == 1) RF24::fifo = {a};
            else RF24::fifo = {b,c}; // new request must ask again for frame 1
        }
        else if (fullRequests == 2 && tx[9] == 0x81)
        {
            requestedFirstAgain = true;
            RF24::fifo = {a};
        }
        return true;
    };
    Check(monitor.QueryInverterInfo(readings, 2, 0) && fullRequests == 2 && requestedFirstAgain, "retry isolation");

    monitor.InitializeCommunication();
    std::vector<uint8_t> tried;
    RF24::onWrite = [&](const Packet &, uint8_t channel) {
        tried.push_back(channel);
        RF24::fifo = {a,b,c}; // request arrived but its radio ACK was lost
        return false;
    };
    Check(query(), "lost ACK prevented reception");
    Check(tried.front() == 3 && std::find(tried.begin(), tried.end(), 23) != tried.end(), "channel search");
    std::cout << "HM-600 replay passed\n";
}
