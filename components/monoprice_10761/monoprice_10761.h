/* Copyright (C) 2021 Johan Henkens
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
#include <Arduino.h>

#ifdef ARDUINO_ARCH_ESP8266
#include <ESPAsyncTCP.h>
#else
#include <AsyncTCP.h>
#endif

#define INDEX_TO_ZONE_ID(index) ((((index / 6) + 1) * 10) + (index % 6) + 1)
#define ZONE_ID_TO_INDEX(zone_id) (((zone_id / 10) - 1) * 6 + ((zone_id % 10) - 1))

#define TO_UINT8T(ptr, offset) ((ptr[offset] - '0')*10 + (ptr[offset+1] -'0'))
#define TO_BOOL(ptr, offset) ((bool) (ptr[offset+1] - '0'))

namespace esphome {
namespace monoprice_10761 {

enum class ZoneStatusDataType : uint8_t {
    PA = 0x00,  // PA mode - 00, 01
    PR = 0x01,  // Power - 00, 01
    MU = 0x02,  // Mute - 00, 01
    DT = 0x03,  // Do not disturb - 00, 01
    VO = 0x04,  // Volume - 00-38
    TR = 0x05,  // Trebel - 00-14
    BS = 0x06,  // Bass - 00-14
    BL = 0x07,  // Balance - 00-20
    CH = 0x08,  // Channel - 01-06
    LS = 0x09,  // Keypad status - 00,01

    UNKNOWN = 0x0a // keep it as the max value + 1
};

struct ZoneStatusListener {
    ZoneStatusDataType data_type;
    std::function<void(uint8_t)> on_update;
};

class ZoneStatus{
public:
    ZoneStatus(int zone, const std::function<void(const uint8_t*, size_t)> &send_command){
        this->zone_ = zone;
        this->send_command_ = send_command;
        for(uint8_t i = 0; i < ((uint8_t)ZoneStatusDataType::UNKNOWN); i++){
            this->data_[i] = (uint8_t)0;
        }
    }
    void register_listener(ZoneStatusDataType datapoint_id, const std::function<void(uint8_t)> &func);
    void update(char* zoneStatus);
    void update(ZoneStatusDataType type, uint8_t val);
    void dump();

    unsigned char zone_;
    void set(ZoneStatusDataType cmd, const unsigned char val);
    void get(ZoneStatusDataType type);

    static const char* data_type_to_str(ZoneStatusDataType type){
        switch(type){
            case ZoneStatusDataType::PA:
            return "PA";
            case ZoneStatusDataType::PR:
            return "PR";
            case ZoneStatusDataType::MU:
            return "MU";
            case ZoneStatusDataType::DT:
            return "DT";
            case ZoneStatusDataType::VO:
            return "VO";
            case ZoneStatusDataType::TR:
            return "TR";
            case ZoneStatusDataType::BS:
            return "BS";
            case ZoneStatusDataType::BL:
            return "BL";
            case ZoneStatusDataType::CH:
            return "CH";
            case ZoneStatusDataType::LS:
            return "LS";
            default:
            return "UNKNOWN";
        }
    }

    static ZoneStatusDataType str_to_data_type(const char* data_type){
        uint8_t first = data_type[0];
        uint8_t second = data_type[1];
        first = first > 'Z' ? (first - 'a') + 'A' : first;
        second = second > 'Z' ? (second - 'a') + 'A' : second;
        uint16_t attribute = first << 8 | second;
        switch(attribute){
            case 'P' << 8 | 'A':
            return ZoneStatusDataType::PA;
            case 'P' << 8 | 'R':
            return ZoneStatusDataType::PR;
            case 'M' << 8 | 'U':
            return ZoneStatusDataType::MU;
            case 'D' << 8 | 'T':
            return ZoneStatusDataType::DT;
            case 'V' << 8 | 'O':
            return ZoneStatusDataType::VO;
            case 'T' << 8 | 'R':
            return ZoneStatusDataType::TR;
            case 'B' << 8 | 'S':
            return ZoneStatusDataType::BS;
            case 'B' << 8 | 'L':
            return ZoneStatusDataType::BL;
            case 'C' << 8 | 'H':
            return ZoneStatusDataType::CH;
            case 'L' << 8 | 'S':
            return ZoneStatusDataType::LS;
            default:
            return ZoneStatusDataType::UNKNOWN;
        }
    }

private:
    uint8_t data_[((uint8_t)ZoneStatusDataType::UNKNOWN)];
    std::function<void(const uint8_t*, size_t)> send_command_;
    std::vector<ZoneStatusListener> listeners_;
};

class Monoprice10761 : public esphome::PollingComponent, public uart::UARTDevice  {
public:
    Monoprice10761() : PollingComponent(15000) {}
    explicit Monoprice10761(esphome::uart::UARTComponent *uart) : PollingComponent(15000), uart_{uart} {}

    void setup() override;
    void loop() override;
    void update() override;
    void dump_config() override;
    void on_shutdown() override;

    void write_command(const uint8_t* cmd, size_t len);

    ZoneStatus* get_zone(uint8_t zone_id);

    float get_setup_priority() const override { return esphome::setup_priority::AFTER_WIFI; }

    void set_uart_parent(esphome::uart::UARTComponent *parent) {
        this->uart_ = parent;
        uart::UARTDevice::set_uart_parent(parent);
    }
    void set_port(uint16_t port) { this->port_ = port; }
    void set_expansions(uint16_t expansions) {
        this->zone_count_ = (expansions+1)*6;
        this->zones_ = new ZoneStatus*[this->zone_count_];
        for(unsigned char i = 0; i < this->zone_count_; i++){
          this->zones_[i] = new ZoneStatus(INDEX_TO_ZONE_ID(i), [this](const uint8_t* buf, size_t len){this->write_command(buf,len);});
        }
     }
    void set_input(uint8_t num, const char* name, bool hide){
        this->inputs_[num-1].number = num;
        this->inputs_[num-1].name = std::string(name);
        this->inputs_[num-1].hide = hide;
    }

    struct Input{
        uint8_t number;
        std::string name;
        bool hide;
    };
    Input inputs_[6];

protected:
    void cleanup();
    void read_from_rs232();
    void write_to_rs232();

    struct Client {
        Client(AsyncClient *client, std::vector<uint8_t> &recv_buf);
        ~Client();

        AsyncClient *tcp_client{nullptr};
        std::string identifier{};
        bool disconnected{false};
    };

    unsigned char zone_count_{0};
    unsigned int errors_{0};
    ZoneStatus** zones_{0};
    esphome::uart::UARTComponent *uart_{nullptr};
    AsyncServer server_{0};
    uint16_t port_{4999};
    std::vector<uint8_t> client_recv_buf_{};
    int send_client_{0};
    std::vector<char> serial_read_buf_{};
    std::vector<std::unique_ptr<Client>> clients_{};
private:
    void write_to_clients(const char* buf, size_t len);
};

} // namespace monoprice_10761
} // namespace esphome
