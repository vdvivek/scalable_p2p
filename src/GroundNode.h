#ifndef GROUNDNODE_H
#define GROUNDNODE_H

#include "Node.h"
#include "SatelliteNode.h"
#include <memory>

class GroundNode : public Node {
public:
    GroundNode(std::string name, const std::string& ip, int port, std::pair<double, double> coords, 
               NetworkManager& networkManager, Node::NodeType type);

    void sendMessage(const std::string& targetName, const std::string& targetIP, int targetPort, const std::string& message) override;
    void updateRoutingTable();

private:
    std::unordered_map<std::string, std::string> routingTable; // targetName -> satelliteName
    std::shared_ptr<SatelliteNode> findNearestSatellite() const;
};

#endif // GROUNDNODE_H
