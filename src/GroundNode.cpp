#include "NetworkManager.h"
#include "GroundNode.h"
#include "SatelliteNode.h"
#include <cmath>
#include <iostream>

GroundNode::GroundNode(std::string name, const std::string& ip, int port, std::pair<double, double> coords, 
                       NetworkManager& networkManager, Node::NodeType type)
    : Node(std::move(name), ip, port, std::move(coords), networkManager, type) {}

std::shared_ptr<SatelliteNode> GroundNode::findNearestSatellite() const {
    double minDistance = std::numeric_limits<double>::max();
    std::shared_ptr<SatelliteNode> nearestSatellite = nullptr;

    std::cout << "[DEBUG] Finding nearest satellite for " << getName() << std::endl;

    for (const auto& node : networkManager.getSatelliteNodes()) {
        auto satellite = std::dynamic_pointer_cast<SatelliteNode>(node);
        if (satellite) {
            auto satelliteCoords = satellite->getCoords();
            double distance = std::sqrt(
                std::pow(satelliteCoords.first - coords.first, 2) +
                std::pow(satelliteCoords.second - coords.second, 2)
            );

            std::cout << "[DEBUG] Distance to satellite " << satellite->getName()
                      << ": " << distance << std::endl;

            if (distance < minDistance) {
                minDistance = distance;
                nearestSatellite = satellite;
            }
        } else {
            std::cerr << "[ERROR] Node " << node->getName() << " is not a satellite." << std::endl;
        }
    }

    if (nearestSatellite) {
        std::cout << "[DEBUG] Nearest satellite to " << getName()
                  << " is " << nearestSatellite->getName()
                  << " at distance " << minDistance << std::endl;
    } else {
        std::cerr << "[ERROR] No satellites found for " << getName() << std::endl;
    }

    return nearestSatellite;
}
void GroundNode::updateRoutingTable() {
    std::cout << "[DEBUG] Updating routing table for " << getName() << std::endl;

    routingTable.clear();

    for (const auto& node : networkManager.getSatelliteNodes()) {
        if (node->getName() == getName()) {
            continue; // Skip self
        }
        std::cout<<""<<std::endl;

        auto nearestSatellite = findNearestSatellite();
        if (nearestSatellite) {
            routingTable[node->getName()] = nearestSatellite->getName();
            std::cout << "[DEBUG] Route to " << node->getName()
                      << " via " << nearestSatellite->getName() << std::endl;
        } else {
            std::cerr << "[ERROR] No satellite found for " << node->getName() << std::endl;
        }
    }

    std::cout << "[DEBUG] Routing table updated for " << getName() << std::endl;
}



void GroundNode::sendMessage(const std::string& targetName, const std::string& targetIP, int targetPort, const std::string& message) {
    if (routingTable.find(targetName) == routingTable.end()) {
        std::cerr << "[ERROR] No route found to target " << targetName << ". Updating routing table." << std::endl;
        updateRoutingTable();
    }

    // Find the satellite for the target node
    std::string satelliteName = routingTable[targetName];
    auto satellite = networkManager.getNodeByName(satelliteName);

    if (satellite) {
        std::cout << "[INFO] Relaying message via satellite: " << satellite->getName() << std::endl;
        satellite->sendMessage(targetName, targetIP, targetPort, message);
    } else {
        std::cerr << "[ERROR] Satellite not found: " << satelliteName << std::endl;
    }
}

