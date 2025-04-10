#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/helpers.h"  // 添加ErrorCode定义
#include "driver/gpio.h"  // 添加gpio_num_t定义

#ifdef USE_ESP32
// 充电状态
#define IP5209_CHARGE_STATUS_IDLE           0x0  // "Idle"
#define IP5209_CHARGE_STATUS_TRICKLE        0x01 // "Trickle Charging"
#define IP5209_CHARGE_STATUS_CCURRENT       0x02 // "Constant Current Charging"
#define IP5209_CHARGE_STATUS_CVOLTAGE       0x03 // "Constant Voltage Charging"
#define IP5209_CHARGE_STATUS_CVSTOP         0x04 // "Constant Voltage Stop Detection"
#define IP5209_CHARGE_STATUS_FULL           0x05 // "Charge Full"
#define IP5209_CHARGE_STATUS_TIMEOUT        0x06 // "Charge Timeout"
#define IP5209_CHARGE_STATUS_UNKNOWN        0x07 // "Unknown"

// 寄存器定义
#define IP5209_REG_SYS_CTL0        0x01
#define IP5209_REG_SYS_CTL1        0x02
#define IP5209_REG_SYS_CTL2        0x0C
#define IP5209_REG_SYS_CTL3        0x03
#define IP5209_REG_SYS_CTL4        0x04
#define IP5209_REG_SYS_CTL5        0x07
#define IP5209_REG_CHARGER_CTL1    0x22
#define IP5209_REG_CHARGER_CTL2    0x24
#define IP5209_REG_CHG_DIG_CTL4    0x25
#define IP5209_REG_BATVADC_DAT0    0xA2
#define IP5209_REG_BATVADC_DAT1    0xA3
#define IP5209_REG_BATIADC_DAT0    0xA4
#define IP5209_REG_BATIADC_DAT1    0xA5
#define IP5209_REG_BATOCV_DAT0     0xA8
#define IP5209_REG_BATOCV_DAT1     0xA9
#define IP5209_REG_READ0           0x71

namespace esphome {
    namespace inj_ip5209 {
        class InjIP5209 : public PollingComponent, public i2c::I2CDevice {  // 修改i2c_device为i2c
            public:
                void setup() override;
                void update() override;
                float get_setup_priority() const override;
                void initialize();
                void set_irq_pin(uint8_t irq_pin) {irq_pin_ = static_cast<gpio_num_t>(irq_pin);}
                void set_charge_current_value(uint8_t charge_current) {charge_current_ = charge_current;}
                void set_auto_poweroff_value(bool auto_poweroff) {auto_poweroff_ = auto_poweroff;}
                void set_battery_level_sensor(sensor::Sensor *battery_level_sensor) { battery_level_sensor_ = battery_level_sensor;}
                void set_battery_voltage_sensor(sensor::Sensor *battery_voltage_sensor) { battery_voltage_sensor_ = battery_voltage_sensor;}
                void set_battery_current_sensor(sensor::Sensor *battery_current_sensor) { battery_current_sensor_ = battery_current_sensor;}
                esphome::i2c::ErrorCode set_disable_auto_poweroff(bool disableAutoPowerOff); 
                esphome::i2c::ErrorCode set_charge_current(uint8_t chage_current) ; 
                float get_battery_oc_voltage();
                float get_battery_level(float voltage);
                float get_battery_current();
                               
            protected:
                gpio_num_t irq_pin_ = GPIO_NUM_NC;
                uint8_t charge_current_ = 0x0;
                uint16_t sensor_id_;
                bool auto_poweroff_ = true;
                sensor::Sensor *battery_level_sensor_{nullptr};
                sensor::Sensor *battery_voltage_sensor_{nullptr};
                sensor::Sensor *battery_current_sensor_{nullptr};
        };
    }  
}

#endif