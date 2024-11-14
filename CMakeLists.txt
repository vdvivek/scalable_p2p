cmake_minimum_required(VERSION 3.29)
project(scalable_p2p)

set(CMAKE_CXX_STANDARD 14)

add_executable(scalable_p2p
    main.cpp
        src/Node/Node.cpp
        src/Node/Node.h
        src/Node/GroundNode.cpp
        src/Node/GroundNode.h
        src/Node/SatelliteNode.cpp
        src/Node/SatelliteNode.h)
