import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import CONF_ID

from . import CONF_LIAN_BMS_BLE_ID, LianBmsBle, lian_bms_ble_ns

DEPENDENCIES = ["lian_bms_ble"]

CONF_ONLINE_STATUS = "online_status"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_LIAN_BMS_BLE_ID): cv.use_id(LianBmsBle),
        cv.Optional(CONF_ONLINE_STATUS): binary_sensor.binary_sensor_schema(),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_LIAN_BMS_BLE_ID])

    if CONF_ONLINE_STATUS in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ONLINE_STATUS])
        cg.add(hub.set_online_status_binary_sensor(sens))
