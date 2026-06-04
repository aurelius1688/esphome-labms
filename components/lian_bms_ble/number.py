import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_ID

from . import CONF_LIAN_BMS_BLE_ID, LianBmsBle, lian_bms_ble_ns

DEPENDENCIES = ["lian_bms_ble"]

CONF_BATTERY_CAPACITY = "battery_capacity"
CONF_CELL_COUNT = "cell_count"

LianBmsBatteryCapacityNumber = lian_bms_ble_ns.class_(
    "LianBmsBatteryCapacityNumber", number.Number, cg.Component
)
LianBmsCellCountNumber = lian_bms_ble_ns.class_(
    "LianBmsCellCountNumber", number.Number, cg.Component
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_LIAN_BMS_BLE_ID): cv.use_id(LianBmsBle),
        cv.Optional(CONF_BATTERY_CAPACITY): number.number_schema(
            LianBmsBatteryCapacityNumber,
            icon="mdi:battery",
        ).extend(
            {
                cv.Optional("initial_value", default=40.0): cv.float_,
                cv.Optional("min_value", default=1.0): cv.float_,
                cv.Optional("max_value", default=1000.0): cv.float_,
                cv.Optional("step", default=1.0): cv.float_,
            }
        ),
        cv.Optional(CONF_CELL_COUNT): number.number_schema(
            LianBmsCellCountNumber,
            icon="mdi:numeric",
        ).extend(
            {
                cv.Optional("initial_value", default=4.0): cv.float_,
                cv.Optional("min_value", default=1.0): cv.float_,
                cv.Optional("max_value", default=32.0): cv.float_,
                cv.Optional("step", default=1.0): cv.float_,
            }
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_LIAN_BMS_BLE_ID])

    if CONF_BATTERY_CAPACITY in config:
        conf = config[CONF_BATTERY_CAPACITY]
        var = await number.new_number(
            conf,
            min_value=conf["min_value"],
            max_value=conf["max_value"],
            step=conf["step"],
        )
        await cg.register_component(var, conf)
        cg.add(var.set_parent(hub))
        cg.add(hub.set_battery_capacity_number(var))

    if CONF_CELL_COUNT in config:
        conf = config[CONF_CELL_COUNT]
        var = await number.new_number(
            conf,
            min_value=conf["min_value"],
            max_value=conf["max_value"],
            step=conf["step"],
        )
        await cg.register_component(var, conf)
        cg.add(var.set_parent(hub))
        cg.add(hub.set_cell_count_number(var))
