# Compiler and standard
CXX = g++
CXXFLAGS = -std=c++11 -pthread -g -Wno-psabi

# Detect OS
UNAME := $(shell uname)

# Paths to include and library directories
ifeq ($(UNAME), Darwin)
    JSONCPP_INC ?= /opt/homebrew/Cellar/jsoncpp/1.9.6/include
    JSONCPP_LIB ?= /opt/homebrew/Cellar/jsoncpp/1.9.6/lib
    CURL_INC ?= /opt/homebrew/opt/curl/include
    CURL_LIB ?= /opt/homebrew/opt/curl/lib
    ZLIB_INC ?= /usr/include
    ZLIB_LIB ?= /usr/lib
else ifeq ($(UNAME), Linux)
    JSONCPP_INC ?= $(HOME)/local/include
    JSONCPP_LIB ?= $(HOME)/local/lib
    CURL_INC ?= $(HOME)/local/include
    CURL_LIB ?= $(HOME)/local/lib
    ZLIB_INC ?= $(HOME)/local/include
    ZLIB_LIB ?= $(HOME)/local/lib
else
    $(error Unsupported operating system detected: $(UNAME))
endif

# Libraries to link
LIBS = -lcurl -ljsoncpp -lz

# Source and object files
NEXUS_SOURCES = nexus_main/main.cpp src/Logger.cpp src/Node.cpp src/NetworkManager.cpp src/Packet.cpp src/Utility.cpp
REGISTRY_SOURCES = registry_main/main.cpp src/Logger.cpp src/NexusRegistryServer.cpp src/Utility.cpp
NEXUS_OBJECTS = $(NEXUS_SOURCES:.cpp=.o)
REGISTRY_OBJECTS = $(REGISTRY_SOURCES:.cpp=.o)

# Targets
all: tidy nexus registry_server

tidy:
	clang-tidy $(NEXUS_SOURCES) $(REGISTRY_SOURCES) -p cmake-build-debug

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(JSONCPP_INC) -I$(CURL_INC) -I$(ZLIB_INC) -c $< -o $@

nexus: $(NEXUS_OBJECTS)
	$(CXX) $(CXXFLAGS) $(NEXUS_OBJECTS) -L$(JSONCPP_LIB) -L$(CURL_LIB) -L$(ZLIB_LIB) $(LIBS) -o $@

registry_server: $(REGISTRY_OBJECTS)
	$(CXX) $(CXXFLAGS) $(REGISTRY_OBJECTS) -L$(JSONCPP_LIB) -L$(CURL_LIB) -L$(ZLIB_LIB) $(LIBS) -o $@

clean:
	rm -f nexus registry_server $(NEXUS_OBJECTS) $(REGISTRY_OBJECTS)
