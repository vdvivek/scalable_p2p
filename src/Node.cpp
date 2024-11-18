#include "Node.h"
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h> // For close()
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include "string"
// Node Constructor
Node::Node(std::string name, const std::string &ip, int port, std::pair<double, double> coords, 
           NetworkManager &networkManager, NodeType type)
    : id(generateUUID()), name(std::move(name)), ip(ip), port(port), coords(std::move(coords)), 
      networkManager(networkManager), nodeType(type) {
    socket_fd = -1;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
}


// Node Destructor
Node::~Node() {
    if (socket_fd >= 0) {
        close(socket_fd);
    }
}

std::string Node::getNodeTypeAsString() const {
    switch (nodeType) {
        case NodeType::Ground: return "Ground";
        case NodeType::Satellite: return "Satellite";
        default: return "Unknown";
    }
}


// Base implementation of printInfo
void Node::printInfo() const {
    std::cout << "Node: " << name << " (" << ip << ":" << port
              << ") [" << coords.first << ", " << coords.second << "]\n";
}

std::string Node::getId() const { return id; }
std::string Node::getName() const { return name; }
std::string Node::getIP() const { return ip; }
int Node::getPort() const { return port; }

std::pair<double, double> Node::getCoords() const {
    return coords;
}
void Node::setCoords(const std::pair<double, double> &newCoords) {
    coords = newCoords;
}

// Utility function to generate UUID
std::string Node::generateUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4"; // UUID version 4
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

std::string Node::extractMessage(const std::string &payload, std::string &senderName, std::string &targetIP, int &targetPort) {
    std::istringstream iss(payload);
    std::string portStr;
    std::string actualMessage;

    // Parse the payload (format: senderName targetIP targetPort message)
    if (iss >> senderName >> targetIP >> portStr) {
        try {
            targetPort = std::stoi(portStr); // Convert port to integer
        } catch (const std::exception &e) {
            std::cerr << "[ERROR] Invalid port in message payload: " << portStr << "\n";
            return "";
        }

        // Extract the remaining part of the payload as the actual message
        std::getline(iss, actualMessage);
        actualMessage.erase(0, actualMessage.find_first_not_of(" ")); // Trim leading spaces
    } else {
        std::cerr << "[ERROR] Malformed message payload: " << payload << "\n";
    }

    return actualMessage;
}

bool Node::bind() {
    // Create a UDP socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        std::cerr << "Failed to create socket for Node " << name << ": " << strerror(errno) << std::endl;
        return false;
    }

    // Bind the socket to the IP and Port
    if (::bind(socket_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Failed to bind socket for Node " << name << ": " << strerror(errno) << std::endl;
        close(socket_fd);
        socket_fd = -1;
        return false;
    }

    std::cout << "[NEXUS] Node " << name << " successfully bound to " << ip << ":" << port << std::endl;
    return true;
}

void Node::sendMessage(const std::string &targetName, const std::string &targetIP, int targetPort, const std::string &message) {
    struct sockaddr_in targetAddr = {};
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(targetPort);
    inet_pton(AF_INET, targetIP.c_str(), &targetAddr.sin_addr);

    // Construct the full message payload
    std::string fullMessage = getName() + " " + targetIP + " " + std::to_string(targetPort) + " " + message;

    const int bytesSent = sendto(socket_fd, fullMessage.c_str(), fullMessage.size(), 0,
                                 reinterpret_cast<struct sockaddr *>(&targetAddr), sizeof(targetAddr));
    if (bytesSent < 0) {
        std::cerr << "[ERROR] Failed to send message: " << strerror(errno) << std::endl;
    } else {
        std::cout << "[INFO] Sent message to " << targetName << " at " << targetIP << ":" << targetPort << std::endl;
    }
}

void Node::receiveMessage(std::string &message) {
    if (socket_fd < 0) {
        std::cerr << "[ERROR] Socket is not initialized for receiving messages." << std::endl;
        return;
    }

    constexpr size_t bufferSize = 256;
    char buffer[bufferSize];

    struct sockaddr_in senderAddr{};
    socklen_t senderAddrLen = sizeof(senderAddr);

    ssize_t bytesReceived = recvfrom(socket_fd, buffer, bufferSize - 1, 0,
                                     reinterpret_cast<struct sockaddr *>(&senderAddr), &senderAddrLen);

    if (bytesReceived < 0) {
        std::cerr << "[ERROR] Failed to receive message: " << strerror(errno) << std::endl;
        return;
    }

    // Null-terminate the buffer to treat it as a string
    buffer[bytesReceived] = '\0';
    message = std::string(buffer);

    // Log the raw message payload
    std::cout << "[DEBUG] Raw message received: \"" << message << "\"" << std::endl;

    // Extract and handle the message
    std::string senderName, targetIP;
    int targetPort = 0;
    const std::string actualMessage = extractMessage(message, senderName, targetIP, targetPort);

    if (!actualMessage.empty()) {
        std::cout << "[INFO] Message from " << senderName << " to " << targetIP << ":" << targetPort
                  << " - \"" << actualMessage << "\"" << std::endl;
    } else {
        std::cerr << "[ERROR] Malformed message payload: " << message << std::endl;
    }
}


