import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import CONF_ID

from . import CONF_LIAN_BMS_BLE_ID, LianBmsBle, lian_bms_ble_ns

DEPENDENCIES = ["lian_bms_ble"]

CONF_RESPONSE = "response"
CONF_DEBUG = "debug"
CONF_CONNECTED_TIME_TEXT = "connected_time_text"
CONF_MAC_ADDRESS = "mac_address"
CONF_MAC_INPUT = "mac_input"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_LIAN_BMS_BLE_ID): cv.use_id(LianBmsBle),
        cv.Optional(CONF_RESPONSE): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_DEBUG): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_CONNECTED_TIME_TEXT): text_sensor.text_sensor_schema(
            icon="mdi:timer",
        ),
        cv.Optional(CONF_MAC_ADDRESS): text_sensor.text_sensor_schema(
            icon="mdi:bluetooth",
        ),
        cv.Optional(CONF_MAC_INPUT): text_sensor.text_sensor_schema(
            icon="mdi:bluetooth-connect",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_LIAN_BMS_BLE_ID])
    if CONF_RESPONSE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_RESPONSE])
        cg.add(hub.set_response_text_sensor(sens))
    if CONF_DEBUG in config:
        sens = await text_sensor.new_text_sensor(config[CONF_DEBUG])
        cg.add(hub.set_debug_text_sensor(sens))
    if CONF_CONNECTED_TIME_TEXT in config:
        sens = await text_sensor.new_text_sensor(config[CONF_CONNECTED_TIME_TEXT])
        cg.add(hub.set_connected_time_text_sensor(sens))
    if CONF_MAC_ADDRESS in config:
        sens = await text_sensor.new_text_sensor(config[CONF_MAC_ADDRESS])
        cg.add(hub.set_mac_address_text_sensor(sens))
    if CONF_MAC_INPUT in config:
        sens = await text_sensor.new_text_sensor(config[CONF_MAC_INPUT])
        cg.add(hub.set_mac_input_text_sensor(sens))
