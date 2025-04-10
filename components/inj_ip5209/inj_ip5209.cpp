#include "inj_ip5209.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "driver/gpio.h"  // 添加gpio相关头文件

namespace esphome {
    namespace inj_ip5209 {

        static const char *const TAG = "inj_ip5209";
        bool isInitialized = false;

        void InjIP5209::setup(){
            ESP_LOGCONFIG(TAG, "Setting up IP5xxx ...");
            gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << this->irq_pin_), // 设置要配置的GPIO引脚
                .mode = GPIO_MODE_INPUT,              // 设置为输入模式
                .pull_up_en = GPIO_PULLUP_DISABLE,     // 禁用上拉电阻
                .pull_down_en = GPIO_PULLDOWN_DISABLE,// 禁用下拉电阻
                .intr_type = GPIO_INTR_DISABLE        // 禁用中断
            };
            gpio_config(&io_conf);
            if (isInitialized) {
                initialize();
            }
            ESP_LOGD(TAG, "Setup(): set charge current to %d", this->charge_current_);
        }

        // initialize
        void InjIP5209::initialize() {
            isInitialized = false;
            if (!gpio_get_level(this->irq_pin_)) {
                ESP_LOGD(TAG, "device was not ready!");
                return;
            }
            esphome::i2c::ErrorCode ret = set_disable_auto_poweroff(this->auto_poweroff_);
            if (ret != esphome::i2c::ERROR_OK) {
                ESP_LOGE(TAG, "Setup(): Failed to set auto poweroff!");
                return;
            }
            ESP_LOGD(TAG, "Setup(): set auto poweroff success to %s", this->auto_poweroff_ ? "true" : "false");
            ret = set_charge_current(this->charge_current_);
            if (ret != esphome::i2c::ERROR_OK) {
                ESP_LOGE(TAG, "Setup(): Failed to set charge current!");
                return;
            }
            isInitialized = true;
        }

        // update
        void InjIP5209::update() {
            float voltage_level = NAN;
            float voltage = NAN;
            float current = NAN;
            
            if (!gpio_get_level(this->irq_pin_)) {
                ESP_LOGD(TAG, "device was not ready!");
                return;
            }
            if (!isInitialized) {
                initialize();
            }

            voltage = get_battery_oc_voltage();
            voltage_level = get_battery_level(voltage);
            if (voltage_level < 0) {
                ESP_LOGE(TAG, "get wrong voltage level value: %f", voltage_level);
            }
            if (this->battery_level_sensor_) {
                this->battery_level_sensor_->publish_state(voltage_level);
            }

            if (this->battery_voltage_sensor_) {
                this->battery_voltage_sensor_->publish_state(voltage);
            }

            if (this->battery_current_sensor_) {
                current = get_battery_current();
                if (current < 0) {
                    ESP_LOGE(TAG, "get wrong current value: %f", current);
                } else {
                    this->battery_current_sensor_->publish_state(current);
                }
            }
        }

        // 关闭低负载自动关机功能
        esphome::i2c::ErrorCode InjIP5209::set_disable_auto_poweroff(bool disableAutoPowerOff) {
            uint8_t data[1];
            esphome::i2c::ErrorCode ret = this->read_register(IP5209_REG_SYS_CTL1, data, 1);
            ESP_LOGD(TAG, "read IP5209_REG_SYS_CTL1: %i", ret);
            if (ret == esphome::i2c::ERROR_OK) {
                uint8_t isDisable = data[0];
                if (disableAutoPowerOff) {
                    isDisable &= ~(1 << 1);
                } else {
                    isDisable |= (1 << 1);
                }
                ret = this->write_register(IP5209_REG_SYS_CTL1, &isDisable, 1);
                return ret;
              } else {
                return esphome::i2c::ERROR_UNKNOWN;
              }
        }

        // 设置充电电流
        esphome::i2c::ErrorCode InjIP5209::set_charge_current(uint8_t chage_current) {
            uint8_t data[1];
            esphome::i2c::ErrorCode ret = this->read_register(IP5209_REG_CHG_DIG_CTL4, data, 1);
            if (ret == esphome::i2c::ERROR_OK) {
                uint8_t sData = data[0];
                sData = (sData & 0xE0) | (chage_current & 0x1F);
                ret = this->write_register(IP5209_REG_CHG_DIG_CTL4, &sData, 1);
                return ret;
            } else {
                return esphome::i2c::ERROR_UNKNOWN;
            }
        }

        float InjIP5209::get_battery_oc_voltage() {
            uint8_t dataLow, dataHigh;
            float voltage = 0.0f;
            esphome::i2c::ErrorCode ret = this->read_register(IP5209_REG_BATOCV_DAT0, &dataLow, 1);
            if (ret == esphome::i2c::ERROR_OK) {
                ret = this->read_register(IP5209_REG_BATOCV_DAT1, &dataHigh, 1);
                if (ret == esphome::i2c::ERROR_OK) {
                        // 判断是否为补码
                    if ((dataHigh & 0x20) == 0x20) { // 补码情况
                        int16_t a = ~dataLow;
                        int16_t b = (~(dataHigh & 0x1F)) & 0x1F;
                        int16_t c = (b << 8) | (a + 1);
                        voltage = 2600.0f - (c * 0.26855f); // 单位：mV
                        voltage = voltage / 1000.0f; // 转换为V
                    } else { // 原码情况
                        uint16_t batOcVadc = (static_cast<uint16_t>(dataHigh) << 8) | dataLow;
                        voltage = 2600.0f + (batOcVadc * 0.26855f); // 单位：mV
                        voltage = voltage / 1000.0f; // 转换为V
                    }
                } else {
                    ESP_LOGD(TAG, "Fail to read voltage value high!");
                }
            } else {
                    ESP_LOGD(TAG, "Fail to read voltage value low!");
            }
            ESP_LOGD(TAG, "Get voltage: %.2f", voltage);
            return voltage;
        }

        // 读取开路电压值并转换成level
        float InjIP5209::get_battery_level(float voltage) {
            float voltage_level = -1.0;

            if (voltage >= 4.2) {
                voltage_level = 100.0f;
            }else if (voltage >= 4.06f && voltage < 4.2f) {
                voltage_level = 90.0f + (voltage - 4.06f) * 71.43f;
            } else if (voltage >= 3.98f && voltage < 4.06f) {
                voltage_level = 80.0f + (voltage - 3.98f) * 125.0f;
            } else if (voltage >= 3.92f && voltage < 3.98f) {
                voltage_level = 70.0f + (voltage - 3.92f) * 166.67f;
            } else if (voltage >= 3.87f && voltage < 3.92f) {
                voltage_level = 60.0f + (voltage - 3.87f) * 200.0f;
            } else if (voltage >= 3.82f && voltage < 3.87f) {
                voltage_level = 50.0f + (voltage - 3.82) * 200.0f;
            } else if (voltage >= 3.79f && voltage < 3.82f) {
                voltage_level = 40.0f + (voltage - 3.79f) * 333.33f;
            } else if (voltage >= 3.77f && voltage < 3.79f) {
                voltage_level = 30.0f + (voltage - 3.77) * 500.0f;
            } else if (voltage >= 3.74f && voltage < 3.77f) {
                voltage_level = 20.0f + (voltage - 3.74f) * 333.33f;
            } else if (voltage >= 3.68f && voltage < 3.74f) {
                voltage_level = 10.0f + (voltage - 3.68f) * 166.67f;
            } else if (voltage >= 3.45f && voltage < 3.68f) {
                voltage_level = 5.0f + (voltage - 3.45f) * 17.86f;
            } else if (voltage >= 3.00f && voltage < 3.45f) {
                voltage_level = (voltage - 3.00f) * 11.11f;
            } 
                
            return voltage_level;            
        }

        float InjIP5209::get_battery_current() {
            uint8_t dataLow, dataHigh;
            esphome::i2c::ErrorCode ret = this-> read_register(IP5209_REG_BATIADC_DAT0, &dataLow, 1);
            if (ret != esphome::i2c::ERROR_OK) {
                ESP_LOGE(TAG, "Failed to read low register");
                return -1.0f;
            }
            ret = this->read_register(IP5209_REG_BATIADC_DAT1, &dataHigh, 1);
            if (ret != esphome::i2c::ERROR_OK) {
                ESP_LOGE(TAG, "Failed to read high register");
                return -1.0f;
            }
            // 判断正负值
            if ((dataHigh & 0x20) == 0x20) { // 负值
                int16_t a = ~dataLow;
                int16_t b = (~(dataHigh & 0x1F)) & 0x1F;
                int16_t batIadc = (b << 8) | (a + 1);
                return -batIadc * 0.745985f; // 单位：mA
            } else { // 正值
                int16_t batIadc = (static_cast<int16_t>(dataHigh) << 8) | dataLow;
                return batIadc * 0.745985f; // 单位：mA
            }        
        }

        float InjIP5209::get_setup_priority() const {
            return setup_priority::DATA;
        }

    }
}