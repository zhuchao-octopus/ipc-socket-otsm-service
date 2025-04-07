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
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialize the DataMessage into a binary format for transmission
std::vector<uint8_t> DataMessage::serialize() const {
    std::vector<uint8_t> buffer;

    // Add group (1 byte)
    buffer.push_back(group);

    // Add message type (1 byte)
    buffer.push_back(msg);

    // Add data length (2 bytes)
    ///uint16_t data_length = static_cast<uint16_t>(data.size());
    ///buffer.push_back(static_cast<uint8_t>((data_length >> 8) & 0xFF)); // High byte
    ///buffer.push_back(static_cast<uint8_t>(data_length & 0xFF));        // Low byte

    // Add data array (each element 1 byte)
    ///for (uint8_t val : data) {
    ///    buffer.push_back(val);
    ///}
    buffer.insert(buffer.end(), data.begin(), data.end());
    return buffer;
}
// Deserialize the binary data into a DataMessage structure
DataMessage DataMessage::deserialize(const std::vector<uint8_t>& buffer) {
    DataMessage msg;
    size_t index = 0;

    // Parse group (1 byte)
    msg.group = buffer[index++];

    // Parse message type (1 byte)
    msg.msg = buffer[index++];

    // Parse data array elements (remaining elements)
    ///while (index < buffer.size()) {
    ///    msg.data.push_back(buffer[index++]);
    ///}

    msg.data.assign(buffer.begin() + index, buffer.end());
    return msg;
}

void DataMessage::print() const
{
    std::cout << "DataMessage "
              << "Group: " << static_cast<int>(group) << " "
              << "Msg: " << static_cast<int>(msg) << " "
              << "Data (" << data.size() << " bytes): ";

    for (size_t i = 0; i < data.size(); ++i)
    {
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(data[i]) << " ";
    }

    std::cout << std::dec << std::endl;
}
