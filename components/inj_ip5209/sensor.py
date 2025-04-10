from esphome import pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor, binary_sensor
from esphome.const import (
    CONF_ID,
    CONF_BATTERY_LEVEL,
    CONF_BATTERY_VOLTAGE,
    CONF_CURRENT,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_CURRENT,
    UNIT_PERCENT,
    UNIT_VOLT,
    UNIT_MILLIAMP,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    CONF_INPUT,
    CONF_OUTPUT,
)

DEPENDENCIES = ["i2c", "sensor"]

# 定义命名空间
inj_ip5209_ns = cg.esphome_ns.namespace("inj_ip5209")
InjIP5209 = inj_ip5209_ns.class_("InjIP5209", cg.PollingComponent, i2c.I2CDevice)

# 定义配置参数
CONF_DISABLE_AUTO_POWEROFF = "disable_auto_poweroff"
CONF_CHARGE_CURRENT = "charge_current"
CONF_IRQ_PIN = "irq_pin"

pin_with_input_and_output_support = pins.internal_gpio_pin_number(
    {CONF_OUTPUT: True, CONF_INPUT: True}
)

# 定义配置模式
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(InjIP5209),
            cv.Required(CONF_BATTERY_LEVEL): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_BATTERY,
                state_class=STATE_CLASS_MEASUREMENT,
                #entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_BATTERY_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_BATTERY,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:battery-check",
            ),
            cv.Optional(CONF_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_MILLIAMP,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:current-dc",
            ),
            cv.Required(CONF_IRQ_PIN): pin_with_input_and_output_support,
            cv.Optional(CONF_DISABLE_AUTO_POWEROFF, default=True): cv.boolean,
            cv.Optional(CONF_CHARGE_CURRENT, default=5): cv.int_range(min=1, max=23),
        }
    )
    .extend(cv.polling_component_schema("5s"))
    .extend(i2c.i2c_device_schema(0x75))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    # 设置中断引脚
    cg.add(var.set_irq_pin(config[CONF_IRQ_PIN]))

    if CONF_BATTERY_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_LEVEL])
        cg.add(var.set_battery_level_sensor(sens))

    if CONF_BATTERY_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_VOLTAGE])
        cg.add(var.set_battery_voltage_sensor(sens))

    if CONF_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT])
        cg.add(var.set_battery_current_sensor(sens))

    # 设置自动关机禁用
    if CONF_DISABLE_AUTO_POWEROFF in config:
        disable_auto_poweroff = config[CONF_DISABLE_AUTO_POWEROFF]
        cg.add(var.set_auto_poweroff_value(disable_auto_poweroff))

    # 设置充电电流
    if CONF_CHARGE_CURRENT in config:
        charge_current = config[CONF_CHARGE_CURRENT]
        cg.add(var.set_charge_current_value(charge_current))