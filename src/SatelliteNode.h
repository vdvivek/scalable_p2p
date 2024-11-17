#ifndef SATELLITENODE_H
#define SATELLITENODE_H

#include "Node.h"
#include <random>

class SatelliteNode: public Node {
public:
    SatelliteNode(std::string name, const std::string &ip, int port, double x, double y, NetworkManager &networkManager);

    void updatePosition() override;

    // TODO: Sample routing function provided below, might need enhancements.
    void receiveMessage(std::string &message) override;

private:
    double delay;

    void simulateSignalDelay();
};

#endif //SATELLITENODE_H
