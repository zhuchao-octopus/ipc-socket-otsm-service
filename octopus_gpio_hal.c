/**
 * ****************************************************************************
 * @copyright Copyright (c) XXX
 * All rights reserved.
 *
 * C file for the Octopus Task Manager module.
 * Defines macros, includes required libraries, and implements functions.
 */

/*********************************************************************
 * INCLUDES
 */

#include "octopus_platform.h" // Include Octopus module header
#include "octopus_log.h"
#include "octopus_gpio_hal.h" // Include the GPIO hardware abstraction layer header

#ifdef PLATFORM_CST_OSAL_RTOS
// Function to handle GPIO callback for different events on specific pins
void hal_gpio_callback(GpioPin_t pin, gpio_polarity_e type)
{
    switch ((uint8)pin)
    {
    case P18:
        LOG("P18 press\r\n"); // Log when P18 is pressed
        if (HalGpioGet(P11))  // Check the state of P11
        {
            LOG("P11 reset\r\n");         // If P11 is set, reset it
            HalGpioSet(P11, Bit_DISABLE); // Set P11 to low
        }
        else
        {
            LOG("P11 set\r\n");          // If P11 is not set, enable it
            HalGpioSet(P11, Bit_ENABLE); // Set P11 to high
        }
        break;
    }
}

// GPIO initialization function
void hal_gpio_init(uint8_t task_id)
{
    // Initialize GPIO pins and set their states
    HalGpioPinInit(GPIO_ACC_SOC_PIN, GPIO_OUTPUT); // Set ACC-->SOC pin (P0) as output
    HalGpioSet(GPIO_ACC_SOC_PIN, Bit_ENABLE);      // Enable the ACC-->SOC pin (P0)

    HalGpioPinInit(GPIO_SIF_S_PIN, GPIO_OUTPUT);
    HalGpioSet(GPIO_SIF_S_PIN, Bit_DISABLE); // Set SIF_S_PIN to low (disabled)

    HalGpioPinInit(GPIO_SIF_R_PIN, GPIO_INPUT);
    HalGpioPupdConfig(GPIO_SIF_R_PIN, GPIO_FLOATING); // Configure SIF_R_PIN as floating input

    // Initialize input pins for various GPIOs
    HalGpioPinInit(GPIO_DDD_PIN, GPIO_INPUT);        // Set DD pin (P31) as input
    HalGpioPupdConfig(GPIO_DDD_PIN, GPIO_PULL_DOWN); // Enable pull-down for DD pin (P31)

    HalGpioPinInit(GPIO_ZZD_PIN, GPIO_INPUT);        // Set ZZ pin (P32) as input
    HalGpioPupdConfig(GPIO_ZZD_PIN, GPIO_PULL_DOWN); // Enable pull-down for ZZ pin (P32)

    HalGpioPinInit(GPIO_YZD_PIN, GPIO_INPUT);        // Set YZ pin (P33) as input
    HalGpioPupdConfig(GPIO_YZD_PIN, GPIO_PULL_DOWN); // Enable pull-down for YZ pin (P33)

    HalGpioPinInit(GPIO_SKD_PIN, GPIO_INPUT);        // Set SKD pin (P34) as input
    HalGpioPupdConfig(GPIO_SKD_PIN, GPIO_PULL_DOWN); // Enable pull-down for SKD pin (P34)

    HalGpioPinInit(GPIO_ACC_PIN, GPIO_INPUT);        // Set ACC pin (P1) as input
    HalGpioPupdConfig(GPIO_ACC_PIN, GPIO_PULL_DOWN); // Enable pull-down for ACC pin (P1)

    HalGpioPinInit(GPIO_KEY_PIN, GPIO_INPUT);      // Set KEY pin (P14) as input
    HalGpioPupdConfig(GPIO_KEY_PIN, GPIO_PULL_UP); // Enable pull-up for KEY pin (P14)

    LOG_LEVEL("hal gpio init\r\n"); // Optional log for GPIO initialization (disabled here)
}

// Function to get the GPIO key mask code based on the pin number
uint8_t hal_get_gpio_key_mask_code(uint8_t pin)
{
    // Return a corresponding key mask based on the pin number
    switch (pin)
    {
    case P0:
        return 0;
    case P1:
        return 1;
    case P2:
        return 2;
    case P3:
        return 3;
    // case P4:    return 4;
    // case P5:    return 5;
    // case P6:    return 6;
    case P7:
        return 7;
    // case P8:    return 8;
    case P9:
        return 9;
    case P10:
        return 10;
    case P11:
        return 11;
    // case P12:   return 12;
    // case P13:   return 13;
    case P14:
        return 14;
    }
    return 0; // Default return value if pin doesn't match any case
}
#elif defined(PLATFORM_ITE_OPEN_RTOS)
// GPIO initialization function for ITE Open RTOS (currently no implementation)
void hal_gpio_init(uint8_t task_id)
{
    ithGpioSetOut(GPIO_MCU_SDIO_PWR_OUTPUT_PIN);
    ithGpioSetMode(GPIO_MCU_SDIO_PWR_OUTPUT_PIN, ITH_GPIO_MODE0);
    hal_gpio_set_wifi_onoff(true);
    LOG_LEVEL("hal gpio init\r\n"); // Optional log for GPIO initialization (disabled here)
}

// Function to get the GPIO key mask code (ITE Open RTOS version, returns 0)
uint8_t hal_get_gpio_key_mask_code(uint8_t pin)
{
    return 0; // Always return 0 for ITE Open RTOS
}

void hal_gpio_set_wifi_onoff(bool onoff)
{
    if (onoff)
        ithGpioSet(GPIO_MCU_SDIO_PWR_OUTPUT_PIN);
    else
        ithGpioClear(GPIO_MCU_SDIO_PWR_OUTPUT_PIN);
}

#else
void hal_gpio_init(uint8_t task_id)
{
    // LOG_LEVEL("GPIO init\r\n");  // Optional log for GPIO initialization (disabled here)
    LOG_LEVEL("hal gpio init\r\n"); // Optional log for GPIO initialization (disabled here)
}

// Function to get the GPIO key mask code (ITE Open RTOS version, returns 0)
uint8_t hal_get_gpio_key_mask_code(uint8_t pin)
{
   // Return a corresponding key mask based on the pin number
   return 0; // Default return value if pin doesn't match any case
}
#endif
