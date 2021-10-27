#pragma once

#include "esphome/core/component.h"
#include "esphome/components/monoprice_10761/monoprice_10761.h"
#include "esphome/components/number/number.h"

namespace esphome {
namespace monoprice_10761 {

class Monoprice10761Number : public number::Number, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void set_parent(Monoprice10761 *monoprice_10761_parent);
  void set_zone(uint8_t zone_id);
  void set_data_type(const char* data_type);
  void set_offset(int8_t offset){this->offset_ = offset;}

 protected:
  void control(float state) override;
  ZoneStatus *status_;
  Monoprice10761 *parent_;
  uint8_t zone_id_;
  int8_t offset_{0};
  ZoneStatusDataType data_type_{ZoneStatusDataType::UNKNOWN};
};

}  // namespace monoprice_10761
}  // namespace esphome
