from esphome.components import number
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID
from .. import monoprice_10761_ns, CONF_MONOPRICE_10761_ID, CONF_ZONE, CONF_COMMAND, Monoprice10761

DEPENDENCIES = ["monoprice_10761"]
CODEOWNERS = ["@jhenkens"]

Monoprice10761Number = monoprice_10761_ns.class_("Monoprice10761Number", number.Number, cg.Component)

CONFIG_SCHEMA = number.NUMBER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(Monoprice10761Number),
        cv.GenerateID(CONF_MONOPRICE_10761_ID): cv.use_id(Monoprice10761),
        cv.Required(CONF_ZONE): cv.int_range(min=11, max=36),
        cv.Required(CONF_COMMAND): cv.one_of("VO", "TR", "BS", "BL", upper=True),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    min = 0
    max = 38
    offset = 0
    command = config[CONF_COMMAND]
    if command == "BL":
        min = -10
        max = 10
        offset= -10
    elif command == "TR" or command == "BS":
        offset= -7
        min = -7
        max = 7

    await number.register_number(var, config, min_value=min, max_value=max, step=1)

    paren = await cg.get_variable(config[CONF_MONOPRICE_10761_ID])
    cg.add(var.set_parent(paren))
    cg.add(var.set_zone(config[CONF_ZONE]))
    cg.add(var.set_data_type(config[CONF_COMMAND]))
    cg.add(var.set_offset(offset))
