#include "GroundNode.h"
#include "SatelliteNode.h"
#include <cmath>
#include <iostream>

GroundNode::GroundNode(std::string name, const std::string &ip, int port, std::pair<double, double> coords, NetworkManager &networkManager)
    : Node(std::move(name), ip, port, std::move(coords), networkManager) {}

std::shared_ptr<SatelliteNode> GroundNode::findNearestSatellite() const {
    double minDistance = std::numeric_limits<double>::max();
    std::shared_ptr<SatelliteNode> nearestSatellite = nullptr;

    // Iterate through the list of satellite nodes to find the nearest
    for (const auto& node : networkManager.getSatelliteNodes()) {
        auto satellite = std::dynamic_pointer_cast<SatelliteNode>(node);
        if (satellite) {
            auto satelliteCoords = satellite->getCoords();
            double distance = std::sqrt(std::pow(satelliteCoords.first - coords.first, 2) +
                                        std::pow(satelliteCoords.second - coords.second, 2));
            if (distance < minDistance) {
                minDistance = distance;
                nearestSatellite = satellite;
            }
        }
    }

    return nearestSatellite;
}

void GroundNode::sendMessage(const std::string &targetName, const std::string &targetIP, int targetPort, const std::string &message) {
    auto nearestSatellite = findNearestSatellite();
    if (nearestSatellite) {
        std::cout << "[INFO] Relaying message to the nearest satellite: " << nearestSatellite->getName() << std::endl;
        nearestSatellite->sendMessage(targetName, targetIP, targetPort, message);
    } else {
        std::cerr << "[ERROR] No satellite available for relaying the message." << std::endl;
    }
}
