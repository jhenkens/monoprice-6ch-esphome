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

#include "monoprice_10761.h"

#include "esphome/core/log.h"
#include "esphome/core/util.h"

static const char *TAG = "monoprice_10761";
static const int BUF_SIZE = 128;

using namespace esphome;
using namespace monoprice_10761;

void Monoprice10761::setup() {
    this->serial_read_buf_.reserve(BUF_SIZE);

    ESP_LOGCONFIG(TAG, "Setting up stream server for external apps on port %d...", this->port_);
    this->client_recv_buf_.reserve(BUF_SIZE);

    this->server_ = AsyncServer(this->port_);
    this->server_.begin();
    this->server_.onClient([this](void *h, AsyncClient *tcpClient) {
        if(tcpClient == nullptr)
            return;

        this->clients_.push_back(std::unique_ptr<Client>(new Client(tcpClient, this->client_recv_buf_)));
    }, this);
    this->update();
}

void Monoprice10761::loop() {
    this->cleanup();
    this->read_from_rs232();
    this->write_to_rs232();
}

void Monoprice10761::cleanup() {
    int count;

    // find first disconnected, and then rewrite rest to keep order
    // to keep `send_client_` be correct
    for (count = 0; count < this->clients_.size(); ++count) {
        if (this->clients_[count]->disconnected)
            break;
    }

    for (int i = count; i < this->clients_.size(); ++i) {
        auto& client = this->clients_[i];

        if (!client->disconnected) {
            this->clients_[count++].swap(client);
            continue;
        }

        ESP_LOGD(TAG, "Client %s disconnected", this->clients_[i]->identifier.c_str());

        if (this->send_client_ > i) {
            this->send_client_--;
        }
    }

    this->clients_.resize(count);
}

void Monoprice10761::update(){
    uint8_t command[5] = {'?','1','0','\r','\n'};
    for(uint8_t i = 0; i < this->zone_count_ / 6; i++){
        ESP_LOGD(TAG,"Updating expansion %u...", i);
        command[1] = '1' + i;
        this->write_command(command, (uint8_t) 5);
    }
}

ZoneStatus* Monoprice10761::get_zone(uint8_t zone_id){
    uint8_t index = ZONE_ID_TO_INDEX(zone_id);
    if(index >= this->zone_count_){
        ESP_LOGE(TAG,"Index out of range for zone_id %u", zone_id);
        return this->zones_[0];
    }
    return this->zones_[index];
}

void Monoprice10761::read_from_rs232() {
    int len;
    while ((len = this->uart_->available()) > 0) {
        uint8_t buf[len];
        this->uart_->read_array(buf, len);
        for( size_t i = 0; i < len; i++){
            this->serial_read_buf_.push_back(buf[i]);
            if(buf[i] == '#'){ // If we finished a line, flush it
                size_t line_len = this->serial_read_buf_.size();
                if(line_len == 1) continue;

                // // Debug info
                // char debug_buf[line_len-1];
                // memcpy(debug_buf, this->serial_read_buf_.data(), line_len-1);
                // debug_buf[line_len] = 0;
                // ESP_LOGD(TAG, "READ: %s", debug_buf);

                this->write_to_clients(this->serial_read_buf_.data(), line_len);

                this->serial_read_buf_.clear();
            }
        }
    }
}


char* get_command_as_string(const char* buf_input, size_t len_input){
    char* buf = (char*) buf_input;
    size_t len = len_input;
    while(len > 0 &&
        (
            buf[0] == '#' ||
            buf[0] == '\r' ||
            buf[0] == '\n' ||
            buf[0] == ' '
        )
    ){
        buf++;
        len--;
    }
    while(len > 0 &&
        (
            buf[len-1] == '#' ||
            buf[len-1] == '\r' ||
            buf[len-1] == '\n' ||
            buf[len-1] == ' '
        )
    ){
        len--;
    }

    char* result = new char[len+1];
    memcpy(result,buf,len);
    result[len]=0;
    return result;
}


void Monoprice10761::write_to_clients(const char* buf_input, size_t len_input){
    char* c_str = get_command_as_string(buf_input, len_input);
    size_t len = strlen(c_str);

    // ESP_LOGI(TAG,"Read: %s", c_str);

    for (auto const& client : this->clients_){
        client->tcp_client->write(c_str, len);
        client->tcp_client->write("\r\n#");
    }

    if(c_str[0] == '>' || c_str[0] == '<'){
        char _1 = c_str[1];
        char _2 = c_str[2];
        unsigned char zone = ((_1 - '0') * 10) + (_2 - '0');
        ESP_LOGI(TAG,"Found zone status for %i", zone);
        unsigned char zoneIndex = ((_1 - '1') * 6) + (_2-'1');
        if(this->zone_count_ <= zoneIndex){
            ESP_LOGD(TAG, "Zone %i is out of range of configured zone count %i", zone, this->zone_count_);
        } else {
            ZoneStatus* status = this->zones_[zoneIndex];
            if(c_str[0] == '>'){
                status->update(c_str);
            } else{
                ZoneStatusDataType type = ZoneStatus::str_to_data_type(c_str+3);
                if(type == ZoneStatusDataType::UNKNOWN){
                    ESP_LOGE(TAG, "Unknown attribute: %c%c", c_str[3],c_str[4]);
                }else{
                    ESP_LOGD(TAG, "Updating %c%c", c_str[3], c_str[4]);
                    uint8_t data = TO_UINT8T(c_str, 5);
                    status->update(type,data);
                }
            }
            status->dump();
        }
    }
    if(strcmp(c_str,"Command Error.") == 0){
        ESP_LOGE(TAG, "Command error received");
        this->errors_++;
    }
    delete c_str;
}

