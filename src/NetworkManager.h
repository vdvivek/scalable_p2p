// #ifndef NETWORKMANAGER_H
// #define NETWORKMANAGER_H

// #include <string>
// #include <vector>
// #include <memory>

// class Node;

// class NetworkManager {
// public:
//     explicit NetworkManager(const std::string& registryAddress);

//     void addNode(const std::shared_ptr<Node>& node);
//     void removeNode(const std::string &id);
//     void listNodes() const;

//     std::vector<std::shared_ptr<Node>> getSatelliteNodes() const;

//     bool registerNodeWithRegistry(const std::shared_ptr<Node>& node);
//     void deregisterNodeWithRegistry(const std::shared_ptr<Node>& node);
//     void fetchNodesFromRegistry();

// private:
//     std::vector<std::shared_ptr<Node>> nodes;
//     std::string registryAddress;
// };

// #endif
#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H
#include "Node.h"

#include <string>
#include <vector>
#include <memory>
#include <mutex>



class NetworkManager {
public:
    explicit NetworkManager(const std::string& registryAddress);
    
    void addNode(const std::shared_ptr<Node>& node);
    void removeNode(const std::string& id);
    std::shared_ptr<Node> getNodeByName(const std::string& name) const;
    std::shared_ptr<Node> getNextHop(const std::shared_ptr<Node>& current, const std::string& targetIP, int targetPort);
    void listNodes() const;
    std::vector<std::shared_ptr<Node>> getSatelliteNodes() const;
    bool registerNodeWithRegistry(const std::shared_ptr<Node>& node);
    void deregisterNodeWithRegistry(const std::shared_ptr<Node>& node);
    void fetchNodesFromRegistry();
    std::shared_ptr<Node> createNode(Node::NodeType type, const std::string& name, const std::string& ip,
                                     int port, const std::pair<double, double>& coords);


    // Routing functionality
    //std::vector<std::shared_ptr<Node>> calculateShortestPath(const std::shared_ptr<Node>& source, const std::shared_ptr<Node>& target);

private:
    std::string registryAddress;
    std::vector<std::shared_ptr<Node>> nodes;
    mutable std::mutex nodesMutex;
};

#endif // NETWORKMANAGER_H
