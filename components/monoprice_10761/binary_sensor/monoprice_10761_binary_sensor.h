#pragma once

#include "esphome/core/component.h"
#include "esphome/components/monoprice_10761/monoprice_10761.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace monoprice_10761 {

class Monoprice10761BinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void set_parent(Monoprice10761 *monoprice_10761_parent);
  void set_zone(uint8_t zone_id);
  void set_data_type(const char* data_type);

 protected:
  ZoneStatus *status_;
  Monoprice10761 *parent_;
  uint8_t zone_id_;
  ZoneStatusDataType data_type_{ZoneStatusDataType::UNKNOWN};
};

}  // namespace monoprice_10761
}  // namespace esphome
