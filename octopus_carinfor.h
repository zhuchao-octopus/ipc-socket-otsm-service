/********************************** (C) COPYRIGHT *******************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * All rights reserved.
 *
 * @file    octopus_task_manager_carinfo.h
 * @brief   This header file defines structures, macros, constants, and function
 *          declarations related to managing car information within the Octopus
 *          task manager system.
 *
 * @details This file contains the definitions for monitoring various car
 *          parameters, including electrical and mechanical states such as
 *          battery voltage, motor faults, gear positions, and more. It also
 *          provides the necessary functions to manage and retrieve car information
 *          during system operation. This is essential for vehicle control systems
 *          that require precise monitoring of the car's status in real-time.
 *
 * @note    This software (modified or not) and binary are intended for use
 *          with microcontrollers manufactured by Nanjing Qinheng Microelectronics.
 * @author  Octopus Team
 * @version 1.0.0
 * @date    2024-12-09
 *******************************************************************************/
#ifndef __OCTOPUS_TASK_MANAGER_CARINFO_H__
#define __OCTOPUS_TASK_MANAGER_CARINFO_H__

/*******************************************************************************
 * INCLUDES
 */

#include "octopus_platform.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /*******************************************************************************
     * DEBUG SWITCH MACROS
     */

    /*******************************************************************************
     * MACROS
     */

    /*******************************************************************************
     * TYPEDEFS
     */
    typedef struct
    {
        uint8_t sideStand;                // Side stand status        0: off     1: on
        uint8_t bootGuard;                // Boot guard status        0: open   1: locked
        uint8_t hallFault;                // Hall fault (sensor)      0: no fault  1: fault
        uint8_t throttleFault;            // Throttle fault
        uint8_t controllerFault;          // Controller fault
        uint8_t lowVoltageProtection;     // Low voltage protection
        uint8_t cruise;                   // Cruise mode indicator
        uint8_t assist;                   // Assist mode indicator
        uint8_t motorFault;               // Motor fault
        uint8_t gear;                     // Gear position //0~7
        uint8_t motorRunning;             // Motor running status     1: running
        uint8_t brake;                    // Brake status
        uint8_t controllerProtection;     // Controller protection
        uint8_t coastCharging;            // Coasting charging status
        uint8_t antiSpeedProtection;      // Anti-speed protection
        uint8_t seventyPercentCurrent;    // 70% current
        uint8_t pushToTalk;               // Push-to-talk signal
        uint8_t ekkBackupPower;           // EKK backup power status
        uint8_t overCurrentProtection;    // Overcurrent protection
        uint8_t motorShaftLockProtection; // Motor shaft lock protection
        uint8_t reverse;                  // Reverse status
        uint8_t electronicBrake;          // Electronic brake
        uint8_t speedLimit;               // Speed limit
        uint8_t current;                  // Current (A)
        uint16_t hallCounter;             // Hall counter (change in value every 0.5 seconds)
        uint8_t soc;                      // State of charge (0-100%) and corresponding voltage levels
        uint8_t voltageSystem;            // Voltage system: 0x01:36V  0x02:48V  0x04:60V  0x08:64V  0x10:72V  0x20:80V  0x40:84V  0x80:96V
    } carinfo_sif_t;

    typedef struct
    {
        uint8_t SideStand;                // Side stand status        0: off     1: on
        uint8_t BootGuard;                // Boot guard status        0: open   1: locked
        uint8_t hallFault;                // Hall fault (sensor)      0: no fault  1: fault
        uint8_t throttleFault;            // Throttle fault
        uint8_t controllerFault;          // Controller fault
        uint8_t lowVoltageProtection;     // Low voltage protection
        uint8_t cruise;                   // Cruise mode indicator
        uint8_t assist;                   // Assist mode indicator
        uint8_t motorFault;               // Motor fault
        uint8_t gear;                     // Gear position //0~7
        uint8_t motorRunning;             // Motor running status     1: running
        uint8_t brake;                    // Brake status
        uint8_t controllerProtection;     // Controller protection
        uint8_t coastCharging;            // Coasting charging status
        uint8_t antiSpeedProtection;      // Anti-speed protection
        uint8_t seventyPercentCurrent;    // 70% current
        uint8_t pushToTalk;               // Push-to-talk signal
        uint8_t ekkBackupPower;           // EKK backup power status
        uint8_t overCurrentProtection;    // Overcurrent protection
        uint8_t motorShaftLockProtection; // Motor shaft lock protection
        uint8_t reverse;                  // Reverse status
        uint8_t electronicBrake;          // Electronic brake
        uint8_t speedLimit;               // Speed limit
        uint32_t current;                 // Current (0.1A)
        uint32_t voltage;                 // Voltage (0.1V)
        uint8_t voltageSystem;            // Voltage system: 0x01:36V  0x02:48V  0x04:60V  0x08:64V  0x10:72V  0x20:80V  0x40:84V  0x80:96V
        uint8_t soc;                      // State of charge (0-100%) and corresponding voltage levels
        uint32_t speed;                   // Speed (0.1 km/h)
        uint32_t speed_real;              // Actual speed (0.1 km/h)
        uint32_t rpm;                     // RPM (revolutions per minute)
    } carinfo_t;

#pragma pack(push, 1)
    typedef struct
    {
        uint16_t current;               // Current (0.1A)
        uint16_t voltage;               // Voltage (0.1V)
        uint8_t soc;                    // State of charge (0-100%) and corresponding voltage levels
        uint16_t speed;                 // Speed (0.1 km/h)
        uint16_t speed_real;            // Actual speed (0.1 km/h)
        uint16_t rpm;                   // RPM (offset: -20000)
        uint8_t voltageSystem;          // Voltage system: 0x01:36V  0x02:48V  0x04:60V  0x08:64V  0x10:72V  0x20:80V  0x40:84V  0x80:96V
        uint8_t gear;                   // Gear information
        uint32_t odo;                   // Odometer (0.1 km)
    } carinfo_meter_t;

