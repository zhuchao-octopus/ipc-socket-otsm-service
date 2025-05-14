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
#define CELL_COUNT 64
#define ERROR_CODE_COUNT 10
    /*******************************************************************************
     * TYPEDEFS
     */
    // 故障-故障信息
    typedef enum  __attribute__((packed))
    {
        ERROR_CODE_IDLE = 0X00,                                      // 无动作
        ERROR_CODE_NORMAL = 0X01,                                    // 正常状态
        ERROR_CODE_BRAKE = 0X03,                                     // 已刹车
        ERROR_CODE_THROTTLE_NOT_ZERO = 0X04,                         // 转把没有归位（停在高位处）
        ERROR_CODE_THROTTLE_HALLSENSOR_ABNORMALITY = 0X05,           // 转把故障
        ERROR_CODE_LOW_VOLTAGE_PROTECTION = 0X06,                    // 低电压保护
        ERROR_CODE_OVER_VOLTAGE_PROTECTION = 0X07,                   // 过电压保护
        ERROR_CODE_HALLSENSOR_ABNORMALITY = 0X08,                    // 电机霍尔信号线故障
        ERROR_CODE_MOTOR_ABNORMALITY = 0X09,                         // 电机相线故障
        ERROR_CODE_CONTROLLER_OVERHEAT = 0X10,                       // 控制器温度高已达到保护点
        ERROR_CODE_CONTROLLER_TEMPERATURE_SENSOR_ABNORMALITY = 0X11, // 控制器温度传感器故障
        ERROR_CODE_CURRENT_SENSOR_ABNORMALITY = 0X12,                // 电流传感器故障
        ERROR_CODE_BATTERY_OVERHEAT = 0X13,                          // 电池内温度故障
        ERROR_CODE_MOTOR_TEMPERATURE_SENSOR_ABNORMALITY = 0X14,      // 电机内温度传感器故障
        ERROR_CODE_CONTROLLER_ABNORMALITY = 0X15,                    // 控制器故障
        ERROR_CODE_ASSIST_POWER_SENSOR_ABNORMALITY = 0X16,           // 助力传感器故障
        ERROR_CODE_SPEED_SENSOR_ABNORMALITY = 0X21,                  // 速度传感器故障
        ERROR_CODE_BMS_ABNORMALITY = 0X22,                           // BMS通讯故障
        ERROR_CODE_LAMP_ABNORMALITY = 0X23,                          // 大灯故障
        ERROR_CODE_LAMP_SENSOR_ABNORMALITY = 0X24,                   // 大灯传感器故障
        ERROR_CODE_COMMUNICATION_ABNORMALITY = 0X30,                 // 通讯故障

        ERROR_CODE_BEGIN = ERROR_CODE_THROTTLE_NOT_ZERO,       // 故障码开始
        ERROR_CODE_END = ERROR_CODE_COMMUNICATION_ABNORMALITY, // 故障码结束
    } __attribute__((packed)) ERROR_CODE;

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

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    typedef struct
    {
        uint16_t temp;
        uint16_t max_temp;
        uint16_t min_temp;
        uint16_t voltage;
        uint16_t current;
        uint16_t avg_current;
        uint16_t res_cap;
        uint16_t full_cap;

        uint16_t cycle_times;
        uint16_t max_uncharge_time;
        uint16_t last_uncharge_time;
        uint16_t cell_voltage[CELL_COUNT];

        uint8_t total_cell;
        uint8_t rel_charge_state;
        uint8_t abs_charge_state;

        uint16_t voltage_system; // Battery system voltage type:
                                 // 0x01:36V, 0x02:48V, 0x04:60V, 0x08:64V,
                                 // 0x10:72V, 0x20:80V, 0x40:84V, 0x80:96V

        uint16_t power; //< Power in W
        uint16_t soc;   // State of Charge: 0–100% (based on voltage curve)
        uint16_t range; //< Estimated range in 100m
        uint16_t max_range;
    } battery_t;

