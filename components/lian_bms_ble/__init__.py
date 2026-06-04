import esphome.codegen as cg
from esphome.components import ble_client
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["ble_client"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor", "switch", "number"]

lian_bms_ble_ns = cg.esphome_ns.namespace("lian_bms_ble")
LianBmsBle = lian_bms_ble_ns.class_(
    "LianBmsBle", ble_client.BLEClientNode, cg.PollingComponent
)

CONF_LIAN_BMS_BLE_ID = "lian_bms_ble_id"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LianBmsBle),
        }
    )
    .extend(ble_client.BLE_CLIENT_SCHEMA)
    .extend(cv.polling_component_schema("5s"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)
