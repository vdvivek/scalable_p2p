#ifndef NEXUS_REGISTRY_SERVER_H
#define NEXUS_REGISTRY_SERVER_H

#include <cstdint>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <thread>
#include <vector>

#include "Logger.h"

struct NodeInfo {
  std::string type;
  std::string name;
  std::string ip;
  std::pair<double, double> coords;
  int port;
  std::string publicKey;
};

class NexusRegistryServer {
public:
  NexusRegistryServer(std::string ip, int port);
  ~NexusRegistryServer();

  void start();
  void stop();

private:
  int serverSocket{};
  std::string ip;
  int port;
  std::vector<NodeInfo> nodes;
  std::mutex nodesMutex;
  bool isRunning;

  void handleClient(int clientSocket);
  std::vector<uint8_t> processRequest(const std::vector<uint8_t> &request);
  void registerNode(const NodeInfo &node);
  void deregisterNode(const std::string &name);
  NodeInfo findNodeByName(const std::string &name);
  std::vector<uint8_t> getNodeList();

  std::vector<uint8_t> readFromSocket(int socket);
  void writeToSocket(int socket, const std::vector<uint8_t> &response);

  std::vector<uint8_t> serializeNode(const NodeInfo &node);
  NodeInfo deserializeNode(const std::vector<uint8_t> &data);
};

#endif // NEXUS_REGISTRY_SERVER_H
