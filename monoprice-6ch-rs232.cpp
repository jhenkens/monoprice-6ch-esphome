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

#include "monoprice-6ch-rs232.h"

#include "esphome/core/log.h"
#include "esphome/core/util.h"

static const char *TAG = "monoprice-6ch-rs232";

using namespace esphome;

void Monoprice6chRs232Component::setup() {
    this->serial_read_buf_.reserve(128);

    ESP_LOGCONFIG(TAG, "Setting up stream server for external apps on port %d...", this->port_);
    this->client_recv_buf_.reserve(128);

    this->server_ = AsyncServer(this->port_);
    this->server_.begin();
    this->server_.onClient([this](void *h, AsyncClient *tcpClient) {
        if(tcpClient == nullptr)
            return;

        this->clients_.push_back(std::unique_ptr<Client>(new Client(tcpClient, this->client_recv_buf_)));
    }, this);
}

void Monoprice6chRs232Component::loop() {
    this->cleanup();
    this->read();
    this->write();
}

void Monoprice6chRs232Component::cleanup() {
    auto discriminator = [](std::unique_ptr<Client> &client) { return !client->disconnected; };
    auto last_client = std::partition(this->clients_.begin(), this->clients_.end(), discriminator);
    for (auto it = last_client; it != this->clients_.end(); it++)
        ESP_LOGD(TAG, "Client %s disconnected", (*it)->identifier.c_str());

    this->clients_.erase(last_client, this->clients_.end());
}

void Monoprice6chRs232Component::read() {
    int len;
    while ((len = this->stream_->available()) > 0) {
        char buf[128];
        size_t read = this->stream_->readBytes(buf, min(len, 128));
        // ESP_LOGD(TAG, "read_bytes: %d", read);
        for( size_t i = 0; i < read; i++){
            // if(buf[i] == '\r'){
            //     ESP_LOGD(TAG,"Pushing CR");
            // }else if (buf[i] == '\n'){
            //     ESP_LOGD(TAG,"Pushing NL");
            // }else{
            //     ESP_LOGD(TAG,"Pushing %c", buf[i]);
            // }
            this->serial_read_buf_.push_back(buf[i]);
            if(buf[i] == '#'){ // If we finished a line, flush it
                size_t line_len = this->serial_read_buf_.size();
                if(line_len == 1) continue;

                // Debug info
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


void Monoprice6chRs232Component::write_to_clients(const char* buf_input, size_t len_input){
    char* c_str = get_command_as_string(buf_input, len_input);
    size_t len = strlen(c_str);

    ESP_LOGI(TAG,"Read: %s", c_str);

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
                uint16_t attribute = c_str[3] << 8 | c_str[4];
                unsigned char data = TO_CHAR(c_str, 5);
                switch(attribute){
                  case 'P' << 8 | 'A':
                    ESP_LOGD(TAG, "Updating pa");
                    status->pa = (bool) data;
                    break;
                  case 'P' << 8 | 'R':
                    ESP_LOGD(TAG, "Updating pr");
                    status->pr = (bool) data;
                    break;
                  case 'M' << 8 | 'U':
                    ESP_LOGD(TAG, "Updating mu");
                    status->mu = (bool) data;
                    break;
                  case 'D' << 8 | 'T':
                    ESP_LOGD(TAG, "Updating dt");
                    status->dt = (bool) data;
                    break;
                  case 'V' << 8 | 'O':
                    ESP_LOGD(TAG, "Updating vo");
                    status->vo = data;
                    break;
                  case 'T' << 8 | 'R':
                    ESP_LOGD(TAG, "Updating tr");
                    status->tr = data;
                    break;
                  case 'B' << 8 | 'S':
                    ESP_LOGD(TAG, "Updating bs");
                    status->bs = data;
                    break;
                  case 'B' << 8 | 'L':
                    ESP_LOGD(TAG, "Updating bl");
                    status->bl = data;
                    break;
                  case 'C' << 8 | 'H':
                    ESP_LOGD(TAG, "Updating ch");
                    status->ch = data;
                    break;
                  case 'L' << 8 | 'S':
                    ESP_LOGD(TAG, "Updating ls");
                    status->ls = (bool)data;
                    break;
                  default:
                    ESP_LOGE(TAG, "Unknown attribute: %c%c", c_str[3],c_str[4]);
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

void Monoprice6chRs232Component::write() {
    size_t len;
    while ((len = this->client_recv_buf_.size()) > 0) {
        this->stream_->write(this->client_recv_buf_.data(), len);

        // Debug info
        char debug_buf[len+1];
        memcpy(debug_buf, this->client_recv_buf_.data(), len);
        debug_buf[len] = 0;
        ESP_LOGD(TAG, "WRITE: %s", debug_buf);

        this->client_recv_buf_.erase(this->client_recv_buf_.begin(), this->client_recv_buf_.begin() + len);
    }
}

void Monoprice6chRs232Component::dump_config() {
    ESP_LOGCONFIG(TAG, "Monoprice 6ch Stream Server:");
    ESP_LOGCONFIG(TAG, "  Address: %s:%u", network_get_address().c_str(), this->port_);
}

void Monoprice6chRs232Component::on_shutdown() {
    for (auto &client : this->clients_)
        client->tcp_client->close(true);
}

Monoprice6chRs232Component::Client::Client(AsyncClient *client, std::vector<uint8_t> &recv_buf) :
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

Monoprice6chRs232Component::Client::~Client() {
    delete this->tcp_client;
}

void Monoprice6chRs232Component::ZoneStatus::update(char* zoneStatus){
    const unsigned char base = 3;
    this->pa = TO_BOOL(zoneStatus, base+0);
    this->pr = TO_BOOL(zoneStatus, base+2);
    this->mu = TO_BOOL(zoneStatus, base+4);
    this->dt = TO_BOOL(zoneStatus, base+6);
    this->vo = TO_CHAR(zoneStatus, base+8);
    this->tr = TO_CHAR(zoneStatus, base+10);
    this->bs = TO_CHAR(zoneStatus, base+12);
    this->bl = TO_CHAR(zoneStatus, base+14);
    this->ch = TO_CHAR(zoneStatus, base+16);
    this->ls = TO_BOOL(zoneStatus, base+18);
}

void Monoprice6chRs232Component::ZoneStatus::dump(){
    ESP_LOGD(TAG, "ZoneStatus(%u): pa(%u) pr(%u) mu(%u) dt(%u) vo(%u) tr(%u) bs(%u) bl(%u) ch(%u) ls(%u)",
        this->zone,
        this->pa,
        this->pr,
        this->mu,
        this->dt,
        this->vo,
        this->tr,
        this->bs,
        this->bl,
        this->ch,
        this->ls
    );
}
