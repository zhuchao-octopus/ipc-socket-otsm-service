//"" 
/***************************************************************************************
 * @file        octopus_message_id.h
 * @brief       Message Group and Command ID Definitions for IPC Socket Communication
 * 
 * @details     This header file defines message groups and their corresponding 
 *              command IDs for inter-process communication (IPC) over socket 
 *              connections in the Octopus system. It also includes utility macros 
 *              for byte manipulation.
 * 
 * @author      [ak47]
 * @version     1.0
 * @date        2025-05-03
 * 
 * @note        This file is part of the Octopus communication protocol stack.
 *              Modifications should be documented, and the version number updated.
 * 
 ****************************************************************************************/

#ifndef ___OCTOPUS_MESSAGE_ID_H___
#define ___OCTOPUS_MESSAGE_ID_H___

/****************************************************************************************
 *                                       INCLUDES
 ****************************************************************************************/
// None

/****************************************************************************************
 *                                MESSAGE GROUP DEFINITIONS
 ****************************************************************************************/
/**
 * @enum    MessageGroup
 * @brief   Defines the different message groups for IPC socket communication.
 */
enum MessageGroup
{
    MSG_GROUP_0 = 0,    /**< General Help Information */
    MSG_GROUP_1,        /**< Configuration and Settings */
    MSG_GROUP_2,        /**< Reserved for future use */
    MSG_GROUP_3,        /**< Reserved for future use */
    MSG_GROUP_4,        /**< Reserved for future use */
    MSG_GROUP_5,        /**< Reserved for future use */
    MSG_GROUP_6,        /**< Reserved for future use */
    MSG_GROUP_7,        /**< Reserved for future use */
    MSG_GROUP_8,        /**< Reserved for future use */
    MSG_GROUP_9,        /**< Reserved for future use */
    MSG_GROUP_10,       /**< Reserved for future use */
    MSG_GROUP_11,       /**< Car Information Commands */
    MSG_GROUP_12,       /**< Reserved for future use */
    MSG_GROUP_13,       /**< Reserved for future use */
    MSG_GROUP_14,       /**< Reserved for future use */
    MSG_GROUP_15        /**< Reserved for future use */
};

/****************************************************************************************
 *                                MESSAGE COMMAND DEFINITIONS
 ****************************************************************************************/

/**
 * @enum    Message_Group_0_Cmd_Id
 * @brief   Command IDs for Message Group 0 (Help Information).
 */
enum Message_Group_0_Cmd_Id
{
    MSG_IPC_SOCKET_HELP_INFO = 0, /**< Display Help Information */
};

/**
 * @enum    Message_Group_1_Cmd_Id
 * @brief   Command IDs for Message Group 1 (Configuration and Settings).
 */
enum Message_Group_1_Cmd_Id
{
    MSG_IPC_SOCKET_CONFIG_FLAG = 50,    /**< Configure Socket Flags */
    MSG_IPC_SOCKET_CONFIG_PUSH_DELAY,   /**< Configure Socket Push Delay */
    MSG_IPC_SOCKET_CONFIG_IP            /**< Configure Socket IP Address */
};

/**
 * @enum    Message_Group_11_Cmd_Id
 * @brief   Command IDs for Message Group 11 (Car Information Commands).
 * @note    Command ID base: 100
 */
typedef enum
{
    MSG_CAR_GET_INDICATOR_INFO = 100,    /**< Request Indicator (lights, signals) Status */
    MSG_CAR_GET_METER_INFO,              /**< Request Meter Readings (odometer, speed, voltage) */
    MSG_CAR_GET_BATTERY_INFO,
    MSG_CAR_GET_ERROR_INFO,
    MSG_CAR_GET_DRIVINFO_INFO,           /**< Request Driving Information (gear, SOC, etc.) */

    MSG_CAR_METER_ODO_CLEAR,             /**< Clear Total Odometer (reset total distance) */
    MSG_CAR_METER_TIME_CLEAR,            /**< Clear Accumulated Ride Time */
    MSG_CAR_METER_TRIP_DISTANCE_CLEAR,   /**< Clear Trip Distance Counter (trip meter) */

    MSG_CAR_SET_GEAR_LEVEL,              /**< Set Car Gear */
    MSG_CAR_SET_LIGHT,                   /**< Control Headlights (turn ON/OFF) */
    MSG_CAR_SET_LOW_BEAM,                /**< Control Low Beam Headlights (turn ON/OFF) */
    MSG_CAR_SET_HIGH_BEAM,               /**< Control High Beam Headlights (turn ON/OFF) */
    MSG_CAR_SETTING_SAVE                 /**< Save Car Settings */
} Message_Group_11_Cmd_Id;

//#define MSG_CAR_SET_GEAR MSG_CAR_SET_GEAR_LEVEL
/****************************************************************************************
 *                                  UTILITY MACROS
 ****************************************************************************************/

/**
 * @brief   Merge two bytes into a single unsigned integer.
 * @param   byte1 The high byte.
 * @param   byte2 The low byte.
 * @return  A 16-bit unsigned integer representing the merged bytes.
 */
#define MERGE_BYTES(byte1, byte2) ((static_cast<unsigned int>(byte1) << 8) | (static_cast<unsigned int>(byte2)))

/**
 * @brief   Split a 16-bit unsigned integer into two separate bytes.
 * @param   value The 16-bit value to split.
 * @param   byte1 Reference to the high byte.
 * @param   byte2 Reference to the low byte.
 */
#define SPLIT_TO_BYTES(value, byte1, byte2)                  \
    byte1 = static_cast<unsigned char>((value >> 8) & 0xFF); \
    byte2 = static_cast<unsigned char>(value & 0xFF);

/****************************************************************************************
 *                               MESSAGE GROUP SHORTCUTS
 ****************************************************************************************/
#define MSG_GROUP_HELP        MSG_GROUP_0    /**< Alias for Help Information Group */
#define MSG_GROUP_SET         MSG_GROUP_1    /**< Alias for Configuration Group */
#define MSG_GROUP_SETTING     MSG_GROUP_1    /**< Alias for Settings Group */
#define MSG_GROUP_IPC_CONFIG  MSG_GROUP_1    /**< Alias for IPC Configuration Group */
#define MSG_GROUP_CAR         MSG_GROUP_11   /**< Alias for Car Information Group */

/****************************************************************************************
 *                                       ENDIF
 ****************************************************************************************/
#endif // ___OCTOPUS_MESSAGE_ID_H___
//""
