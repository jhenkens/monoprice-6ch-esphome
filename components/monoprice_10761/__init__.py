from esphome.components import time
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_PORT, CONF_NUMBER, CONF_NAME

DEPENDENCIES = ["uart"]

CONF_EXPANSIONS = "expansions"
CONF_ZONE = "zone"
CONF_COMMAND = "command"
CONF_INPUTS = "inputs"
CONF_HIDDEN = "hide"

monoprice_10761_ns = cg.esphome_ns.namespace("monoprice_10761")
Monoprice10761 = monoprice_10761_ns.class_("Monoprice10761", cg.Component, uart.UARTDevice, cg.PollingComponent)

INPUT_SCHEMA = (cv.Schema(
    {
        cv.Required(CONF_NUMBER): cv.int_range(min=1, max=6),
        cv.Optional(CONF_NAME): cv.string,
        cv.Optional(CONF_HIDDEN, default=False): cv.boolean,
    }
))

CONF_MONOPRICE_10761_ID = "monoprice_10761_id"
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Monoprice10761),
            cv.Optional(CONF_PORT, default=4999): cv.port,
            cv.Optional(CONF_EXPANSIONS, default=0): cv.int_range(
                min=0, max=2
            ),
            cv.Optional(CONF_INPUTS): cv.All(cv.ensure_list(INPUT_SCHEMA), cv.Length(min=1,max=6))
        }
    )
    .extend(cv.polling_component_schema("15s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_port(config[CONF_PORT]))
    cg.add(var.set_expansions(config[CONF_EXPANSIONS]))
    inputs = [{'num': i, 'name': "Input " + str(i), 'hide': False} for i in range(1,7)]
    if CONF_INPUTS in config:
        for input_config in config.get(CONF_INPUTS, []):
            input = inputs[input_config[CONF_NUMBER]-1]
            if CONF_NAME in input_config:
                name = input_config[CONF_NAME]
                if name is not None:
                    input['name'] = name
            input['hide'] = input[CONF_HIDDEN]
    for input in inputs:
        cg.add(var.set_input(input['num'], input['name'], input['hide']))

    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
