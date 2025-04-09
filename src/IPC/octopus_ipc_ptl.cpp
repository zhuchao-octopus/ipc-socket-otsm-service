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
//[Header:2字节][Group:1字节][Msg:1字节][Length:1字节][Data:Length字节]
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialize the DataMessage into a binary format for transmission
std::vector<uint8_t> DataMessage::serialize() const
{
    std::vector<uint8_t> serialized_data;

    // Add header (2 bytes)
    serialized_data.push_back(static_cast<uint8_t>(_HEADER_ >> 8));   // high byte of header
    serialized_data.push_back(static_cast<uint8_t>(_HEADER_ & 0xFF)); // low byte of header

    // Add group (1 byte)
    serialized_data.push_back(group);

    // Add msg (1 byte)
    serialized_data.push_back(msg);

    // Add length (2 bytes)
    serialized_data.push_back(static_cast<uint8_t>(length >> 8));   // high byte of length
    serialized_data.push_back(static_cast<uint8_t>(length & 0xFF)); // low byte of length

    // Add data elements
    serialized_data.insert(serialized_data.end(), data.begin(), data.end());

    return serialized_data;
}
// Deserialize the binary data into a DataMessage structure
DataMessage DataMessage::deserialize(const std::vector<uint8_t> &buffer)
{
    DataMessage msg;

    // Extract header (2 bytes)
    msg.header = (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];

    // Extract group (1 byte)
    msg.group = buffer[2];

    // Extract msg (1 byte)
    msg.msg = buffer[3];

    // Extract length (2 bytes)
    msg.length = (static_cast<uint16_t>(buffer[4]) << 8) | buffer[5];

    // Extract the data (length bytes)
    msg.data.assign(buffer.begin() + 6, buffer.begin() + 6 + msg.length);

    return msg;
}

bool DataMessage::is_valid_packet()
{
    if (header != _HEADER_)
        return false;

    if (data.size() != length)
        return false;

    if (group < 0)
        return false;

    if (msg < 0)
        return false;

    return true;
}
size_t DataMessage::get_total_packet_size() const
{
    return sizeof(header) + sizeof(group) + sizeof(msg) + sizeof(length) + data.size();
}
void DataMessage::print(std::string tag) const
{
    std::cout << tag << " - Header: 0x" << std::hex << std::setw(2) << std::setfill('0')
              << header
              << ", Group: 0x" << static_cast<int>(group)
              << ", Msg: 0x" << static_cast<int>(msg)
              << ", Length: " << std::dec << length
              << ", Data: ";

    for (auto byte : data)
    {
        std::cout << std::hex << "0x" << static_cast<int>(byte) << " ";
    }
    std::cout << std::dec << std::endl;
}
