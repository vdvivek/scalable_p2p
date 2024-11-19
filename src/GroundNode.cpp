#include "GroundNode.h"
#include "SatelliteNode.h"
#include <cmath>
#include <iostream>
#include "Utility.h"
#include "NodeType.h"

GroundNode::GroundNode(NodeType::Type nodeType, std::string name, const std::string &ip, int port, std::pair<double, double> coords, NetworkManager &networkManager)
    : Node(nodeType, std::move(name), ip, port, std::move(coords), networkManager) {}

void GroundNode::updatePosition() {
    coords.first = roundToTwoDecimalPlaces(coords.first);
    coords.second = roundToTwoDecimalPlaces(coords.second);
    std::cout << NodeType::toString(this->type) << " " << this->name << "is stationary at " << "(" << this->coords.first << ", " << this->coords.second << ")" << std::endl;
}

std::shared_ptr<SatelliteNode> GroundNode::findNearestSatellite() const {
    double minDistance = std::numeric_limits<double>::max();
    std::shared_ptr<SatelliteNode> nearestSatellite = nullptr;

    for (const auto& node : networkManager.getSatelliteNodes()) {
        std::cout << "[DEBUG2] Node type: " << static_cast<int>(node->getType()) << std::endl;
        auto satellite = std::dynamic_pointer_cast<SatelliteNode>(node);

        if (satellite) {
            std::cout << "[DEBUG2] satellite node " << satellite->getName() << std::endl;

            auto satelliteCoords = satellite->getCoords();
            double distance = std::sqrt(std::pow(satelliteCoords.first - coords.first, 2) +
                                        std::pow(satelliteCoords.second - coords.second, 2));
            if (distance < minDistance) {
                minDistance = distance;
                nearestSatellite = satellite;
            }
        }
    }
    std::cout << "[DEBUG2] nearest satellite node " << nearestSatellite->getName() << std::endl;
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
