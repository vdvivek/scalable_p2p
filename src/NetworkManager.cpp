#include "NetworkManager.h"

#include <curl/curl.h>
#include <json/json.h>
#include "climits"

#include <iostream>

#include "Logger.h"
#include "Node.h"

#ifndef LONG_LONG_MAX
#define LONG_LONG_MAX LLONG_MAX
#endif

NetworkManager::NetworkManager(const std::string &registryAddress)
    : registryAddress(registryAddress) {}

// Helper: Check if a node exists in the list
bool NetworkManager::nodeExists(const std::shared_ptr<Node> &node) const {
  return std::any_of(nodes.begin(), nodes.end(),
                     [&](const std::shared_ptr<Node> &existingNode) {
                       return existingNode->getName() == node->getName() &&
                              existingNode->getIP() == node->getIP() &&
                              existingNode->getPort() == node->getPort();
                     });
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            std::string *s) {
  size_t totalSize = size * nmemb;
  if (s) {
    s->append(static_cast<char *>(contents), totalSize);
    return totalSize;
  }
  return 0;
}

// Helper: Perform CURL requests
bool NetworkManager::performCurlRequest(const std::string &url,
                                        const std::string &payload,
                                        std::string &response) {
  logger.log(LogLevel::INFO,
             "[NEXUS] Performing request to NexusRegistryServer ...");
  CURL *curl = curl_easy_init();
  if (!curl) {
    logger.log(LogLevel::ERROR, "Failed to initialize CURL.");
    return false;
  }

  struct curl_slist *headers =
      curl_slist_append(nullptr, "Content-Type: application/json");
  if (!headers) {
    logger.log(LogLevel::ERROR, "Failed to set CURL headers.");
    curl_easy_cleanup(curl);
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);       // Set timeout
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); // Set connection timeout

  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    logger.log(LogLevel::ERROR, "Failed to send CURL request: " +
                                    std::string(curl_easy_strerror(res)));
    return false;
  }

  long httpCode = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
  if (httpCode >= 400) {
    logger.log(LogLevel::ERROR,
               "Server responded with HTTP code: " + std::to_string(httpCode));
    return false;
  }

  return true;
}

void NetworkManager::addNode(const std::shared_ptr<Node> &node) {
  if (nodeExists(node)) {
    for (auto &existingNode : nodes) {
      if (existingNode->getName() == node->getName()) {
        existingNode->setCoords(node->getCoords());
        return;
      }
    }
  }

  nodes.push_back(node);
  logger.log(LogLevel::INFO, "[NEXUS] Added node: " + node->getName() + " (" +
                                 node->getId() + ") at " + node->getIP() + ":" +
                                 std::to_string(node->getPort()) +
                                 " to the network.");
}

void NetworkManager::removeNode(const std::string &id) {
  auto it = std::remove_if(
      nodes.begin(), nodes.end(),
      [&id](const std::shared_ptr<Node> &node) { return node->getId() == id; });

  if (it != nodes.end()) {
    nodes.erase(it, nodes.end());
    logger.log(LogLevel::INFO, "[NEXUS] Removed node with ID: " + id);
  } else {
    logger.log(LogLevel::WARNING,
               "[NEXUS] Node with ID " + id + " not found in the network.");
  }
}

void NetworkManager::listNodes() const {
  if (nodes.empty()) {
    logger.log(LogLevel::INFO,
               "[NEXUS] No nodes are currently registered in the network.");
    return;
  }

  logger.log(LogLevel::INFO,
             "[NEXUS] Listing nodes identified in the local network.");
  for (const std::shared_ptr<Node> &node : nodes) {
    auto coords = node->getCoords();
    logger.log(LogLevel::INFO,
               "[NEXUS] " + NodeType::toString(node->getType()) + " " +
                   node->getName() + " (" + node->getId() + ") at " +
                   node->getIP() + ":" + std::to_string(node->getPort()) +
                   " [" + std::to_string(coords.first) + ", " +
                   std::to_string(coords.second) + "]");
  }
}

std::shared_ptr<Node> NetworkManager::findNode(const std::string &name) const {
  for (auto &node : nodes) {
    if (node->getName() == name) {
      return node;
    }
  }
  return nullptr;
}