#pragma pack(push, 1)
    typedef struct
    {
        uint8_t highBeam;  // High beam status
        uint8_t lowBeam;   // Low beam status
        uint8_t position;  // Position light status
        uint8_t frontFog;  // Front fog light status
        uint8_t rearFog;   // Rear fog light status
        uint8_t leftTurn;  // Left turn indicator status
        uint8_t rightTurn; // Right turn indicator status
        uint8_t ready;     // Ready status
        uint8_t parking;   // Parking status
        uint8_t brake;     // Brake status

        uint8_t bt;   // Bluetooth indicator status
        uint8_t wifi; // Wi-Fi indicator status
        uint8_t walk_assist;
    } carinfo_indicator_t;

    typedef struct
    {
        uint16_t odo;           // Total distance traveled (unit: 0.1 km) Trip Odometer
        uint16_t rpm;           // Motor RPM (raw value, offset by -20000)
        uint16_t speed;         // Displayed speed (unit: 0.1 km/h)
        uint16_t speed_actual;  // Actual wheel speed (unit: 0.1 km/h)
        uint16_t speed_limit;   // 0 = OFF, 10~90 km/h
        uint16_t ride_time;     // Total ride time (unit: seconds)
        uint16_t trip_distance; // Trip meter (unit: 0.1 km)

        uint8_t gear; // Current gear level (0–N)
        uint8_t max_gear_level;

        uint8_t unit_type; // Unit system: 0 = Metric (km/km/h), 1 = Imperial (mi/mph)
        uint8_t wheel_diameter;
    } carinfo_meter_t;

    typedef struct
    {
        uint16_t voltage;
        uint16_t current;
        uint16_t power; //< Power in W
        uint16_t soc;   // State of Charge: 0–100% (based on voltage curve)
        uint16_t range; //< Estimated range in 100m
        uint16_t max_range;
        uint8_t rel_charge_state;
        uint8_t abs_charge_state;
    } carinfo_battery_t;

    // 故障信息
    typedef struct
    {
        uint8_t ecuFault;    // ECU fault status
        uint8_t sensorFault; // Sensor fault status
        uint8_t motorFault;  // Motor fault status

        uint8_t fuse_fault;     // Fuse fault status (0: OK, 1: Fault, others: reserved)
        uint8_t plug_fault;     // Charging plug fault status
        uint8_t battery_fault;  // Battery fault status
        uint8_t brake_fault;    // Brake fault status
        uint8_t throttle_fault; // Throttle fault status
        uint8_t error[ERROR_CODE_COUNT];
    } __attribute__((packed)) carinfo_error_t;
#pragma pack(pop) 	
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
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
        DRIVINFO_DRIVEMODE_Comfort = 0x00,  // Comfort mode
        DRIVINFO_DRIVEMODE_ECO = 0x01,      // ECO mode
        DRIVINFO_DRIVEMODE_AUTO = 0x02,     // Auto mode
        DRIVINFO_DRIVEMODE_Sport = 0x03,    // Sport mode
        DRIVINFO_DRIVEMODE_SNOW = 0x04,     // Snow mode
        DRIVINFO_DRIVEMODE_OFF_ROAD = 0x05, // Off-road mode
    } carinfo_drivinfo_drivemode_t;