void Monoprice10761::write_command(const uint8_t* cmd, size_t len){
    this->uart_->write_array(cmd, len);
}

void Monoprice10761::write_to_rs232() {
    size_t len;
    while ((len = this->client_recv_buf_.size()) > 0) {
        this->uart_->write_array(this->client_recv_buf_.data(), len);

        // Debug info
        char debug_buf[len+1];
        memcpy(debug_buf, this->client_recv_buf_.data(), len);
        debug_buf[len] = 0;

        this->client_recv_buf_.erase(this->client_recv_buf_.begin(), this->client_recv_buf_.begin() + len);
    }
}

void Monoprice10761::dump_config() {
    ESP_LOGCONFIG(TAG, "Monoprice 10761 Stream Server:");
    ESP_LOGCONFIG(TAG, "  Port: %u", this->port_);
    this->check_uart_settings(9600);
}

void Monoprice10761::on_shutdown() {
    for (auto &client : this->clients_)
    client->tcp_client->close(true);
}

Monoprice10761::Client::Client(AsyncClient *client, std::vector<uint8_t> &recv_buf) :
tcp_client{client}, identifier{client->remoteIP().toString().c_str()}, disconnected{false} {
    ESP_LOGD(TAG, "New client connected from %s", this->identifier.c_str());

    this->tcp_client->onError(     [this](void *h, AsyncClient *client, int8_t error)  { this->disconnected = true; });
    this->tcp_client->onDisconnect([this](void *h, AsyncClient *client)                { this->disconnected = true; });
    this->tcp_client->onTimeout(   [this](void *h, AsyncClient *client, uint32_t time) { this->disconnected = true; });

    this->tcp_client->onData([&](void *h, AsyncClient *client, void *data, size_t len) {
        if (len == 0 || data == nullptr)
        return;

        auto buf = static_cast<uint8_t *>(data);
        recv_buf.insert(recv_buf.end(), buf, buf + len);
    }, nullptr);
}

Monoprice10761::Client::~Client() {
    delete this->tcp_client;
}

void ZoneStatus::update(char* zoneStatus){
    const unsigned char base = 3;
    this->update(ZoneStatusDataType::PA, TO_BOOL(zoneStatus, base+0));
    this->update(ZoneStatusDataType::PR, TO_BOOL(zoneStatus, base+2));
    this->update(ZoneStatusDataType::MU, TO_BOOL(zoneStatus, base+4));
    this->update(ZoneStatusDataType::DT, TO_BOOL(zoneStatus, base+6));
    this->update(ZoneStatusDataType::VO, TO_UINT8T(zoneStatus, base+8));
    this->update(ZoneStatusDataType::TR, TO_UINT8T(zoneStatus, base+10));
    this->update(ZoneStatusDataType::BS, TO_UINT8T(zoneStatus, base+12));
    this->update(ZoneStatusDataType::BL, TO_UINT8T(zoneStatus, base+14));
    this->update(ZoneStatusDataType::CH, TO_UINT8T(zoneStatus, base+16));
    this->update(ZoneStatusDataType::LS, TO_BOOL(zoneStatus, base+18));
}

void ZoneStatus::update(ZoneStatusDataType type, uint8_t val){
    const uint8_t type_uint8 = (uint8_t) type;
    if(this->data_[type_uint8] != val){
        this->data_[type_uint8] = val;
        // Run through listeners
        for (auto &listener : this->listeners_){
            if (listener.data_type == type){
                listener.on_update(val);
            }
        }
    }
}

void ZoneStatus::dump(){
    ESP_LOGD(TAG, "ZoneStatus(%u): pa(%u) pr(%u) mu(%u) dt(%u) vo(%u) tr(%u) bs(%u) bl(%u) ch(%u) ls(%u)",
        this->zone_,
        this->data_[(uint8_t)ZoneStatusDataType::PA],
        this->data_[(uint8_t)ZoneStatusDataType::PR],
        this->data_[(uint8_t)ZoneStatusDataType::MU],
        this->data_[(uint8_t)ZoneStatusDataType::DT],
        this->data_[(uint8_t)ZoneStatusDataType::VO],
        this->data_[(uint8_t)ZoneStatusDataType::TR],
        this->data_[(uint8_t)ZoneStatusDataType::BS],
        this->data_[(uint8_t)ZoneStatusDataType::BL],
        this->data_[(uint8_t)ZoneStatusDataType::CH],
        this->data_[(uint8_t)ZoneStatusDataType::LS]
    );
}

void ZoneStatus::set(ZoneStatusDataType type, const unsigned char val){
    const int len = 9;
    uint8_t buf[len];
    buf[0] = '<';
    buf[1] = '0' + this->zone_/10;
    buf[2] = '0' + this->zone_%10;
    const char* type_str = ZoneStatus::data_type_to_str(type);
    buf[3] = type_str[0];
    buf[4] = type_str[1];
    buf[5] = '0' + val / 10;
    buf[6] = '0' + val % 10;
    buf[7] = '\r';
    buf[8] = '\n';
    ESP_LOGD(TAG, "Setting zone %c param %s to %c", this->zone_, type_str, val);
    this->send_command_(buf, len);
}

void ZoneStatus::register_listener(ZoneStatusDataType data_type, const std::function<void(uint8_t)> &func){
    auto listener = ZoneStatusListener{
        .data_type = data_type,
        .on_update = func,
    };
    this->listeners_.push_back(listener);
}
