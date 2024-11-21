cmake_minimum_required(VERSION 3.25)
project(scalable_p2p)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(CURL REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(OpenSSL REQUIRED)

# Find jsoncpp headers and libraries
include_directories(/opt/homebrew/Cellar/jsoncpp/1.9.6/include)
include_directories(/opt/homebrew/opt/curl/include)
link_directories(/opt/homebrew/Cellar/jsoncpp/1.9.6/lib)

# Shared source files
set(SHARED_SOURCES
        src/Node.cpp
        src/Logger.cpp
        src/CryptoManager.cpp
        src/NetworkManager.cpp
)

set(NEXUS_SRC
        src/Packet.cpp
        src/Utility.cpp
        src/NodeType.h
)

set(REGISTRY_SRC
        src/Utility.cpp
)

# Nexus executable
add_executable(nexus nexus_main/main.cpp ${SHARED_SOURCES} ${NEXUS_SRC})
if(${CURL_FOUND} AND ${jsoncpp_FOUND} AND ${OPENSSL_FOUND})
        target_link_libraries(nexus CURL::libcurl jsoncpp OpenSSL::SSL OpenSSL::Crypto)
else()
        target_link_libraries(nexus jsoncpp /opt/homebrew/opt/curl/lib/libcurl.dylib /opt/homebrew/opt/openssl/lib/libssl.dylib /opt/homebrew/opt/openssl/lib/libcrypto.dylib)
endif()

# Registry Server executable
add_executable(registry_server registry_main/main.cpp src/NexusRegistryServer.cpp ${REGISTRY_SRC})
if(${CURL_FOUND} AND ${jsoncpp_FOUND} AND ${OPENSSL_FOUND})
        target_link_libraries(registry_server CURL::libcurl jsoncpp OpenSSL::SSL OpenSSL::Crypto)
else()
        target_link_libraries(registry_server jsoncpp /opt/homebrew/opt/curl/lib/libcurl.dylib /opt/homebrew/opt/openssl/lib/libssl.dylib /opt/homebrew/opt/openssl/lib/libcrypto.dylib)
endif()

# Include directories for source files
target_include_directories(nexus PRIVATE "${PROJECT_SOURCE_DIR}/src/")
target_include_directories(registry_server PRIVATE "${PROJECT_SOURCE_DIR}/src/")