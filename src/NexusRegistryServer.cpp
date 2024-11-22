#include "NexusRegistryServer.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <utility>

NexusRegistryServer::NexusRegistryServer(std::string ip, const int port)
    : ip(std::move(ip)), port(port), isRunning(false) {}

NexusRegistryServer::~NexusRegistryServer() { stop(); }

void NexusRegistryServer::start() {
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket < 0) {
    logger.log(LogLevel::ERROR, "Failed to create socket.");
    return;
  }

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);

  if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
    logger.log(LogLevel::ERROR, "Invalid IP address: " + ip);
    close(serverSocket);
    return;
  }

  if (bind(serverSocket, reinterpret_cast<struct sockaddr *>(&serverAddr),
           sizeof(serverAddr)) < 0) {
    logger.log(LogLevel::ERROR,
               "Failed to bind socket to " + ip + ":" + std::to_string(port));
    close(serverSocket);
    return;
  }

  if (listen(serverSocket, 10) < 0) {
    logger.log(LogLevel::ERROR, "Failed to listen on socket.");
    close(serverSocket);
    return;
  }

  isRunning = true;
  logger.log(LogLevel::INFO, "NexusRegistryServer is running on " + ip + ":" +
                                 std::to_string(port) + ".");

  while (isRunning) {
    sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);
    int clientSocket =
        accept(serverSocket, reinterpret_cast<struct sockaddr *>(&clientAddr),
               &clientLen);

    if (clientSocket < 0) {
      logger.log(LogLevel::ERROR, "Failed to accept connection.");
      continue;
    }

    std::thread(&NexusRegistryServer::handleClient, this, clientSocket)
        .detach();
  }
}

void NexusRegistryServer::stop() {
  isRunning = false;
  close(serverSocket);
  logger.log(LogLevel::INFO, "NexusRegistryServer stopped.");
}

void NexusRegistryServer::handleClient(int clientSocket) {
  auto request = readFromSocket(clientSocket);
  auto response = processRequest(request);
  writeToSocket(clientSocket, response);
  close(clientSocket);
}

std::vector<uint8_t>
NexusRegistryServer::processRequest(const std::vector<uint8_t> &request) {
  if (request.empty()) {
    return {0xFF}; // Error code for invalid request
  }

  uint8_t action = request[0];
  std::vector<uint8_t> response;

  switch (action) {
  case 0x01: { // Register node
    NodeInfo node = deserializeNode(
        std::vector<uint8_t>(request.begin() + 1, request.end()));
    registerNode(node);
    response.push_back(0x00); // Success
    break;
  }
  case 0x02: { // Deregister node
    std::string name(reinterpret_cast<const char *>(request.data() + 1),
                     request.size() - 1);
    deregisterNode(name);
    response.push_back(0x00); // Success
    break;
  }
  case 0x03: { // Get node list
    response = getNodeList();
    break;
  }
  default:
    response.push_back(0xFF); // Unknown action
    break;
  }

  return response;
}

void NexusRegistryServer::registerNode(const NodeInfo &node) {
  std::lock_guard<std::mutex> lock(nodesMutex);
  nodes.push_back(node);
  logger.log(LogLevel::INFO, "Registered node: " + node.name + " (" + node.ip +
                                 ":" + std::to_string(node.port) + ")" +
                                 " with public key: " + node.publicKey);
}

std::string trimNull(const std::string &str) {
  size_t end = str.find('\0');
  if (end != std::string::npos) {
    return str.substr(0, end);
  }
  return str;
}

void NexusRegistryServer::deregisterNode(const std::string &name) {
  std::lock_guard<std::mutex> lock(nodesMutex);

  std::string sanitizedName = trimNull(name);
  logger.log(LogLevel::DEBUG, "Deregistering node: " + sanitizedName);

  auto initialSize = nodes.size();

  nodes.erase(std::remove_if(
                  nodes.begin(), nodes.end(),
                  [&sanitizedName](const NodeInfo &node) {
                      return node.name == sanitizedName;
                  }),
              nodes.end());

  auto finalSize = nodes.size();

  if (finalSize < initialSize) {
    logger.log(LogLevel::INFO, "Deregistered node: " + sanitizedName);
  } else {
    logger.log(LogLevel::WARNING, "Node not found for deregistration: " + sanitizedName);
  }
}


NodeInfo NexusRegistryServer::findNodeByName(const std::string &name) {
  std::lock_guard<std::mutex> lock(nodesMutex);
  for (const auto &node : nodes) {
    if (node.name == name) {
      return node;
    }
  }
  return NodeInfo{};
}

std::vector<uint8_t> NexusRegistryServer::getNodeList() {
  std::lock_guard<std::mutex> lock(nodesMutex);
  std::vector<uint8_t> response;

  for (const auto &node : nodes) {
    auto serializedNode = serializeNode(node);
    response.insert(response.end(), serializedNode.begin(),
                    serializedNode.end());
  }

  return response;
}

std::vector<uint8_t> NexusRegistryServer::serializeNode(const NodeInfo &node) {
  std::vector<uint8_t> data;
  data.insert(data.end(), node.name.begin(), node.name.end());
  data.push_back('\0');
  data.insert(data.end(), node.ip.begin(), node.ip.end());
  data.push_back('\0');

  auto port = reinterpret_cast<const uint8_t *>(&node.port);
  data.insert(data.end(), port, port + sizeof(node.port));

  auto coords = reinterpret_cast<const uint8_t *>(&node.coords);
  data.insert(data.end(), coords, coords + sizeof(node.coords));

  data.insert(data.end(), node.publicKey.begin(), node.publicKey.end());
  data.push_back('\0'); // Null-terminate the public key

  return data;
}

NodeInfo
NexusRegistryServer::deserializeNode(const std::vector<uint8_t> &data) {
  NodeInfo node;
  size_t offset = 0;

  while (offset < data.size() && data[offset] != '\0') {
    node.name.push_back(data[offset++]);
  }
  offset++;

  while (offset < data.size() && data[offset] != '\0') {
    node.ip.push_back(data[offset++]);
  }
  offset++;

  std::memcpy(&node.port, data.data() + offset, sizeof(node.port));
  offset += sizeof(node.port);

  std::memcpy(&node.coords, data.data() + offset, sizeof(node.coords));
  offset += sizeof(node.coords);

  while (offset < data.size() && data[offset] != '\0') {
    node.publicKey.push_back(data[offset++]);
  }

  return node;
}

std::vector<uint8_t> NexusRegistryServer::readFromSocket(int socket) {
  std::vector<uint8_t> buffer(4096);
  int bytesRead = recv(socket, buffer.data(), buffer.size(), 0);
  if (bytesRead <= 0) {
    return {};
  }
  buffer.resize(bytesRead);
  return buffer;
}

void NexusRegistryServer::writeToSocket(int socket,
                                        const std::vector<uint8_t> &response) {
  send(socket, response.data(), response.size(), 0);
}
