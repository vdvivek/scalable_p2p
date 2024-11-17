cmake_minimum_required(VERSION 3.29)
project(scalable_p2p)

set(CMAKE_CXX_STANDARD 11)

# Find jsoncpp headers and libraries
include_directories(/opt/homebrew/Cellar/jsoncpp/1.9.6/include)
include_directories(/opt/homebrew/opt/curl/include)
link_directories(/opt/homebrew/Cellar/jsoncpp/1.9.6/lib)

# Shared source files
set(SHARED_SOURCES
        src/Node.cpp
        src/NetworkManager.cpp
)

# Nexus executable
add_executable(nexus nexus_main/main.cpp ${SHARED_SOURCES})
target_link_libraries(nexus jsoncpp /opt/homebrew/opt/curl/lib/libcurl.dylib)

# Registry Server executable
add_executable(registry_server registry_main/main.cpp src/NexusRegistryServer.cpp)
target_link_libraries(registry_server jsoncpp /opt/homebrew/opt/curl/lib/libcurl.dylib)