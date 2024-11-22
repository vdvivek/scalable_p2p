#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <json/json.h>

using matrix = std::vector<std::vector<long long>>;

class Node;

class NetworkManager {
public:
  explicit NetworkManager(const std::string &registryAddress);

  bool nodeExists(const std::shared_ptr<Node> &node) const;
  static bool performCurlRequest(const std::string &url,
                                 const std::string &payload,
                                 std::string &response);
  static Json::Value createNodePayload(const std::string &action,
                                       const std::shared_ptr<Node> &node);
  std::shared_ptr<Node> parseNodeFromJson(const Json::Value &nodeJson) const;

  void addNode(const std::shared_ptr<Node> &node);
  void removeNode(const std::string &id);
  void listNodes() const;
  std::shared_ptr<Node> findNode(const std::string &name) const;

  std::vector<std::shared_ptr<Node>> getSatelliteNodes() const;

  void fetchNodesFromRegistry();
  static std::string serializeNode(const std::shared_ptr<Node> &node);
  bool registerNodeWithRegistry(const std::shared_ptr<Node> &node);
  void deregisterNodeWithRegistry(const std::shared_ptr<Node> &node);
  bool sendToRegistryServer(const std::string &endpoint,
                            const std::string &jsonPayload);
  bool updateNodeInRegistry(const std::shared_ptr<Node> &node) const;

  void createRoutingTable();
  void updateRoutingTable(const std::shared_ptr<Node> &src);
  void route(int src_idx);
  std::shared_ptr<Node> getNextHop(const std::string &name) const;

  std::vector<int> nextHop;

private:
  matrix topology;
  // Holds index for next hop for given destination
  // nextHop[S3 idx] = next node in the shortest path to S3
  std::vector<std::shared_ptr<Node>> nodes;
  std::string registryAddress;
};

#endif