#pragma pack(push, 1)
    typedef struct
    {
        uint32_t odo;                  // Odometer (0.1 km)
        uint16_t tripA;                // Trip A distance (0.1 km)
        uint16_t tripB;                // Trip B distance (0.1 km)
        uint8_t energyType;            // Energy type: 0x00: fuel (L/100KM), 0x01: electric (0.1 kWh/100KM)
        uint16_t enduranceMileage;     // Endurance mileage (0.1 km)
        uint16_t insEnergyConsumption; // Instantaneous energy consumption (0.1 kWh/km)

        carinfo_drivinfo_form_t odoForm;   // Odometer driving information
        carinfo_drivinfo_form_t tripAForm; // Trip A driving information
        carinfo_drivinfo_form_t tripBForm; // Trip B driving information

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

    /**
     * @brief Retrieve current vehicle speed (0.1 km/h).
     * @return uint16_t Speed in 0.1 km/h.
     */
    uint16_t app_carinfo_getSpeed(void);

    /**
     * @brief Retrieve indicator status structure.
     * @return Pointer to current carinfo_indicator_t.
     */
    carinfo_indicator_t *app_carinfo_get_indicator_info(void);
    /**
     * @brief Retrieve meter information pointer.
     * @return Pointer to current carinfo_meter_t.
     */
    carinfo_meter_t *app_carinfo_get_meter_info(void);
    carinfo_battery_t *app_carinfo_get_battery_info(void);
    carinfo_error_t *app_carinfo_get_error_info(void);
    /**
     * @brief Retrieve drive information.
     * @return Pointer to current carinfo_drivinfo_t.
     */
    carinfo_drivinfo_t *app_carinfo_get_drivinfo_info(void);

    /**
     * @brief Initializes the car indicator module.
     *        This function resets all indicator flags, change flags, and warning flags.
     */
    // void car_indicator_init(void);

    /**
     * @brief Gets the value of the indicator flag array at the specified index.
     *
     * @param index The index to retrieve (0 to INDICATOR_TYPE_ARRAY_COUNT - 1).
     * @param indicator Pointer to store the value at the specified index.
     * @return uint8_t Returns 1 if the index is valid and the value is retrieved, 0 otherwise.
     */
    // uint8_t car_indicator_get_indicator(uint8_t index, uint8_t *indicator);

    /**
     * @brief Returns the number of bytes used to store all indicator flags.
     *
     * @return uint8_t The count of indicator flag array elements.
     */
    // uint8_t data_indicator_get_indicator_type_array_count(void);

    /**
     * @brief Gets the status of a specific indicator.
     *
     * @param type The indicator type to check.
     * @return uint8_t Returns 1 if the indicator is on, 0 if off.
     */
    // uint8_t car_indicator_get_indicatorflag(IndicatorType type);

    /**
     * @brief Sets the status of a specific indicator.
     *        If the status has changed, the change flag will be updated accordingly.
     *
     * @param type The indicator type to set.
     * @param flag The new status (1 for on, 0 for off).
     */
    // void car_indicator_set_indicatorflag(IndicatorType type, uint8_t flag);

    /**
     * @brief Checks whether any indicator flag has changed since last clear.
     *
     * @return uint8_t Returns 1 if any change has occurred, 0 otherwise.
     */
    // uint8_t car_indicator_get_indicatorflag_changed(void);

    /**
     * @brief Clears all indicator flag change records.
     */
    // void car_indicator_clear_indicatorflag_changed(void);

    /**
     * @brief Gets the current status of the hazard warning flag.
     *
     * @return uint8_t Returns 1 if hazard is active, 0 otherwise.
     */
    // uint8_t car_indicator_get_hazard(void);

    /**
     * @brief Sets the hazard warning flag status.
     *
     * @param flag 1 to enable hazard, 0 to disable.
     */
    // void car_indicator_set_hazard(uint8_t flag);

    /**
     * @brief Gets the warning flag status for a specific indicator.
     *
     * @param type The indicator type to check.
     * @return uint8_t Returns 1 if warning flag is set, 0 otherwise.
     */
    // uint8_t car_indicator_get_warnflag(IndicatorType type);

    /**
     * @brief Sets or clears the warning flag for a specific indicator.
     *        Setting the flag also starts the warning timer.
     *
     * @param type The indicator type to set.
     * @param flag 1 to set the warning, 0 to clear.
     */
    // void car_indicator_set_warnflag(IndicatorType type, uint8_t flag);

    /**
     * @brief Checks whether there is an active warning event.
     *        This can be based on current warning flags or timer condition.
     *
     * @return uint8_t Returns 1 if a warning event is active, 0 otherwise.
     */
    // uint8_t car_indicator_get_warnflag_event(void);

    // void car_indicator_proc_turn_signal(void);
    // void car_meter_proc_speed_rpm(void);
    void app_carinfo_add_error_code(ERROR_CODE code);
    uint8_t app_carinfo_get_top_error_code(void);

    extern carinfo_meter_t lt_carinfo_meter;         // Local meter data structure
    extern carinfo_indicator_t lt_carinfo_indicator; // Local indicator data structure
    extern carinfo_battery_t lt_carinfo_battery;
    extern carinfo_error_t lt_carinfo_error;

#ifdef __cplusplus
}
#endif

#endif