std::vector<std::shared_ptr<Node>> NetworkManager::getSatelliteNodes() const {
  std::vector<std::shared_ptr<Node>> satellites;
  for (const std::shared_ptr<Node> &node : nodes) {
    if (node->getType() == NodeType::SATELLITE) {
      satellites.push_back(node);
    }
  }
  return satellites;
}

bool NetworkManager::registerNodeWithRegistry(
    const std::shared_ptr<Node> &node) {
  logger.log(LogLevel::DEBUG,
             "[NEXUS] Registering node at NexusRegistryServer ...");
  Json::Value payload = createNodePayload("register", node);
  std::string url = registryAddress + "/register";
  std::string response;
  return performCurlRequest(url, payload.toStyledString(), response);
}

void NetworkManager::deregisterNodeWithRegistry(
    const std::shared_ptr<Node> &node) {
  logger.log(LogLevel::DEBUG,
             "[NEXUS] Deregistering node at NexusRegistryServer ...");
  Json::Value payload = createNodePayload("deregister", node);
  std::string url = registryAddress + "/deregister";
  std::string response;
  performCurlRequest(url, payload.toStyledString(), response);
}

Json::Value
NetworkManager::createNodePayload(const std::string &action,
                                  const std::shared_ptr<Node> &node) {
  Json::Value payload;
  payload["action"] = action;
  payload["type"] = NodeType::toString(node->getType());
  payload["name"] = node->getName();
  payload["ip"] = node->getIP();
  payload["port"] = node->getPort();
  auto coords = node->getCoords();
  payload["x"] = coords.first;
  payload["y"] = coords.second;
  payload["publicKey"] = node->getPublicKey();
  return payload;
}

bool NetworkManager::updateNodeInRegistry(
    const std::shared_ptr<Node> &node) const {
  // logger.log(LogLevel::DEBUG,
  //            "[NEXUS] Updating node at NexusRegistryServer ...");
  Json::Value payload = createNodePayload("update", node);
  std::string url = registryAddress + "/update";
  std::string response;
  return performCurlRequest(url, payload.toStyledString(), response);
}

std::string NetworkManager::getNodePublicKey(const std::string &nodeName) const {
  Json::Value payload;
  payload["action"] = "getPublicKey";
  payload["name"] = nodeName;

  std::string response;
  std::string url = registryAddress + "/getPublicKey";
  if (!performCurlRequest(url, payload.toStyledString(), response)) {
    throw std::runtime_error("Failed to fetch public key for node: " +
                             nodeName);
  }

  Json::CharReaderBuilder reader;
  Json::Value jsonResponse;
  std::istringstream responseStream(response);
  std::string errs;

  if (!Json::parseFromStream(reader, responseStream, &jsonResponse, &errs)) {
    throw std::runtime_error("Invalid response while fetching public key: " +
                             errs);
  }

  if (!jsonResponse.isMember("publicKey")) {
    throw std::runtime_error("No public key found for node: " + nodeName);
  }

  return jsonResponse["publicKey"].asString();
}

void NetworkManager::fetchNodesFromRegistry() {
  Json::Value payload;
  payload["action"] = "list";
  std::string response;
  performCurlRequest(registryAddress, payload.toStyledString(), response);

  Json::Value nodeListJson;
  Json::CharReaderBuilder reader;
  std::istringstream responseStream(response);
  std::string errs;

  if (!Json::parseFromStream(reader, responseStream, &nodeListJson, &errs)) {
    logger.log(LogLevel::ERROR, "Failed to parse node list: " + errs);
    return;
  }

  if (!nodeListJson.isArray()) {
    logger.log(LogLevel::ERROR,
               "Unexpected response format: Expected an array.");
    return;
  }

  for (const auto &nodeJson : nodeListJson) {
    if (!nodeJson.isObject()) {
      logger.log(LogLevel::ERROR, "Malformed node entry in response.");
      continue;
    }

    auto node = parseNodeFromJson(nodeJson);
    if (node) {
      addNode(node);
    }
  }
}

