#pragma once

#include "esphome/core/component.h"
#include "esphome/components/monoprice_10761/monoprice_10761.h"
#include "esphome/components/select/select.h"

namespace esphome {
namespace monoprice_10761 {

class Monoprice10761Select : public select::Select, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void set_parent(Monoprice10761 *monoprice_10761_parent);
  void set_zone(uint8_t zone_id);
  void set_data_type(const char* data_type);

 protected:
  void control(const std::string &value) override;
  ZoneStatus *status_;
  Monoprice10761 *parent_;
  uint8_t zone_id_;
  ZoneStatusDataType data_type_{ZoneStatusDataType::UNKNOWN};
};

}  // namespace monoprice_10761
}  // namespace esphome
