// #ifndef SATELLITENODE_H
// #define SATELLITENODE_H

// #include "Node.h"
// #include <random>

// class SatelliteNode: public Node {
// public:
//     SatelliteNode(std::string name, const std::string &ip, int port, std::pair<double, double> coords, NetworkManager &networkManager);

//     void updatePosition() override;

//     // TODO: Sample routing function provided below, might need enhancements.
//     void receiveMessage(std::string &message) override;

// private:
//     double delay;

//     void simulateSignalDelay();
// };

// #endif //SATELLITENODE_H
#ifndef SATELLITENODE_H
#define SATELLITENODE_H

#include "Node.h"
#include <random>

class SatelliteNode : public Node {
public:
    SatelliteNode(std::string name, const std::string& ip, int port, std::pair<double, double> coords, 
                  NetworkManager& networkManager, Node::NodeType type);


    void printInfo() const override {
        std::cout << "SatelliteNode: " << name << " (" << ip << ":" << port
                  << ") [" << coords.first << ", " << coords.second << "]\n";
    }
    //SatelliteNode(std::string name, const std::string& ip, int port, std::pair<double, double> coords, NetworkManager& networkManager);

    void updatePosition();
    void receiveMessage(std::string& message) override;

private:
    void simulateSignalDelay();

    double delay; // Simulated signal delay
};

#endif // SATELLITENODE_H
