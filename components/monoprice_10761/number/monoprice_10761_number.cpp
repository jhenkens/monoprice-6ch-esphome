#include "esphome/core/log.h"
#include "monoprice_10761_number.h"

namespace esphome {
namespace monoprice_10761 {

static const char *const TAG = "monoprice_10761.number";

void Monoprice10761Number::setup() {
    this->status_ = this->parent_->get_zone(this->zone_id_);
    this->status_->register_listener(this->data_type_, [this](const uint8_t data) {
        this->publish_state(data+this->offset_);
    });
}

void Monoprice10761Number::control(float state) {
    this->status_->set(this->data_type_, (uint8_t) (state-this->offset_));
    this->status_->update(this->data_type_, (uint8_t) (state-this->offset_));
}

void Monoprice10761Number::dump_config() {
    LOG_NUMBER("", "Monoprice10761 Number", this);
    ESP_LOGCONFIG(TAG, "  Number has zone ID %u and type %s", this->status_->zone_, ZoneStatus::data_type_to_str(this->data_type_));
}

void Monoprice10761Number::set_parent(Monoprice10761 *monoprice_10761_parent){
    this->parent_ = monoprice_10761_parent;
}

void Monoprice10761Number::set_zone(uint8_t zone_id){
    this->zone_id_ = zone_id;
}

void Monoprice10761Number::set_data_type(const char* data_type_str){
    ZoneStatusDataType type = ZoneStatus::str_to_data_type(data_type_str);
    if(type == ZoneStatusDataType::UNKNOWN){
        ESP_LOGE(TAG, "UNKNOWN DATA TYPE: %s", data_type_str);
    }
    this->data_type_ = type;
}

}  // namespace monoprice_10761
}  // namespace esphome
