
#ifndef OCTOPUS_IPC_PTL_HANDLER_HPP
#define OCTOPUS_IPC_PTL_HANDLER_HPP

/*
 * File: octopus_ipc_ptl_handler.hpp
 * Description: This header file defines the IPC (Inter-Process Communication) message structure and message group types
 *              used in the Octopus system. It includes the necessary declarations for the message serialization and
 *              deserialization process. Additionally, it defines the command types and message groups that are used to
 *              differentiate different types of messages sent and received via the communication channel.
 *
 *              The file also includes utility functions for mapping message IDs and groups to human-readable strings
 *              for debugging or logging purposes.
 *
 * Features:
 * - Defines message groups for categorizing different types of messages
 * - Declares the `DataMessage` structure, which includes message ID, command type, and data elements
 * - Provides functions for serializing and deserializing `DataMessage` objects into binary format
 * - Includes functions for retrieving human-readable message names and group names for debugging and logging
 *
 * Author: [Your Name]
 * Date: [Date]
 * Version: 1.0
 */
/// @brief ///////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <iostream>
#include <iomanip>
#include <stdexcept>
/// @brief ///////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Message group definitions for IPC socket communication
enum MessageGroup
{
    MSG_GROUP_0 = 0,
    MSG_GROUP_1,
    MSG_GROUP_2,
    MSG_GROUP_3,
    MSG_GROUP_4,
    MSG_GROUP_5,
    MSG_GROUP_6,
    MSG_GROUP_7,
    MSG_GROUP_8,
    MSG_GROUP_9,
    MSG_GROUP_10,
    MSG_GROUP_11,
    MSG_GROUP_12,
    MSG_GROUP_13,
    MSG_GROUP_14,
    MSG_GROUP_15
};

// Individual message definitions
enum Message
{
    MSG_GET_HELP_INFO = 0,
    MSG_IPC_SOCKET_CONFIG_FLAG = 50,
    MSG_IPC_SOCKET_CONFIG_PUSH_DELAY,
    MSG_IPC_SOCKET_CONFIG_IP,

    MSG_GET_INDICATOR_INFO = 100,
    MSG_GET_METER_INFO = 101,
    MSG_GET_DRIVINFO_INFO = 102
};

// Function to get a human-readable string for a message group
inline const char *getMessageGroupName(MessageGroup group)
{
    switch (group)
    {
    case MSG_GROUP_0:
        return "Message Group 0";
    case MSG_GROUP_1:
        return "Message Group 1";
    case MSG_GROUP_2:
        return "Message Group 2";
    case MSG_GROUP_3:
        return "Message Group 3";
    case MSG_GROUP_4:
        return "Message Group 4";
    case MSG_GROUP_5:
        return "Message Group 5";
    case MSG_GROUP_6:
        return "Message Group 6";
    case MSG_GROUP_7:
        return "Message Group 7";
    case MSG_GROUP_8:
        return "Message Group 8";
    case MSG_GROUP_9:
        return "Message Group 9";
    case MSG_GROUP_10:
        return "Message Group 10";
    case MSG_GROUP_11:
        return "Message Group 11";
    case MSG_GROUP_12:
        return "Message Group 12";
    case MSG_GROUP_13:
        return "Message Group 13";
    case MSG_GROUP_14:
        return "Message Group 14";
    case MSG_GROUP_15:
        return "Message Group 15";
    default:
        return "Unknown Message Group";
    }
}

// Function to get a human-readable string for a message
inline const char *getMessageName(Message msg)
{
    switch (msg)
    {
    case MSG_GET_HELP_INFO:
        return "Get Help Info";
    case MSG_IPC_SOCKET_CONFIG_FLAG:
        return "IPC Socket Config Flag";
    case MSG_GET_INDICATOR_INFO:
        return "Get Indicator Info";
    case MSG_GET_METER_INFO:
        return "Get Meter Info";
    case MSG_GET_DRIVINFO_INFO:
        return "Get Driver Info";
    default:
        return "Unknown Message";
    }
}

#define MERGE_BYTES(byte1, byte2) ((static_cast<unsigned int>(byte1) << 8) | (static_cast<unsigned int>(byte2)))

#define SPLIT_TO_BYTES(value, byte1, byte2)                  \
    byte1 = static_cast<unsigned char>((value >> 8) & 0xFF); \
    byte2 = static_cast<unsigned char>(value & 0xFF);

#define MSG_GROUP_HELP MSG_GROUP_0
#define MSG_GROUP_SET MSG_GROUP_1
#define MSG_GROUP_CAR MSG_GROUP_11
/// @brief ///////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class DataMessage
{
public:
    // A constant for the fixed header value
    static constexpr uint16_t _HEADER_ = 0xA5A5; ///< Fixed header value indicating the start of a message

    uint16_t header;           ///< Header for identifying the message (usually fixed)
    uint8_t group;             ///< Group ID for categorizing the message type
    uint8_t msg;               ///< Message ID within the group
    uint16_t length;            ///< Length of the data in the message (max 255)
    std::vector<uint8_t> data; ///< Message data (content of the message)

    /**
     * @brief Default constructor for the DataMessage object.
     *
     * Initializes the header with the fixed HEADER value and other fields to 0.
     */
    DataMessage();
  
    DataMessage(const std::vector<uint8_t> &data_array);

    DataMessage(uint8_t group, uint8_t msg, const std::vector<uint8_t>& data_array);  // Constructor declaration
    /**
     * @brief Serializes the DataMessage object into a byte vector.
     *
     * Converts the message into a binary format suitable for sending over a communication channel.
     *
     * @return std::vector<uint8_t> The serialized byte vector representation of the message.
     */
    std::vector<uint8_t> serializeMessage() const;

    /**
     * @brief Deserializes a byte vector into a DataMessage object.
     *
     * Converts a serialized byte array back into a DataMessage object, which represents the original message.
     *
     * @param buffer The byte vector containing the serialized message data.
     * @return DataMessage The deserialized DataMessage object.
     */
    static DataMessage deserializeMessage(const std::vector<uint8_t> &buffer);

    /**
     * @brief Validates if the message has a valid structure.
     *
     * Checks that the message has a valid header, group ID, message ID, and data length.
     *
     * @return true if the message is valid, false otherwise.
     */
    bool isValid() const;

    /**
     * @brief Prints the contents of the DataMessage object.
     *
     * Outputs the message's header, group ID, message ID, length, and data in a human-readable format.
     *
     * @param tag A label to distinguish where the message is being printed from.
     */
    void printMessage(const std::string &tag) const;

    size_t get_base_length() const;

    size_t get_total_length() const; ///< Returns the total length of the serialized message (header + group + msg + length + data).
    
    size_t get_data_length() const; ///< Returns the length of the data portion of the message.
};

#endif // OCTOPUS_IPC_PTL_HANDLER_HPP
