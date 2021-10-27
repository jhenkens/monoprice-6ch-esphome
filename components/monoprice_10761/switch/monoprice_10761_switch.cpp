#include "esphome/core/log.h"
#include "monoprice_10761_switch.h"

namespace esphome {
namespace monoprice_10761 {

static const char *const TAG = "monoprice_10761.switch";

void Monoprice10761Switch::setup() {
    this->status_ = this->parent_->get_zone(this->zone_id_);
    this->status_->register_listener(this->data_type_, [this](const uint8_t data) {
        this->publish_state((bool) data);
    });
}

void Monoprice10761Switch::write_state(bool state) {
    ESP_LOGV(TAG, "Setting zone %u : %s", this->status_->zone, ONOFF(state));
    this->status_->set(this->data_type_, (uint8_t) state);
    this->status_->update(this->data_type_, (uint8_t) state);
}

void Monoprice10761Switch::dump_config() {
    LOG_SWITCH("", "Monoprice10761 Switch", this);
    ESP_LOGCONFIG(TAG, "  Switch has zone ID %u and type %s", this->status_->zone_, ZoneStatus::data_type_to_str(this->data_type_));
}

void Monoprice10761Switch::set_parent(Monoprice10761 *monoprice_10761_parent){
    this->parent_ = monoprice_10761_parent;
}

void Monoprice10761Switch::set_zone(uint8_t zone_id){
    this->zone_id_ = zone_id;
}

void Monoprice10761Switch::set_data_type(const char* data_type_str){
    ZoneStatusDataType type = ZoneStatus::str_to_data_type(data_type_str);
    if(type == ZoneStatusDataType::UNKNOWN){
        ESP_LOGE(TAG, "UNKNOWN DATA TYPE: %s", data_type_str);
    }
    this->data_type_ = type;
}

}  // namespace monoprice_10761
}  // namespace esphome
