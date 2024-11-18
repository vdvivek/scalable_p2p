#ifndef GROUNDNODE_H
#define GROUNDNODE_H

#include "Node.h"

class SatelliteNode;

class GroundNode: public Node {
public:
    GroundNode(std::string name, const std::string &ip, int port, std::pair<double, double> coords, NetworkManager &networkManager);

    void sendMessage(const std::string &targetName, const std::string &targetIP, int targetPort, const std::string &message) override;

private:
    std::shared_ptr<SatelliteNode> findNearestSatellite() const;

};

#endif //GROUNDNODE_H
