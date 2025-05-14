/*******************************************************************************
 * @file octopus_task_manager_gpio_hal.h
 * @brief Header file for handling GPIO (General Purpose Input/Output) operations in the Octopus Task Manager.
 * 
 * This file provides macros and function declarations for managing GPIO pins for different hardware components,
 * reading and writing GPIO pin states, and controlling GPIO behavior such as low/high states.
 * It supports platform-specific configurations for different RTOS environments (CST OSAL RTOS, ITE Open RTOS).
 * 
 * @note Ensure that platform-specific GPIO pins are defined according to the hardware configuration.
 * 
 * @ingroup APP:SUB_TYPE
 * @version  1.0
 * @date     2024-12-12
 * @author   Octopus Team
 ******************************************************************************/

#ifndef __OCTOPUS_TASK_MANAGER_GPIO_HAL_H__
#define __OCTOPUS_TASK_MANAGER_GPIO_HAL_H__

/*******************************************************************************
 * INCLUDES
 */
#include "octopus_platform.h"  // Include the Octopus platform-specific configurations

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************* 
 * MACRO DEFINITIONS
 * The following macros define GPIO pin mappings and their operations for different platforms.
 */

#ifdef PLATFORM_CST_OSAL_RTOS

// Event identifiers for user test timers
#define USR_TEST_TIMER1_EVT                 0x0001  /**< Event identifier for test timer 1 */
#define USR_TEST_TIMER2_EVT                 0x0002  /**< Event identifier for test timer 2 */

// Define GPIO pins for various hardware components (mapped to platform-specific pins)
#define GPIO_ACC_SOC_PIN                    P1     /**< ACC_SOC pin */
#define GPIO_ACC_PIN                        P0     /**< ACC pin */

#define GPIO_DDD_PIN                        P31    /**< DDD pin */
#define GPIO_ZZD_PIN                        P32    /**< ZZD pin */
#define GPIO_YZD_PIN                        P33    /**< YZD pin */
#define GPIO_SKD_PIN                        P34    /**< SKD pin */

#define GPIO_KEY_PIN                        P14    /**< Key pin */

#define GPIO_SIF_R_PIN                      P15    /**< SIF Receive pin */
#define GPIO_SIF_S_PIN                      P20    /**< SIF Send pin */

#define GPIO_BMS_R_PIN                      P11    /**< BMS Receive pin */
//#define GPIO_BMS_S_PIN                      P11    /**< BMS Send pin */

// Macros for controlling GPIO pin states (Low/High)
#define GPIO_ACC_SOC_LOW()                  (HalGpioSet(GPIO_ACC_SOC_PIN, Bit_DISABLE))  /**< Set GPIO_ACC_SOC_PIN to Low */
#define GPIO_ACC_SOC_HIGH()                 (HalGpioSet(GPIO_ACC_SOC_PIN, Bit_ENABLE))   /**< Set GPIO_ACC_SOC_PIN to High */

// Macros for writing to and reading from GPIO pins
#define GPIO_PIN_WRITE(pin)                 (HalGpioSet((GpioPin_t)pin))    /**< Write to the specified GPIO pin */
#define GPIO_PIN_READ(pin)                  (HalGpioGet((GpioPin_t)pin))    /**< Read the state of the specified GPIO pin */

// Specific macros for reading GPIO states for various pins
#define GPIO_PIN_READ_ACC()                 (HalGpioGet(GPIO_ACC_PIN))      /**< Read the state of ACC_PIN */
#define GPIO_PIN_READ_DDD()                 (HalGpioGet(GPIO_DDD_PIN))      /**< Read the state of DDD_PIN */
#define GPIO_PIN_READ_ZZD()                 (HalGpioGet(GPIO_ZZD_PIN))      /**< Read the state of ZZD_PIN */
#define GPIO_PIN_READ_YZD()                 (HalGpioGet(GPIO_YZD_PIN))      /**< Read the state of YZD_PIN */
#define GPIO_PIN_READ_SKD()                 (HalGpioGet(GPIO_SKD_PIN))      /**< Read the state of SKD_PIN */

// Macros for controlling SIF pins
#define GPIO_PIN_SIF_SET_LOW()              (HalGpioSet(GPIO_SIF_S_PIN, Bit_DISABLE))  /**< Set GPIO_SIF_S_PIN to Low */
#define GPIO_PIN_SIF_SET_HIGH()             (HalGpioSet(GPIO_SIF_S_PIN, Bit_ENABLE))   /**< Set GPIO_SIF_S_PIN to High */
#define GPIO_PIN_READ_SIF()                 (HalGpioGet(GPIO_SIF_R_PIN))    					 /**< Read the state of SIF_R_PIN */

// Macros for controlling BMS pins
#define GPIO_PIN_BMS_SET_LOW()              //(HalGpioSet(GPIO_BMS_R_PIN, Bit_DISABLE))  /**< Set GPIO_BMS_S_PIN to Low */
#define GPIO_PIN_BMS_SET_HIGH()             //(HalGpioSet(GPIO_BMS_R_PIN, Bit_ENABLE))   /**< Set GPIO_BMS_S_PIN to High */
#define GPIO_PIN_READ_BMS()                 (HalGpioGet(GPIO_BMS_R_PIN))    					 	 /**< Read the state of BMS_R_PIN */

#elif defined(PLATFORM_ITE_OPEN_RTOS)

