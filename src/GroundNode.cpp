#include "GroundNode.h"
#include "SatelliteNode.h"
#include <cmath>
#include <iostream>

GroundNode::GroundNode(std::string name, const std::string &ip, int port, double x, double y, NetworkManager &networkManager)
    : Node(std::move(name), ip, port, x, y, networkManager) {}

std::shared_ptr<SatelliteNode> GroundNode::findNearestSatellite() const {
    double minDistance = std::numeric_limits<double>::max();
    std::shared_ptr<SatelliteNode> nearestSatellite = nullptr;

    for (const auto& node : networkManager.getNodes()) {
        auto satellite = std::dynamic_pointer_cast<SatelliteNode>(node);
        if (satellite) {
            double distance = std::sqrt(std::pow(satellite->getX() - x, 2) + std::pow(satellite->getY() - y, 2));
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
