# Collect source files, excluding executables
file(GLOB IPC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

# Remove server and client from the shared library source list
list(REMOVE_ITEM IPC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/octopus_ipc_server.cpp)
list(REMOVE_ITEM IPC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/octopus_ipc_client.cpp)

# Create shared library OIPC
add_library(OIPC SHARED ${IPC_SOURCES})

# Create executable for server
add_executable(octopus_ipc_server ${CMAKE_CURRENT_SOURCE_DIR}/octopus_ipc_server.cpp)

# Create executable for client
add_executable(octopus_ipc_client ${CMAKE_CURRENT_SOURCE_DIR}/octopus_ipc_client.cpp)

# Link executables with OIPC shared library
target_link_libraries(octopus_ipc_server PRIVATE OIPC)
target_link_libraries(octopus_ipc_client PRIVATE OIPC)

# Link dl (dynamic linking) and pthread (thread library) for server
target_link_libraries(octopus_ipc_server PRIVATE dl pthread)
# Link pthread for client if it uses threads
target_link_libraries(octopus_ipc_client PRIVATE pthread)

# Link pthread to OIPC library if it uses threads internally
target_link_libraries(OIPC PRIVATE pthread)

# Make headers of OIPC visible to other modules
target_include_directories(OIPC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Set install path for the OIPC library
install(TARGETS OIPC DESTINATION /usr/local/lib)

# Install IPC header files
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION /usr/local/include
        FILES_MATCHING PATTERN "*.hpp")

# Install executables to /usr/local/bin
install(TARGETS octopus_ipc_server octopus_ipc_client
        DESTINATION /usr/local/bin)

# Install systemd service file
#install(
#    FILES ${CMAKE_CURRENT_SOURCE_DIR}/octopus_ipc_server.service
#    DESTINATION /etc/systemd/system
#)
