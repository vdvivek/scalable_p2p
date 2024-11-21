# Compiler and standard
CXX = g++
CXXFLAGS = -std=c++11 -pthread -g -Wno-psabi

# Detect OS
UNAME := $(shell uname)

# Paths to include and library directories
ifeq ($(UNAME), Darwin)  # macOS
    JSONCPP_INC = /opt/homebrew/Cellar/jsoncpp/1.9.6/include
    JSONCPP_LIB = /opt/homebrew/Cellar/jsoncpp/1.9.6/lib
    CURL_INC = /opt/homebrew/opt/curl/include
    CURL_LIB = /opt/homebrew/opt/curl/lib
    ZLIB_INC = /usr/include
    ZLIB_LIB = /usr/lib
else ifeq ($(UNAME), Linux)  # Linux (Ubuntu/Debian)
    JSONCPP_INC = $(HOME)/local/include
    JSONCPP_LIB = $(HOME)/local/lib
    CURL_INC = $(HOME)/local/include
    CURL_LIB = $(HOME)/local/lib
    ZLIB_INC = $(HOME)/local/include
    ZLIB_LIB = $(HOME)/local/lib
else
    $(error Unsupported operating system detected: $(UNAME))
endif

# Libraries to link
LIBS = -lcurl -ljsoncpp -lz

# Source files
NEXUS_SOURCES = nexus_main/main.cpp src/Logger.cpp src/Node.cpp src/NetworkManager.cpp src/Packet.cpp src/Utility.cpp
REGISTRY_SOURCES = registry_main/main.cpp src/Logger.cpp src/NexusRegistryServer.cpp src/Utility.cpp

# Targets
all: nexus registry_server

nexus: $(NEXUS_SOURCES)
	$(CXX) $(CXXFLAGS) -I$(JSONCPP_INC) -I$(CURL_INC) -I$(ZLIB_INC) \
	-L$(JSONCPP_LIB) -L$(CURL_LIB) -L$(ZLIB_LIB) \
	$(NEXUS_SOURCES) $(LIBS) -o nexus

registry_server: $(REGISTRY_SOURCES)
	$(CXX) $(CXXFLAGS) -I$(JSONCPP_INC) -I$(CURL_INC) -I$(ZLIB_INC) \
	-L$(JSONCPP_LIB) -L$(CURL_LIB) -L$(ZLIB_LIB) \
	$(REGISTRY_SOURCES) $(LIBS) -o registry_server

clean:
	rm -f nexus registry_server
