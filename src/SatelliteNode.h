#ifndef SATELLITENODE_H
#define SATELLITENODE_H

#include "Node.h"
#include <random>

class SatelliteNode: public Node {
public:
    SatelliteNode(NodeType::Type nodeType, std::string name, const std::string &ip, int port, std::pair<double, double> coords, NetworkManager &networkManager);

    void updatePosition() override;

    // TODO: Sample routing function provided below, might need enhancements.
    void receiveMessage(std::string &message) override;

private:
    double delay;
    double speedX = 0.05;
    double speedY = 0.1;

    void simulateSignalDelay();
};

#endif //SATELLITENODE_H
