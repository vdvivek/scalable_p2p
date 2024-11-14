#ifndef SATELLITENODE_H
#define SATELLITENODE_H

#include "Node.h"
#include <random>

class SatelliteNode : public Node {
public:
    SatelliteNode(int id, const std::string &name, double orbitRadius, double orbitSpeed, double delay, double range, double bandwidth);

    void updatePosition() override;
    bool connect(std::shared_ptr<Node> other) override;
    bool transmitMessage(const std::string &message, std::shared_ptr<Node> targetNode);

private:
    double orbitRadius;
    double orbitSpeed;
    double angle;        // Angle for calculating orbital position
    double delay;        // Base communication delay
    double range;        // Communication range
    double bandwidth;    // Data transmission rate (bits per second)
    double signalLossProbability;

    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution; // For simulating signal loss
};

#endif
