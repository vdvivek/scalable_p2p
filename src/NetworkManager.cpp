#include "NetworkManager.h"
#include <curl/curl.h> // Requires libcurl
#include <iostream>
#include <json/json.h>

#include "Node.h"

NetworkManager::NetworkManager(const std::string &registryAddress)
    : registryAddress(registryAddress) {}

void NetworkManager::addNode(const std::shared_ptr<Node> &node) {
  // Check if the node already exists in the list, return if it does
  for (const auto &existingNode : nodes) {
    if (existingNode->getName() == node->getName() && existingNode->getIP() == node->getIP() &&
        existingNode->getPort() == node->getPort()) {
      return;
    }
  }

  // Add the node to the list if not present
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
  // Fetch nodes from the registry
  const_cast<NetworkManager *>(this)->fetchNodesFromRegistry();

  if (nodes.empty()) {
    std::cout << "[INFO] No nodes are currently registered in the network." << std::endl;
    return;
  }

  std::cout << "[INFO] Current nodes in the network:" << std::endl;
  for (const auto &node : nodes) {
    if (node) { // Ensure the pointer is valid
      std::cout << node->getName() << " (" << node->getId() << ") at " << node->getIP() << ":"
                << node->getPort() << " [" << node->getX() << ", " << node->getY() << ", "
                << node->getZ() << "]" << std::endl;
    } else {
      std::cerr << "[WARNING] Encountered a null node entry in the network." << std::endl;
    }
  }
}

bool NetworkManager::registerNodeWithRegistry(const std::shared_ptr<Node> &node) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    std::cerr << "[ERROR] Failed to initialize CURL for node registration." << std::endl;
    return false;
  }
  std::cout << "[DEBUG] Registry server is present at " << registryAddress << std::endl;
  std::string url = registryAddress + "/register";
  Json::Value payload;
  payload["action"] = "register";
  payload["name"] = node->getName();
  payload["ip"] = node->getIP();
  payload["port"] = node->getPort();
  std::string payloadStr = payload.toStyledString();

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::cerr << "[ERROR] Failed to register node: " << curl_easy_strerror(res) << std::endl;
    return false;
  }

  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);

  return true;
}

void NetworkManager::deregisterNodeWithRegistry(const std::shared_ptr<Node> &node) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    std::cerr << "[ERROR] Failed to initialize CURL for node deregistration." << std::endl;
    return;
  }

  std::string url = registryAddress + "/deregister";
  Json::Value payload;
  payload["action"] = "deregister";
  payload["name"] = node->getName();
  payload["ip"] = node->getIP();
  payload["port"] = node->getPort();
  std::string payloadStr = payload.toStyledString();

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::cerr << "[ERROR] Failed to deregister node: " << curl_easy_strerror(res) << std::endl;
  }

  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *s) {
  size_t totalSize = size * nmemb;
  if (s) {
    s->append(static_cast<char *>(contents), totalSize);
    return totalSize;
  }
  std::cerr << "[ERROR] Response string is null." << std::endl;
  return 0;
}

void NetworkManager::fetchNodesFromRegistry() {
  CURL *curl = curl_easy_init();
  if (!curl) {
    std::cerr << "[ERROR] Failed to initialize CURL for fetching nodes." << std::endl;
    return;
  }

  std::string url = registryAddress;
  std::string response;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1L);

  Json::Value payload;
  payload["action"] = "list";
  std::string payloadStr = payload.toStyledString();

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Accept: application/json");
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "charsets: utf-8");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // Enable verbose logging for CURL

  // Perform the request
  CURLcode res = curl_easy_perform(curl);

  if (response.empty()) {
    std::cerr << "[ERROR] Received empty response from registry." << std::endl;
    curl_easy_cleanup(curl);
    return;
  }

  if (res != CURLE_OK) {
    std::cerr << "[ERROR] Failed to fetch node list: " << curl_easy_strerror(res) << std::endl;
    curl_easy_cleanup(curl);
    return;
  }

  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);

  // Parse the JSON response
  Json::Value nodeListJson;
  Json::CharReaderBuilder reader;
  std::string errs;
  std::istringstream responseStream(response);

  if (!Json::parseFromStream(reader, responseStream, &nodeListJson, &errs)) {
    std::cerr << "[ERROR] Failed to parse node list: " << errs << std::endl;
    return;
  }

  if (!nodeListJson.isArray()) {
    std::cerr << "[ERROR] Unexpected response format: Expected an array." << std::endl;
    return;
  }

  // Process each node in the response
  for (const auto &nodeJson : nodeListJson) {
    if (!nodeJson.isObject() || !nodeJson.isMember("name") || !nodeJson.isMember("ip") ||
        !nodeJson.isMember("port")) {
      std::cerr << "[ERROR] Malformed node entry in response: " << nodeJson << std::endl;
      continue;
    }

    auto node = std::make_shared<Node>(nodeJson["name"].asString(), nodeJson["ip"].asString(),
                                       nodeJson["port"].asInt(), 0, 0, 0, // Default coordinates
                                       *this);
    addNode(node);
  }
}
