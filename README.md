example yaml:

i2c:
  - id: bus_a
    scl: GPIO13
    sda: GPIO12
    scan: false
    frequency: 400kHz
    # timeout: 1ms

external_components:
  - source:
      type: git
      url: https://gitee.com/zexuntec/inj_ip5209_esphome.git
    refresh: 0ms

sensor:
  - platform: inj_ip5209
    i2c_id: bus_a
    irq_pin: GPIO7
    id: ip5209
    disable_auto_poweroff: true
    charge_current: 5
    battery_level:
      name: "Battery Level"
    battery_voltage:
      name: "Battery Voltage"
    current:
      name: "Battery Current"
    update_interval: 10s