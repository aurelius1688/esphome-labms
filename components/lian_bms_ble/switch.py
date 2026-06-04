import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_ID

from . import CONF_LIAN_BMS_BLE_ID, LianBmsBle, lian_bms_ble_ns

DEPENDENCIES = ["lian_bms_ble"]

CONF_CHARGE = "charge"
CONF_DISCHARGE = "discharge"
CONF_HEATING = "heating"

LianBmsSwitch = lian_bms_ble_ns.class_("LianBmsSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_LIAN_BMS_BLE_ID): cv.use_id(LianBmsBle),
        cv.Optional(CONF_CHARGE): switch.switch_schema(
            LianBmsSwitch,
            icon="mdi:battery-charging",
        ),
        cv.Optional(CONF_DISCHARGE): switch.switch_schema(
            LianBmsSwitch,
            icon="mdi:battery-arrow-down",
        ),
        cv.Optional(CONF_HEATING): switch.switch_schema(
            LianBmsSwitch,
            icon="mdi:heating-coil",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_LIAN_BMS_BLE_ID])

    for key, reg_on, reg_off in [
        (CONF_CHARGE, 6, 7),
        (CONF_DISCHARGE, 3, 4),
        (CONF_HEATING, 32, 32),
    ]:
        if key in config:
            conf = config[key]
            var = await switch.new_switch(conf)
            cg.add(var.set_parent(hub))
            cg.add(var.set_reg_on(reg_on))
            cg.add(var.set_reg_off(reg_off))
            if key == CONF_HEATING:
                cg.add(var.set_value_on(1))
                cg.add(var.set_value_off(0))
