#include <curl/curl.h> // Requires libcurl
#include <iostream>
#include <json/json.h>

#include "Node.h"
#include "NetworkManager.h"

NetworkManager::NetworkManager(const std::string &registryAddress)
    : registryAddress(registryAddress) {}

// Helper: Check if a node exists in the list
bool NetworkManager::nodeExists(const std::shared_ptr<Node>& node) const {
  return std::any_of(nodes.begin(), nodes.end(), [&](const std::shared_ptr<Node>& existingNode) {
    return existingNode->getName() == node->getName() &&
           existingNode->getIP() == node->getIP() &&
           existingNode->getPort() == node->getPort();
  });
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
  size_t totalSize = size * nmemb;
  if (s) {
    s->append(static_cast<char*>(contents), totalSize);
    return totalSize;
  }
  return 0;
}

// Helper: Perform CURL requests
bool NetworkManager::performCurlRequest(const std::string& url, const std::string& payload, std::string& response) {
  std::cout << "[DEBUG] Performing request to NexusRegistryServer..." << std::endl;
  CURL* curl = curl_easy_init();
  if (!curl) {
    std::cerr << "[ERROR] Failed to initialize CURL" << std::endl;
    return false;
  }

  struct curl_slist* headers = curl_slist_append(nullptr, "Content-Type: application/json");
  if (!headers) {
    std::cerr << "[ERROR] Failed to set CURL headers" << std::endl;
    curl_easy_cleanup(curl);
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);         // Set timeout
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);  // Set connection timeout

  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    std::cerr << "[ERROR] CURL request failed: " << curl_easy_strerror(res) << std::endl;
    return false;
  }

  long httpCode = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
  if (httpCode >= 400) {
    std::cerr << "[ERROR] Server responded with HTTP code: " << httpCode << std::endl;
    return false;
  }

  std::cout << "[DEBUG] Response from server: " << response << std::endl;
  return true;
}


void NetworkManager::addNode(const std::shared_ptr<Node>& node) {
  if (nodeExists(node)) {
    for (auto& existingNode : nodes) {
      if (existingNode->getName() == node->getName()) {
        existingNode->setCoords(node->getCoords());
        return;
      }
    }
  }

  nodes.push_back(node);
  std::cout << "[INFO] Added node: " << node->getName() << " (" << node->getId() << ") at "
            << node->getIP() << ":" << node->getPort() << " to the network." << std::endl;
}

void NetworkManager::removeNode(const std::string &id) {
  auto it = std::remove_if(nodes.begin(), nodes.end(), [&id](const std::shared_ptr<Node> &node) {
    return node->getId() == id;
  });

  if (it != nodes.end()) {
    nodes.erase(it, nodes.end());
    std::cout << "[INFO] Removed node with ID: " << id << std::endl;
  } else {
    std::cerr << "[WARNING] Node with ID " << id << " not found in the network." << std::endl;
  }
}

void NetworkManager::listNodes() const {
  if (nodes.empty()) {
    std::cout << "[INFO] No nodes are currently registered in the network." << std::endl;
    return;
  }

  std::cout << "[INFO] Current nodes in the network:" << std::endl;
  for (const std::shared_ptr<Node>& node : nodes) {
    auto coords = node->getCoords();
    std::cout << NodeType::toString(node->getType()) << " " << node->getName() << " ("
              << node->getId() << ") at " << node->getIP() << ":" << node->getPort() << " ["
              << coords.first << ", " << coords.second << "]" << std::endl;
  }
}

std::vector<std::shared_ptr<Node>> NetworkManager::getSatelliteNodes() const {
  std::vector<std::shared_ptr<Node>> satellites;
  for (const std::shared_ptr<Node>& node : nodes) {
    if (node->getType() == NodeType::SATELLITE) {
      satellites.push_back(node);
    }
  }
  return satellites;
}

bool NetworkManager::registerNodeWithRegistry(const std::shared_ptr<Node>& node) {
  std::cout << "[DEBUG] Registering node at NexusRegistryServer..."  << std::endl;

  Json::Value payload = createNodePayload("register", node);
  std::string url = registryAddress + "/register";
  std::string response;
  return performCurlRequest(url, payload.toStyledString(), response);
}

