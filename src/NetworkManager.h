#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <memory>
#include <string>
#include <vector>

class Node;

class NetworkManager {
public:
  NetworkManager(const std::string &registryAddress);

  void addNode(const std::shared_ptr<Node> &node);
  void removeNode(const std::string &id);
  void listNodes() const;

  bool registerNodeWithRegistry(const std::shared_ptr<Node> &node);
  void deregisterNodeWithRegistry(const std::shared_ptr<Node> &node);
  void fetchNodesFromRegistry();

private:
  std::vector<std::shared_ptr<Node>> nodes;
  std::string registryAddress;
};

#endif