std::shared_ptr<Node>
NetworkManager::parseNodeFromJson(const Json::Value &nodeJson) const {
  if (!nodeJson.isMember("name") || !nodeJson.isMember("ip") ||
      !nodeJson.isMember("port")) {
    logger.log(LogLevel::ERROR, "Missing required fields in node JSON.");
    return nullptr;
  }

  double x = nodeJson.get("x", 0.0).asDouble();
  double y = nodeJson.get("y", 0.0).asDouble();
  NodeType::Type type = NodeType::fromString(nodeJson["type"].asString());

  std::shared_ptr<Node> node = std::make_shared<Node>(
      type, nodeJson["name"].asString(), nodeJson["ip"].asString(),
      nodeJson["port"].asInt(), std::make_pair(x, y), *this);

  return node;
}

void NetworkManager::createRoutingTable() {
  topology.clear();
  topology.resize(nodes.size());

  for (int i = 0; i < nodes.size(); i++) {
    topology[i] = std::vector<long long>(nodes.size(), 0);
  }
}

void NetworkManager::updateRoutingTable(const std::shared_ptr<Node> &src) {
  if (nodes.size() != topology.size()) {
    createRoutingTable();
  }

  for (int i = 0; i < nodes.size(); i++) {
    for (int j = 0; j < nodes.size(); j++) {
      auto i_coords = nodes[i]->getCoords();
      auto j_coords = nodes[j]->getCoords();

      int x_diff = std::pow(i_coords.first - j_coords.first, 2);
      int y_diff = std::pow(i_coords.second - j_coords.second, 2);
      long long weight = std::sqrt(x_diff + y_diff);

      auto i_isGround = (nodes[i]->getType() == NodeType::GROUND);
      auto j_isGround = (nodes[j]->getType() == NodeType::GROUND);

      if (i_isGround && j_isGround) {
        topology[i][j] = LONG_LONG_MAX;
        topology[j][i] = LONG_LONG_MAX;
        continue;
      } else if (i_isGround) {
        weight += 1000;
      } else if (j_isGround) {
        weight += 1000;
      }

      // Applying a quadratic penalty when exceeding 500 units
      if (weight > 500) {
        weight = 500 + (weight - 500) * (weight - 500);
      }

      topology[i][j] = weight;
      topology[j][i] = weight;
    }
  }

  int src_idx = 0;
  for (int i = 0; i < nodes.size(); i++) {
    if (nodes[i]->getName() == src->getName()) {
      src_idx = i;
      break;
    }
  }

  route(src_idx);
}

void NetworkManager::route(int src_idx) {
  if (nextHop.size() != nodes.size())
    nextHop.resize(nodes.size());

  for (int i = 0; i < nodes.size(); i++) {
    nextHop[i] = i;
  }

  std::vector<bool> visited(nodes.size(), false);
  std::vector<int> minDist(nodes.size(), INT_MAX);
  minDist[src_idx] = 0;

  for (int i = 0; i < nodes.size(); i++) {
    int minUnvisited = INT_MAX, minUnvIdx;

    for (int j = 0; j < nodes.size(); j++) {
      if (!visited[j] && minDist[j] <= minUnvisited) {
        minUnvisited = minDist[j];
        minUnvIdx = j;
      }
    }

    visited[minUnvIdx] = true;

    for (int j = 0; j < nodes.size(); j++) {
      if (!visited[j] && topology[minUnvIdx][j] != LONG_LONG_MAX &&
          minDist[minUnvIdx] + topology[minUnvIdx][j] < minDist[j]) {
        minDist[j] = minDist[minUnvIdx] + topology[minUnvIdx][j];
        if (minUnvIdx == src_idx)
          continue;
        nextHop[j] = minUnvIdx;
      }
    }
  }
}

std::shared_ptr<Node>
NetworkManager::getNextHop(const std::string &name) const {
  int n_idx = -1;

  for (int i = 0; i < nodes.size(); i++) {
    if (nodes[i]->getName() == name) {
      n_idx = i;
      break;
    }
  }

  if (n_idx == -1 || n_idx >= nodes.size()) {
    return nullptr;
  } else {
    return nodes[nextHop[n_idx]];
  }
}
