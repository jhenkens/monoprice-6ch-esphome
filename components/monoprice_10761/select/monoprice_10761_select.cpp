#include "esphome/core/log.h"
#include "monoprice_10761_select.h"

namespace esphome {
namespace monoprice_10761 {

static const char *const TAG = "monoprice_10761.select";

void Monoprice10761Select::setup() {
    std::vector<std::string> options;
    for(uint8_t i = 0; i < 6; i++){
        if(this->parent_->inputs_[i].hide) continue;
        std::string name = this->parent_->inputs_[i].name;
        options.push_back(name);
    }
    this->traits.set_options(options);
    this->status_ = this->parent_->get_zone(this->zone_id_);
    this->status_->register_listener(this->data_type_, [this](const uint8_t data) {
        if(data < 1 || data > 6){
            ESP_LOGE(TAG,"Input %u is out of range!", data);
        }
        std::string name = this->parent_->inputs_[data-1].name;
        this->publish_state(name);
    });
}

void Monoprice10761Select::control(const std::string &value) {
    for(uint8_t i = 0; i < 6; i++){
        if(value.compare(this->parent_->inputs_[i].name) == 0){
            this->status_->set(this->data_type_, i+1);
            this->status_->update(this->data_type_, i+1);
            return;
        }
    }
    ESP_LOGE(TAG, "Could not find input named %s", value.c_str());
}

void Monoprice10761Select::dump_config() {
    LOG_SELECT("", "Monoprice10761 Select", this);
    ESP_LOGCONFIG(TAG, "  Select has zone ID %u and type %s", this->status_->zone_, ZoneStatus::data_type_to_str(this->data_type_));
}

void Monoprice10761Select::set_parent(Monoprice10761 *monoprice_10761_parent){
    this->parent_ = monoprice_10761_parent;
}

void Monoprice10761Select::set_zone(uint8_t zone_id){
    this->zone_id_ = zone_id;
}

void Monoprice10761Select::set_data_type(const char* data_type_str){
    ZoneStatusDataType type = ZoneStatus::str_to_data_type(data_type_str);
    if(type == ZoneStatusDataType::UNKNOWN){
        ESP_LOGE(TAG, "UNKNOWN DATA TYPE: %s", data_type_str);
    }
    this->data_type_ = type;
}

}  // namespace monoprice_10761
}  // namespace esphome