void NetworkManager::deregisterNodeWithRegistry(const std::shared_ptr<Node>& node) {
  std::cout << "[DEBUG] Deregistering node at NexusRegistryServer..."  << std::endl;

  Json::Value payload = createNodePayload("deregister", node);
  std::string url = registryAddress + "/deregister";
  std::string response;
  performCurlRequest(url, payload.toStyledString(), response);
}

Json::Value NetworkManager::createNodePayload(const std::string& action, const std::shared_ptr<Node>& node) {
  std::cout << "[DEBUG] Create payload for Node"  << std::endl;

  Json::Value payload;
  payload["action"] = action;
  payload["type"] = NodeType::toString(node->getType());
  payload["name"] = node->getName();
  payload["ip"] = node->getIP();
  payload["port"] = node->getPort();
  auto coords = node->getCoords();
  payload["x"] = coords.first;
  payload["y"] = coords.second;
  return payload;
}

bool NetworkManager::updateNodeInRegistry(const std::shared_ptr<Node>& node) {
  Json::Value payload = createNodePayload("update", node);
  std::string url = registryAddress + "/update";
  std::string response;
  return performCurlRequest(url, payload.toStyledString(), response);
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

  std::cout << "[DEBUG] Raw response: " << response << std::endl;

  if (!Json::parseFromStream(reader, responseStream, &nodeListJson, &errs)) {
    std::cerr << "[ERROR] Failed to parse node list: " << errs << std::endl;
    return;
  }

  if (!nodeListJson.isArray()) {
    std::cerr << "[ERROR] Unexpected response format: Expected an array." << std::endl;
    return;
  }

  for (const auto& nodeJson : nodeListJson) {
    if (!nodeJson.isObject()) {
      std::cerr << "[ERROR] Malformed node entry in response." << std::endl;
      continue;
    }

    auto node = parseNodeFromJson(nodeJson);
    if (node) addNode(node);
  }
}

std::shared_ptr<Node> NetworkManager::parseNodeFromJson(const Json::Value& nodeJson) const {
  if (!nodeJson.isMember("name") || !nodeJson.isMember("ip") || !nodeJson.isMember("port")) {
    std::cerr << "[ERROR] Missing required fields in node JSON." << std::endl;
    return nullptr;
  }

  double x = nodeJson.get("x", 0.0).asDouble();
  double y = nodeJson.get("y", 0.0).asDouble();
  NodeType::Type type = NodeType::fromString(nodeJson["type"].asString());

  return std::make_shared<Node>(type, nodeJson["name"].asString(), nodeJson["ip"].asString(),
                                nodeJson["port"].asInt(), std::make_pair(x, y), *this);
}

void NetworkManager::createRoutingTable() {
  topology.clear();
  topology.resize(nodes.size());

  for (int i = 0; i < nodes.size(); i++) {
    topology[i] = std::vector<int>(nodes.size(), 0);
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

      int weight = 0;

      auto i_isGround = nodes[i]->getType() == NodeType::GROUND;
      auto j_isGround = nodes[j]->getType() == NodeType::GROUND;

      if (i_isGround && j_isGround) {
        weight += 10000; // PRIORITIZE SATELLITE
      } else if (i_isGround) {
        weight += 50;
      } else if (j_isGround) {
        weight += 50;
      }

      // Add any extra weight
      topology[i][j] = std::sqrt(x_diff + y_diff) + weight;
      topology[j][i] = std::sqrt(x_diff + y_diff) + weight;
    }
  }

  for (int i = 0; i < nodes.size(); i++) {
    for (int j = 0; j < nodes.size(); j++) {
      std::cout << topology[i][j] << "\t";
    }
    std::cout << std::endl;
  }

  int src_idx = 0;
  for (int i = 0; i < nodes.size(); i++) {
    if (nodes[i]->getId() == src->getId()) {
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
      if (!visited[j] && topology[minUnvIdx][j] != 0 &&
          minDist[minUnvIdx] + topology[minUnvIdx][j] < minDist[j]) {
        minDist[j] = minDist[minUnvIdx] + topology[minUnvIdx][j];
        if (minUnvIdx == src_idx)
          continue;
        nextHop[j] = minUnvIdx;
      }
    }
  }
}

std::shared_ptr<Node> NetworkManager::getNextHop(const std::string &name) {
  int n_idx = -1;

  for (int i = 0; i < nodes.size(); i++) {
    if (nodes[i]->getName() == name) {
      n_idx = i;
      break;
    }
  }

  return nodes[nextHop[n_idx]];
}
