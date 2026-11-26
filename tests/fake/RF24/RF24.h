#pragma once

#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>
#include <stdexcept>
#include <vector>

// In-memory RF24 interface used only by the replay test target.
enum rf24_pa_dbm_e { RF24_PA_MIN, RF24_PA_LOW };
enum rf24_datarate_e { RF24_250KBPS };
enum rf24_crclength_e { RF24_CRC_16 };

class RF24
{
public:
    using Packet = std::vector<uint8_t>;
    inline static std::function<bool(const Packet &, uint8_t)> onWrite;
    inline static std::deque<Packet> fifo;
    RF24(int, int, int) {}
    bool begin() { return true; }
    bool isChipConnected() { return true; }
    bool setDataRate(rf24_datarate_e) { return true; }
    void setPALevel(rf24_pa_dbm_e) {}
    void setCRCLength(rf24_crclength_e) {}
    void setAddressWidth(int) {}
    void openWritingPipe(const uint8_t *) {}
    void openReadingPipe(int, const uint8_t *) {}
    void enableDynamicPayloads() {}
    void setAutoAck(bool enabled) { autoAck = enabled; }
    void setRetries(int, int count)
    {
        if (count != 0)
            throw std::logic_error("automatic retries enabled");
    }
    void startListening() { listening = true; }
    void stopListening() { listening = false; }
    void setChannel(uint8_t value) { channel = value; }
    bool testRPD() const
    {
        if (!listening || autoAck)
            throw std::logic_error("RPD sampled outside passive RX mode");
        return true;
    }
    void flush_rx() { fifo.clear(); }
    void flush_tx() {}
    bool available() const { return listening && !fifo.empty(); }
    uint8_t getDynamicPayloadSize()
    {
        if (fifo.front().empty() || fifo.front().size() > 32)
        {
            flush_rx();
            return 0;
        }
        return fifo.front().size();
    }
    void read(void * dest, uint8_t length)
    {
        if (!available() || length != fifo.front().size())
            throw std::logic_error("invalid read");
        std::copy_n(fifo.front().begin(), length, static_cast<uint8_t *>(dest));
        fifo.pop_front();
    }
    bool write(const void * src, uint8_t length)
    {
        if (listening || !autoAck)
            throw std::logic_error("write outside acknowledged TX mode");
        const auto * bytes = static_cast<const uint8_t *>(src);
        return onWrite(Packet(bytes, bytes + length), channel);
    }
    void sprintfPrettyDetails(char * buffer) { *buffer = 0; }

private:
    bool listening = false;
    bool autoAck = true;
    uint8_t channel = 0;
};
