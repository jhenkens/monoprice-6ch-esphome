from esphome.components import binary_sensor
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID
from .. import monoprice_10761_ns, CONF_MONOPRICE_10761_ID, CONF_ZONE, CONF_COMMAND, Monoprice10761

DEPENDENCIES = ["monoprice_10761"]
CODEOWNERS = ["@jhenkens"]

Monoprice10761BinarySensor = monoprice_10761_ns.class_("Monoprice10761BinarySensor", binary_sensor.BinarySensor, cg.Component)

CONFIG_SCHEMA = binary_sensor.BINARY_SENSOR_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(Monoprice10761BinarySensor),
        cv.GenerateID(CONF_MONOPRICE_10761_ID): cv.use_id(Monoprice10761),
        cv.Required(CONF_ZONE): cv.int_range(min=11, max=36),
        cv.Required(CONF_COMMAND): cv.one_of("LS", "PA", upper=True),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await binary_sensor.register_binary_sensor(var, config)

    paren = await cg.get_variable(config[CONF_MONOPRICE_10761_ID])
    cg.add(var.set_parent(paren))
    cg.add(var.set_zone(config[CONF_ZONE]))
    cg.add(var.set_data_type(config[CONF_COMMAND]))
