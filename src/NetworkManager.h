#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <cmath>
#include <memory>
#include <string>
#include <vector>

using matrix = std::vector<std::vector<int>>;

class Node;

class NetworkManager {
public:
  explicit NetworkManager(const std::string &registryAddress);

  void addNode(const std::shared_ptr<Node> &node);
  void removeNode(const std::string &id);
  void listNodes() const;

  std::vector<std::shared_ptr<Node>> getSatelliteNodes() const;

  bool registerNodeWithRegistry(const std::shared_ptr<Node> &node);
  void deregisterNodeWithRegistry(const std::shared_ptr<Node> &node);
  void fetchNodesFromRegistry();

  void createRoutingTable();
  void updateRoutingTable(const std::shared_ptr<Node> &node);

  void route(int src_idx);
  std::shared_ptr<Node> getNextHop(const std::string &name);

  std::vector<int> nextHop;

private:
  matrix topology;
  // Holds index for next hop for given destination
  // nextHop[S3 idx] = next node in the shortest path to S3
  std::vector<std::shared_ptr<Node>> nodes;
  std::string registryAddress;
};

#endif
