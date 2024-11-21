#ifndef NEXUS_REGISTRY_SERVER_H
#define NEXUS_REGISTRY_SERVER_H

#include <iostream>
#include <json/json.h>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "Utility.h"

struct NodeInfo {
  std::string type;
  std::string name;
  std::string ip;
  std::pair<double, double> coords;
  int port;
};

class NexusRegistryServer {
public:
  explicit NexusRegistryServer(int port);
  ~NexusRegistryServer();

  void start();
  void stop();

private:
  int serverSocket{};
  int port;
  std::vector<NodeInfo> nodes;
  std::mutex nodesMutex;
  bool isRunning;

  void handleClient(int clientSocket);
  void processRequest(const std::string &request, std::string &response);
  void registerNode(const NodeInfo &node);
  void deregisterNode(const std::string &name);
  void updateNode(const NodeInfo &node);
  std::string getNodeList();

  static std::string readFromSocket(int socket);
  static void writeToSocket(int socket, const std::string &response);
};

#endif // NEXUS_REGISTRY_SERVER_H
