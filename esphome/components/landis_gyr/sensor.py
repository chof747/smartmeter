import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor
from esphome.const import (
  CONF_ID,
  CONF_POWER,
  CONF_ENERGY,
  STATE_CLASS_MEASUREMENT,
  STATE_CLASS_TOTAL_INCREASING,
  UNIT_KILOWATT_HOURS,
  UNIT_WATT,
  DEVICE_CLASS_ENERGY,
  DEVICE_CLASS_POWER
)

DEPENDENCIES = [ 'uart' ]
SMART_METER_DECRYPTION_KEY = 'meter_key'
MAX_MESSAGE_LENGTH = 'max_message_length'
CONF_POWEROUT = 'powerout'
CONF_ENERGYOUT = 'energyout'
CONF_BLINDENERGYIN = 'blindenergyin'
CONF_BLINDENERGYOUT = 'blindenergyout'
CONF_BLINDPOWERIN = 'blindpowerin'
CONF_BLINDPOWEROUT = 'blindpowerout'
CONF_TELEGRAM_COUNT = 'telegramcount_lasthour'
CONF_SERIALBLOCK_COUNT = 'serialblockcount_lasthour'


SENSOR_BEGIN_POS = 'begin_pos'

landisGyr_ns = cg.esphome_ns.namespace("landis_gyr")
LandisGyrComponent = landisGyr_ns.class_("LandysGyrReader", cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = (
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(LandisGyrComponent),
      cv.Required(SMART_METER_DECRYPTION_KEY): cv.string,
      cv.Optional(MAX_MESSAGE_LENGTH, 80): cv.int_, 

      cv.Optional(CONF_ENERGY): sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL_INCREASING
      ),
      cv.Optional(CONF_ENERGYOUT): sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL_INCREASING
      ),
      cv.Optional(CONF_BLINDENERGYIN): sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL_INCREASING
      ),
      cv.Optional(CONF_BLINDENERGYOUT): sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL_INCREASING
      ),
      cv.Optional(CONF_POWER): sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT
      ),
      cv.Optional(CONF_POWEROUT): sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT
      ),
      cv.Optional(CONF_BLINDPOWERIN): sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT
      ),
      cv.Optional(CONF_BLINDPOWEROUT): sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT
      ),
      cv.Optional(CONF_TELEGRAM_COUNT): sensor.sensor_schema(
        unit_of_measurement="Telegrams",
        icon="mdi:counter",
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT
      ),
      cv.Optional(CONF_SERIALBLOCK_COUNT): sensor.sensor_schema(
        unit_of_measurement="blocks",
        icon="mdi:counter",
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT
      )
    }
  )
  .extend(cv.COMPONENT_SCHEMA)
  .extend(uart.UART_DEVICE_SCHEMA)
)

async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  await uart.register_uart_device(var, config)

  if (CONF_ENERGY in config):
    sens = await sensor.new_sensor(config[CONF_ENERGY])
    cg.add(var.set_energy_sensor(sens))

  if (CONF_ENERGYOUT in config):
    sens = await sensor.new_sensor(config[CONF_ENERGYOUT])
    cg.add(var.set_energyout_sensor(sens))

  if (CONF_BLINDENERGYIN in config):
    sens = await sensor.new_sensor(config[CONF_BLINDENERGYIN])
    cg.add(var.set_blindenergyin_sensor(sens))

  if (CONF_BLINDENERGYOUT in config):
    sens = await sensor.new_sensor(config[CONF_BLINDENERGYOUT])
    cg.add(var.set_blindenergyout_sensor(sens))

  if (CONF_POWER in config):
    sens = await sensor.new_sensor(config[CONF_POWER])
    cg.add(var.set_power_sensor(sens))

  if (CONF_POWEROUT in config):
    sens = await sensor.new_sensor(config[CONF_POWEROUT])
    cg.add(var.set_powerout_sensor(sens))

  if (CONF_BLINDPOWERIN in config):
    sens = await sensor.new_sensor(config[CONF_BLINDPOWERIN])
    cg.add(var.set_blindpowerin_sensor(sens))

  if (CONF_BLINDPOWEROUT in config):
    sens = await sensor.new_sensor(config[CONF_BLINDPOWEROUT])
    cg.add(var.set_blindpowerout_sensor(sens))
  
  if (CONF_TELEGRAM_COUNT in config):
    sens = await sensor.new_sensor(config[CONF_TELEGRAM_COUNT])
    cg.add(var.set_telegram_count_sensor(sens))

  if (CONF_SERIALBLOCK_COUNT in config):
    sens = await sensor.new_sensor(config[CONF_SERIALBLOCK_COUNT])
    cg.add(var.set_serialblock_count_sensor(sens))

  if (SMART_METER_DECRYPTION_KEY in config):
    cg.add(var.set_smartmeter_decryption_key(config[SMART_METER_DECRYPTION_KEY]))

  if (MAX_MESSAGE_LENGTH in config):
    cg.add(var.set_max_message_length(config[MAX_MESSAGE_LENGTH]))