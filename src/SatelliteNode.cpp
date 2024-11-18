#include "SatelliteNode.h"
#include "Utility.h"
#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>

SatelliteNode::SatelliteNode(std::string name, const std::string &ip, int port, std::pair<double, double> coords, NetworkManager &networkManager)
    : Node(std::move(name), ip, port, std::move(coords), networkManager), delay(0) {}


void SatelliteNode::updatePosition() {
    double speedX = 0.05; // Example: Horizontal speed
    double speedY = 0.1;  // Example: Vertical speed
    coords.first = roundToTwoDecimalPlaces(coords.first + speedX);
    coords.second = roundToTwoDecimalPlaces(coords.second + speedY);
}

void SatelliteNode::simulateSignalDelay() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.1, 2.0);
    delay =  dis(gen);
}

void SatelliteNode::receiveMessage(std::string &message) {
    if (message.empty()) {
    // No message received yet on satellite, so skip
        return;
    }
    simulateSignalDelay();
    std::cout << "[INFO] Satellite " << name << " relaying message after " << delay << " seconds delay." << std::endl;

    // Simulate the delay
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(delay * 1000)));

    // Extract target information from the message
    std::string senderName, targetIP;
    int targetPort;
    std::string actualMessage = Node::extractMessage(message, senderName, targetIP, targetPort);

    if (!actualMessage.empty()) {
        // Relay the message to the target
        std::cout << "[INFO] Satellite " << name << " forwarding message to target IP " << targetIP
                  << " on port " << targetPort << std::endl;
        sendMessage(senderName, targetIP, targetPort, actualMessage);
    } else {
        std::cerr << "[ERROR] Failed to parse message for forwarding." << std::endl;
    }
}