// Define macros for ITE Open RTOS platform (example macros; to be implemented as needed)
#define GPIO_MCU_SDIO_PWR_OUTPUT_PIN        21     /**< SDIO Power Output pin for MCU */

// Placeholder GPIO pins for ITE Open RTOS platform
#define GPIO_ACC_SOC_PIN                    (0x00) /**< ACC_SOC pin */
#define GPIO_ACC_PIN                        (0x00) /**< ACC pin */
#define GPIO_KEY_PIN                        (0x00) /**< Key pin */
#define GPIO_DDD_PIN                        (0x00) /**< DDD pin */
#define GPIO_ZZD_PIN                        (0x00) /**< ZZD pin */
#define GPIO_YZD_PIN                        (0x00) /**< YZD pin */
#define GPIO_SKD_PIN                        (0x00) /**< SKD pin */

// Macros for controlling GPIO pin states (Low/High) (platform-specific implementation)
#define GPIO_ACC_SOC_LOW()                  // Set GPIO_ACC_SOC_PIN to Low
#define GPIO_ACC_SOC_HIGH()                 // Set GPIO_ACC_SOC_PIN to High

// Macros for writing to and reading from GPIO pins (platform-specific implementation)
#define GPIO_PIN_WRITE(pin)                 (0x00) // Write to the specified GPIO pin
#define GPIO_PIN_READ(pin)                  (0x00) // Read the state of the specified GPIO pin

// Macros for reading GPIO states for various pins (platform-specific)
#define GPIO_PIN_READ_ACC()                 (0x00) // Read the state of ACC_PIN
#define GPIO_PIN_READ_DDD()                 (0x00) // Read the state of DDD_PIN
#define GPIO_PIN_READ_ZZD()                 (0x00) // Read the state of ZZD_PIN
#define GPIO_PIN_READ_YZD()                 (0x00) // Read the state of YZD_PIN
#define GPIO_PIN_READ_SKD()                 (0x00) // Read the state of SKD_PIN

// Macros for controlling SIF pins
#define GPIO_PIN_SIF_SET_LOW()              // Set GPIO_SIF_S_PIN to Low
#define GPIO_PIN_SIF_SET_HIGH()             // Set GPIO_SIF_S_PIN to High
#define GPIO_PIN_READ_SIF()                 (0x00) // Read the state of SIF_R_PIN

#elif defined(PLATFORM_STM32_RTOS)
typedef GPIO_TypeDef GPIO_GROUP;
// Default GPIO pin definitions for unsupported platforms
#define GPIO_ACC_SOC_PIN                    (0x00) /**< ACC_SOC pin */
#define GPIO_ACC_PIN                        (0x00) /**< ACC pin */
#define GPIO_KEY_PIN                        (0x00) /**< Key pin */
#define GPIO_DDD_PIN                        (0x00) /**< DDD pin */
#define GPIO_ZZD_PIN                        (0x00) /**< ZZD pin */
#define GPIO_YZD_PIN                        (0x00) /**< YZD pin */
#define GPIO_SKD_PIN                        (0x00) /**< SKD pin */

#define GPIO_POWER_GROUP GPIOA
#define GPIO_POWER_PIN GPIO_Pin_12

#define GPIO_POWER_F113_GROUP GPIOB
#define GPIO_POWER_F113_PIN GPIO_Pin_4
// Macros for controlling GPIO pin states (Low/High)
#define GPIO_ACC_SOC_LOW()                  // Set GPIO_ACC_SOC_PIN to Low
#define GPIO_ACC_SOC_HIGH()                 // Set GPIO_ACC_SOC_PIN to High

// Macros for writing to and reading from GPIO pins
#define GPIO_PIN_WRITE(GPIOx,pin,bit)        (GPIO_WriteBit(GPIOx,pin,bit)) // Write to the specified GPIO pin
#define GPIO_PIN_READ(GPIOx,pin)             (GPIO_ReadInputDataBit(GPIOx,pin)) // Read the state of the specified GPIO pin

// Macros for reading GPIO states for various pins
#define GPIO_PIN_READ_ACC()                 (0x00) // Read the state of ACC_PIN
#define GPIO_PIN_READ_DDD()                 (0x00) // Read the state of DDD_PIN
#define GPIO_PIN_READ_ZZD()                 (0x00) // Read the state of ZZD_PIN
#define GPIO_PIN_READ_YZD()                 (0x00) // Read the state of YZD_PIN
#define GPIO_PIN_READ_SKD()                 (0x00) // Read the state of SKD_PIN

// Macros for controlling SIF pins
#define GPIO_PIN_SIF_SET_LOW()              // Set GPIO_SIF_S_PIN to Low
#define GPIO_PIN_SIF_SET_HIGH()             // Set GPIO_SIF_S_PIN to High
#define GPIO_PIN_READ_SIF()                 (0x00) // Read the state of SIF_R_PIN

#endif

/*********************************************************************
 * FUNCTION DECLARATIONS
 */

// Function for turning Wi-Fi on or off (platform-specific)
#ifdef PLATFORM_ITE_OPEN_RTOS
void hal_gpio_set_wifi_onoff(bool onoff);
#endif

// Function to initialize GPIOs for the task
void hal_gpio_init(uint8_t task_id);

// Function to get the GPIO key mask code based on the pin number
uint8_t hal_get_gpio_key_mask_code(uint8_t pin);

bool IsPowerOn(void);
void power_on_off(bool onoff);

#ifdef __cplusplus
}
#endif

#endif /* __OCTOPUS_TASK_MANAGER_GPIO_HAL_H__ */
