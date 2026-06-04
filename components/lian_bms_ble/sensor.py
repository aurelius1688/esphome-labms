import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_CURRENT,
    CONF_POWER,
    CONF_TEMPERATURE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_EMPTY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_EMPTY,
    UNIT_PERCENT,
    UNIT_VOLT,
    UNIT_WATT,
    UNIT_WATT_HOURS,
)

from . import CONF_LIAN_BMS_BLE_ID, LianBmsBle, lian_bms_ble_ns

DEPENDENCIES = ["lian_bms_ble"]

CONF_TOTAL_VOLTAGE = "total_voltage"
CONF_CAPACITY_REMAINING = "capacity_remaining"
CONF_STATE_OF_CHARGE = "state_of_charge"
CONF_MOS_TEMPERATURE = "mos_temperature"
CONF_TEMPERATURE_SENSOR_1 = "temperature_sensor_1"
CONF_TEMPERATURE_SENSOR_2 = "temperature_sensor_2"
CONF_MIN_CELL_VOLTAGE = "min_cell_voltage"
CONF_MAX_CELL_VOLTAGE = "max_cell_voltage"
CONF_DELTA_CELL_VOLTAGE = "delta_cell_voltage"
CONF_AVERAGE_CELL_VOLTAGE = "average_cell_voltage"
CONF_CHARGING_CYCLES = "charging_cycles"
CONF_FULL_CAPACITY = "full_capacity"
CONF_DESIGN_CAPACITY = "design_capacity"
CONF_TOTAL_CYCLE_CAPACITY = "total_cycle_capacity"
CONF_CONNECTED_TIME = "connected_time"
CONF_CELL_VOLTAGE = "cell_voltage_"

SENSORS = [
    CONF_TOTAL_VOLTAGE,
    CONF_CURRENT,
    CONF_POWER,
    CONF_CAPACITY_REMAINING,
    CONF_STATE_OF_CHARGE,
    CONF_MOS_TEMPERATURE,
    CONF_TEMPERATURE_SENSOR_1,
    CONF_TEMPERATURE_SENSOR_2,
    CONF_MIN_CELL_VOLTAGE,
    CONF_MAX_CELL_VOLTAGE,
    CONF_DELTA_CELL_VOLTAGE,
    CONF_AVERAGE_CELL_VOLTAGE,
    CONF_CHARGING_CYCLES,
    CONF_FULL_CAPACITY,
    CONF_DESIGN_CAPACITY,
    CONF_TOTAL_CYCLE_CAPACITY,
    CONF_CONNECTED_TIME,
]

CELL_VOLTAGE_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_VOLT,
    accuracy_decimals=3,
    device_class=DEVICE_CLASS_VOLTAGE,
    state_class=STATE_CLASS_MEASUREMENT,
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_LIAN_BMS_BLE_ID): cv.use_id(LianBmsBle),
        cv.Optional(CONF_TOTAL_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CURRENT): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CAPACITY_REMAINING): sensor.sensor_schema(
            unit_of_measurement="Ah",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_STATE_OF_CHARGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_MOS_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TEMPERATURE_SENSOR_1): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TEMPERATURE_SENSOR_2): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_MIN_CELL_VOLTAGE): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_MAX_CELL_VOLTAGE): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_DELTA_CELL_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_AVERAGE_CELL_VOLTAGE): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CHARGING_CYCLES): sensor.sensor_schema(
            unit_of_measurement=UNIT_EMPTY,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_FULL_CAPACITY): sensor.sensor_schema(
            unit_of_measurement="Ah",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_DESIGN_CAPACITY): sensor.sensor_schema(
            unit_of_measurement="Ah",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TOTAL_CYCLE_CAPACITY): sensor.sensor_schema(
            unit_of_measurement="Ah",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CONNECTED_TIME): sensor.sensor_schema(
            unit_of_measurement="s",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:timer",
        ),
    }
).extend(
    {
        cv.Optional(f"{CONF_CELL_VOLTAGE}{i}"): CELL_VOLTAGE_SCHEMA for i in range(1, 33)
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_LIAN_BMS_BLE_ID])

    for key in SENSORS:
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(hub, f"set_{key}_sensor")(sens))

    for i in range(1, 33):
        key = f"{CONF_CELL_VOLTAGE}{i}"
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(hub.set_cell_voltage_sensor(i - 1, sens))
