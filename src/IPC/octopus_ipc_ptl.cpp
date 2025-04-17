// File: octopus_ipc_ptl.cpp
// Description: This file implements a custom data exchange format for inter-process communication (IPC)
//              using Unix domain sockets. It supports both message serialization and deserialization.
//              The messages consist of a message ID, command type, and an array of integers as the data.
//              The file contains both message sending and receiving functionalities using Unix domain sockets.

#include <iostream>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "octopus_ipc_ptl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//[Header:2字节][Group:1字节][Msg:1字节][Length:2字节][Data:Length字节]
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialize the DataMessage into a binary format for transmission

DataMessage::DataMessage()
    : header(_HEADER_), group(0), msg(0), length(0)
{
    // Default constructor initializes the header with HEADER and all other fields to 0.
}
DataMessage::DataMessage(const std::vector<uint8_t> &data_array)
{
    size_t baseSize = sizeof(this->header) + sizeof(this->group) + sizeof(this->msg) + sizeof(this->length);

    // Ensure there is enough data for header, group, and msg
    if (data_array.size() < baseSize)
    {
        std::cerr << "DataMessage Insufficient data to deserialize." << std::endl;
        return;
    }

    // Extract header (2 bytes)
    header = _HEADER_;

    // Extract group (1 byte)
    group = data_array[0]; // group is the first byte (index 0)

    // Extract msg (1 byte)
    msg = data_array[1]; // msg is the second byte (index 1)

    // Extract the data portion (starting from index 2 onwards)
    this->data.assign(data_array.begin() + 2, data_array.end());

    // Update the length based on the size of the data portion
    length = this->data.size();

    // Check if the data size is correct (the size of the data portion should match the remaining size in the array)
    // if (data_array.size() != baseSize + length)
    //{
    //    std::cerr << "DataMessage Data size mismatch during deserialization." << std::endl;
    //    return;
    //}
}

DataMessage::DataMessage(uint8_t group, uint8_t msg, const std::vector<uint8_t> &data_array)
{
    ///size_t head_Size = sizeof(this->header) + sizeof(this->group) + sizeof(this->msg) + sizeof(this->length);

    /// Ensure there is enough data for header, group, and msg
    ///if (data_array.size() < baseSize)
    ///{
    ///    std::cerr << "DataMessage Insufficient data to deserialize." << std::endl;
    ///    return;
    ///}

    // Extract header (2 bytes)
    this->header = _HEADER_;

    // Extract group (1 byte)
    this->group = group; // group is the first byte (index 0)

    // Extract msg (1 byte)
    this->msg = msg; // msg is the second byte (index 1)

    // Extract the data portion (starting from index 2 onwards)
    this->data.assign(data_array.begin(), data_array.end());

    // Update the length based on the size of the data portion
    this->length = this->data.size();
}
/**
 * @brief Serializes the DataMessage object into a byte vector.
 *
 * This function converts the DataMessage into a sequence of bytes, which can be transmitted over a communication interface.
 *
 * @return std::vector<uint8_t> A vector containing the serialized byte representation of the message.
 */
std::vector<uint8_t> DataMessage::serializeMessage() const
{
    std::vector<uint8_t> serializedData;

    // Add header (2 bytes)
    serializedData.push_back(static_cast<uint8_t>(header >> 8));   // High byte of header
    serializedData.push_back(static_cast<uint8_t>(header & 0xFF)); // Low byte of header

    // Add group (1 byte)
    serializedData.push_back(group);

    // Add msg (1 byte)
    serializedData.push_back(msg);

    // Add length (2 bytes)
    serializedData.push_back(static_cast<uint8_t>(length >> 8));   // High byte
    serializedData.push_back(static_cast<uint8_t>(length & 0xFF)); // Low byte

    // Add data elements
    serializedData.insert(serializedData.end(), data.begin(), data.end());

    return serializedData;
}

