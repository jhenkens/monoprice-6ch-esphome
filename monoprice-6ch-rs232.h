/* Copyright (C) 2020-2021 Oxan van Leeuwen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

#include <memory>
#include <string>
#include <vector>
#include <Stream.h>

#ifdef ARDUINO_ARCH_ESP8266
#include <ESPAsyncTCP.h>
#else
#include <AsyncTCP.h>
#endif
#define TO_CHAR(ptr, offset) ((ptr[offset] - '0')*10 + (ptr[offset+1] -'0'))
#define TO_BOOL(ptr, offset) ((bool) (ptr[offset+1] - '0'))

class Monoprice6chRs232Component : public esphome::Component {
public:
    Monoprice6chRs232Component() = default;
    explicit Monoprice6chRs232Component(unsigned char zones, Stream *stream) : stream_{stream} {
      this->zones_ = new ZoneStatus*[zones];
      for(unsigned char i = 0; i < zones; i++){
        this->zones_[i] = new ZoneStatus(i);
      }
      this->zone_count_ = zones;
    }
    void set_uart_parent(esphome::uart::UARTComponent *parent) { this->stream_ = parent; }

    void setup() override;
    void loop() override;
    void dump_config() override;
    void on_shutdown() override;
    void query_zones();

    float get_setup_priority() const override { return esphome::setup_priority::AFTER_WIFI; }

    void set_port(uint16_t port) { this->port_ = port; }

protected:
    void cleanup();
    void read();
    void write();

    struct Client {
        Client(AsyncClient *client, std::vector<uint8_t> &recv_buf);
        ~Client();

        AsyncClient *tcp_client{nullptr};
        std::string identifier{};
        bool disconnected{false};
    };

    class ZoneStatus{
    public:
        ZoneStatus(int zone){
            this->zone = ((zone % 6) + 1) * 10 + // expansion
                          (zone + 1); // zone
        }
        void update(char* zoneStatus);
        void dump();
        unsigned char zone;
        bool pa{0};
        bool pr{0};
        bool mu{0};
        bool dt{0};
        unsigned char vo{0}; // vol - 0-38
        unsigned char tr{0}; // trebel - 0-14
        unsigned char bs{0}; // bass - 0-14
        unsigned char bl{0}; // balance - 0-20
        unsigned char ch{0}; // channel - 0-60
        bool ls{0};
    };

    unsigned int errors_{0};
    unsigned char zone_count_{0};
    ZoneStatus** zones_{0};
    Stream *stream_{nullptr};
    AsyncServer server_{0};
    uint16_t port_{4999};
    std::vector<uint8_t> client_recv_buf_{};
    std::vector<char> serial_read_buf_{};
    std::vector<std::unique_ptr<Client>> clients_{};
private:
    void write_to_clients(const char* buf, size_t len);
};