#pragma pack(push, 1)
    typedef struct
    {
        uint8_t highBeam;               // High beam status
        uint8_t lowBeam;                // Low beam status
        uint8_t position;               // Position light status
        uint8_t frontFog;               // Front fog light status
        uint8_t rearFog;                // Rear fog light status
        uint8_t leftTurn;               // Left turn indicator status
        uint8_t rightTurn;              // Right turn indicator status
        uint8_t ready;                  // Ready status
        uint8_t charge;                 // Charging status
        uint8_t parking;                // Parking status
        
        uint8_t ecuFault;               // ECU fault status
        uint8_t sensorFault;            // Sensor fault status
        uint8_t motorFault;             // Motor fault status
       
        uint8_t fuse_fault;             // Fuse fault status (0: OK, 1: Fault, others: reserved)
        uint8_t plug_fault;             // Charging plug fault status
        uint8_t battery_fault;          // Battery fault status
        uint8_t brake_fault;            // Brake fault status
        uint8_t throttle_fault;         // Throttle fault status

        uint8_t bt;                     // Bluetooth indicator status
        uint8_t wifi;                   // Wi-Fi indicator status
    } carinfo_indicator_t;

    typedef struct
    {
        uint16_t avgEnergyConsumption; // Average energy consumption
        uint16_t travelTime;           // Travel time in minutes
        uint16_t avgSpeed;             // Average speed (0.1 km/h)
    } carinfo_drivinfo_form_t;

    typedef enum
    {
        DRIVINFO_GEAR_UNKNOWN = 0x00,  // Unknown
        DRIVINFO_GEAR_MANUAL_1 = 0x01, // Manual gear 1
        DRIVINFO_GEAR_MANUAL_2 = 0x02, // Manual gear 2
        DRIVINFO_GEAR_MANUAL_3 = 0x03, // Manual gear 3
        DRIVINFO_GEAR_MANUAL_4 = 0x04, // Manual gear 4
        DRIVINFO_GEAR_MANUAL_5 = 0x05, // Manual gear 5
        DRIVINFO_GEAR_MANUAL_6 = 0x06, // Manual gear 6
        DRIVINFO_GEAR_MANUAL_7 = 0x07, // Manual gear 7

        DRIVINFO_GEAR_REVERSE = 0x0A, // Reverse gear
        DRIVINFO_GEAR_PARK = 0x0B,    // Park gear
        DRIVINFO_GEAR_NEUTRAL = 0x0C, // Neutral gear
        DRIVINFO_GEAR_DRIVE = 0x0D,   // Drive gear
        DRIVINFO_GEAR_SPORT = 0x0E,   // Sport mode
        DRIVINFO_GEAR_FAULT = 0x0F,   // Gear fault
    } carinfo_drivinfo_gear_t;

    typedef enum
    {
        DRIVINFO_DRIVEMODE_Comfort = 0x00,      // Comfort mode
        DRIVINFO_DRIVEMODE_ECO = 0x01,          // ECO mode
        DRIVINFO_DRIVEMODE_AUTO = 0x02,         // Auto mode
        DRIVINFO_DRIVEMODE_Sport = 0x03,        // Sport mode
        DRIVINFO_DRIVEMODE_SNOW = 0x04,         // Snow mode
        DRIVINFO_DRIVEMODE_OFF_ROAD = 0x05,     // Off-road mode
    } carinfo_drivinfo_drivemode_t;

#pragma pack(push, 1)
    typedef struct
    {
        uint32_t odo;                           // Odometer (0.1 km)
        uint16_t tripA;                         // Trip A distance (0.1 km)
        uint16_t tripB;                         // Trip B distance (0.1 km)
        uint8_t energyType;                     // Energy type: 0x00: fuel (L/100KM), 0x01: electric (0.1 kWh/100KM)
        uint16_t enduranceMileage;              // Endurance mileage (0.1 km)
        uint16_t insEnergyConsumption;          // Instantaneous energy consumption (0.1 kWh/km)

        carinfo_drivinfo_form_t odoForm;        // Odometer driving information
        carinfo_drivinfo_form_t tripAForm;      // Trip A driving information
        carinfo_drivinfo_form_t tripBForm;      // Trip B driving information

        carinfo_drivinfo_gear_t gear;           // Gear information
        carinfo_drivinfo_drivemode_t driveMode; // Driving mode
    } carinfo_drivinfo_t;

    /*******************************************************************************
     * CONSTANTS
     */

    /*******************************************************************************
     * GLOBAL VARIABLES DECLEAR
     */

    /*******************************************************************************
     * GLOBAL FUNCTIONS DECLEAR
     */

    void app_carinfo_init_running(void);
    void app_carinfo_start_running(void);
    void app_carinfo_assert_running(void);
    void app_carinfo_running(void);
    void app_carinfo_post_running(void);
    void app_carinfo_stop_running(void);

    /// void app_carinfo_on_enter_run(void);
    /// void app_carinfo_on_exit_post_run(void);

    uint16_t app_carinfo_getSpeed(void);
    carinfo_meter_t *app_carinfo_get_meter_info(void);
    carinfo_indicator_t *app_carinfo_get_indicator_info(void);
    carinfo_drivinfo_t *app_carinfo_get_drivinfo_info(void);

#ifdef __cplusplus
}
#endif

#endif