/**
 * @brief Deserializes a byte vector into a DataMessage object.
 *
 * This function converts a serialized byte array back into a DataMessage object.
 * The byte array is expected to have the header, group ID, message ID, length, and data.
 *
 * @param buffer The byte vector to be deserialized.
 * @return DataMessage The resulting DataMessage object after deserialization.
 * @throws std::runtime_error If the byte vector is insufficient in size or has invalid data.
 */
DataMessage DataMessage::deserializeMessage(const std::vector<uint8_t> &buffer)
{
    DataMessage msg;
    size_t baseSize = sizeof(msg.header) + sizeof(msg.group) + sizeof(msg.msg) + sizeof(msg.length);

    if (buffer.size() < baseSize)
    {
        // throw std::runtime_error("Insufficient data to deserialize.");
        //std::cerr << "DataMessage Error during deserialization" << std::endl;
        return msg;
    }

    // Extract header (2 bytes)
    msg.header = (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];

    // Extract group (1 byte)
    msg.group = buffer[2];

    // Extract msg (1 byte)
    msg.msg = buffer[3];

    // Extract length (2 bytes)
    msg.length = (static_cast<uint16_t>(buffer[4]) << 8) | buffer[5];

    // Check if remaining buffer matches length
    // if (buffer.size() < (baseSize + msg.length))
    //{
    // throw std::runtime_error("Invalid data size, cannot deserialize.");
    //    std::cerr << "DataMessage Invalid data size, cannot deserialize." << std::endl;
    //}

    // Only extract data if buffer is large enough
    if (buffer.size() >= baseSize + msg.length)
    {
        msg.data.assign(buffer.begin() + baseSize, buffer.begin() + baseSize + msg.length);
    }

    return msg;
}

/**
 * @brief Validates if the message has a valid structure.
 *
 * This function checks if the message has the correct header and if the data length is within the acceptable range.
 *
 * @return true if the message is valid, false otherwise.
 */
bool DataMessage::isValid() const
{
    // size_t baseSize = sizeof(msg.header) + sizeof(msg.group) + sizeof(msg.msg) + sizeof(msg.length);
    return (header == _HEADER_ && length == data.size() && group >= 0 && msg >= 0);
}

/**
 * @brief Prints the contents of the DataMessage object for debugging purposes.
 *
 * This function displays the message's header, group ID, message ID, length, and the data in a formatted manner.
 *
 * @param tag A label to help identify which part of the code is printing the message.
 */
void DataMessage::printMessage(const std::string &tag) const
{
    std::cout << tag << ": Header 0x" << std::hex << std::setw(2) << std::setfill('0')
              << header
              << ", Group: 0x" << static_cast<int>(group)
              << ", Msg: 0x" << static_cast<int>(msg)
              << ", Length: " << std::dec << static_cast<int>(length)
              << ", Data: ";

    for (auto byte : data)
    {
        std::cout << std::hex << "0x" << static_cast<int>(byte) << " ";
    }
    std::cout << std::dec << std::endl;
}

/**
 * @brief Returns the total length of the serialized message.
 *
 * The total length includes the header (2 bytes), group (1 byte), message ID (1 byte),
 * length (1 byte), and the data (size of the vector).
 *
 * @return size_t The total length of the message.
 */
size_t DataMessage::get_total_length() const
{
    return sizeof(header) + sizeof(group) + sizeof(msg) + sizeof(length) + data.size();
    // return sizeof(header) + sizeof(group) + sizeof(msg) + sizeof(length) + data.length();
}

size_t DataMessage::get_base_length() const
{
    return sizeof(header) + sizeof(group) + sizeof(msg) + sizeof(length);
}

/**
 * @brief Returns the length of the data portion of the message.
 *
 * This function returns the size of the data vector, which holds the actual message content.
 *
 * @return size_t The length of the data portion of the message.
 */
size_t DataMessage::get_data_length() const
{
    return data.size();
}
