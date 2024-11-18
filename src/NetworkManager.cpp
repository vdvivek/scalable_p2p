#include "NetworkManager.h"
#include <json/json.h>
#include <curl/curl.h> // Requires libcurl
#include <iostream>
#include <unordered_map>
#include <queue>

#include "Node.h"
#include "SatelliteNode.h"
#include "GroundNode.h"

NetworkManager::NetworkManager(const std::string& registryAddress)
    : registryAddress(registryAddress) {}


std::shared_ptr<Node> NetworkManager::createNode(Node::NodeType type, const std::string& name, 
                                                 const std::string& ip, int port, 
                                                 const std::pair<double, double>& coords) {
    std::shared_ptr<Node> node;

    if (type == Node::NodeType::Ground) {
        std::cout << "[INFO] Creating GroundNode: " << name << " at " << ip << ":" << port << std::endl;
        node = std::make_shared<GroundNode>(name, ip, port, coords, *this, Node::NodeType::Ground);
    } else if (type == Node::NodeType::Satellite) {
        std::cout << "[INFO] Creating SatelliteNode: " << name << " at " << ip << ":" << port << std::endl;
        node = std::make_shared<SatelliteNode>(name, ip, port, coords, *this, Node::NodeType::Satellite);
    } else {
        std::cerr << "[ERROR] Unknown NodeType specified. Node creation failed." << std::endl;
        return nullptr;
    }

    addNode(node);
    return node;
}


void NetworkManager::addNode(const std::shared_ptr<Node>& node) {
    // Check if the node already exists to prevent duplicates
    for (const auto& existingNode : nodes) {
        if (existingNode->getName() == node->getName() &&
            existingNode->getIP() == node->getIP() &&
            existingNode->getPort() == node->getPort()) {
            std::cerr << "[WARNING] Duplicate node detected: " << node->getName() << std::endl;
            return; // Do not add duplicate node
        }
    }

    // Add the node to the list
    nodes.push_back(node);
    std::cout << "[INFO] Added node: " << node->getName() 
              << " (" << node->getId() << ") at " << node->getIP() << ":" 
              << node->getPort() << " to the network. Type: " 
              << node->getNodeTypeAsString() << std::endl;
}



void NetworkManager::removeNode(const std::string &id) {
    auto it = std::remove_if(nodes.begin(), nodes.end(),
                             [&id](const std::shared_ptr<Node>& node) {
                                 return node->getId() == id;
                             });

    if (it != nodes.end()) {
        nodes.erase(it, nodes.end());
        std::cout << "[INFO] Removed node with ID: " << id << std::endl;
    } else {
        std::cerr << "[WARNING] Node with ID " << id << " not found in the network." << std::endl;
    }
}

// std::vector<std::shared_ptr<Node>> NetworkManager::getSatelliteNodes() const {
//     std::cout<<"Inside get satellite"<<std::endl;
//     std::vector<std::shared_ptr<Node>> satellites;
//     for (const auto& node : nodes) {
//         if (std::dynamic_pointer_cast<SatelliteNode>(node)) {
//             satellites.push_back(node);
//             std::cout << "[DEBUG] Found satellite: " << node->getName() << std::endl;
//         }
//     }
//     return satellites;
// }
// std::vector<std::shared_ptr<Node>> NetworkManager::getSatelliteNodes() const {
//     std::cout << "Inside getSatelliteNodes()" << std::endl;

//     std::vector<std::shared_ptr<Node>> satellites;

//     // Fetch registered nodes
//     for (const auto& node : nodes) {
//         // Check if the node is a SatelliteNode
//         auto satellite = std::dynamic_pointer_cast<SatelliteNode>(node);
//         if (satellite) {
//             satellites.push_back(satellite);
//             std::cout << "[DEBUG] Found satellite: " << node->getName() 
//                       << " at " << node->getIP() << ":" << node->getPort() 
//                       << " [" << node->getCoords().first << ", " << node->getCoords().second << "]" 
//                       << std::endl;
//         }
//     }

//     if (satellites.empty()) {
//         std::cerr << "[WARNING] No satellites found among registered nodes." << std::endl;
//     }

//     return satellites;
// }

std::vector<std::shared_ptr<Node>> NetworkManager::getSatelliteNodes() const {
    std::vector<std::shared_ptr<Node>> satellites;

    for (const auto& node : nodes) {
        if (node->getNodeTypeAsString() == "Satellite") {
            satellites.push_back(node);
            std::cout << "[DEBUG] Found satellite: " << node->getName() << std::endl;
        }
    }

    return satellites;
}


void NetworkManager::listNodes() const {
    // Fetch nodes from the registry
    const_cast<NetworkManager*>(this)->fetchNodesFromRegistry();

    if (nodes.empty()) {
        std::cout << "[INFO] No nodes are currently registered in the network." << std::endl;
        return;
    }

    std::cout << "[INFO] Current nodes in the network:" << std::endl;
    for (const auto& node : nodes) {
        if (node) {
            auto coords = node->getCoords();
            std::cout << node->getName() << " (" << node->getId() << ") at "
                      << node->getIP() << ":" << node->getPort()
                      << " [" << coords.first << ", " << coords.second << "]" << std::endl;
        } else {
            std::cerr << "[WARNING] Encountered a null node entry in the network." << std::endl;
        }
    }
}

bool NetworkManager::registerNodeWithRegistry(const std::shared_ptr<Node>& node) {
    CURL* curl = curl_easy_init();
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
    auto coords = node->getCoords();
    payload["x"] = coords.first;
    payload["y"] = coords.second;
    std::string payloadStr = payload.toStyledString();

    struct curl_slist* headers = nullptr;
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

void NetworkManager::deregisterNodeWithRegistry(const std::shared_ptr<Node>& node) {
    CURL* curl = curl_easy_init();
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
    auto coords = node->getCoords();
    payload["x"] = coords.first;
    payload["y"] = coords.second;
    std::string payloadStr = payload.toStyledString();

    struct curl_slist* headers = nullptr;
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

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t totalSize = size * nmemb;
    if (s) {
        s->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }
    std::cerr << "[ERROR] Response string is null." << std::endl;
    return 0;
}

void NetworkManager::fetchNodesFromRegistry() {
    CURL* curl = curl_easy_init();
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

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "[ERROR] Failed to fetch node list: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        return;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    Json::Value nodeListJson;
    Json::CharReaderBuilder reader;
    std::string errs;
    std::istringstream responseStream(response);

    if (!Json::parseFromStream(reader, responseStream, &nodeListJson, &errs)) {
        std::cerr << "[ERROR] Failed to parse node list: " << errs << std::endl;
        return;
    }

    nodes.clear();

    for (const auto& nodeJson : nodeListJson) {
        if (!nodeJson.isObject() || !nodeJson.isMember("name") || !nodeJson.isMember("ip") || !nodeJson.isMember("port")) {
            std::cerr << "[ERROR] Malformed node entry: " << nodeJson << std::endl;
            continue;
        }

        std::string type = nodeJson["type"].asString(); // Expect a "type" field to distinguish node types
        std::string name = nodeJson["name"].asString();
        std::string ip = nodeJson["ip"].asString();
        int port = nodeJson["port"].asInt();
        double x = nodeJson["x"].asDouble();
        double y = nodeJson["y"].asDouble();

        std::shared_ptr<Node> node;
        if (type == "satellite") {
            node = std::make_shared<SatelliteNode>(name, ip, port, std::make_pair(x, y), *this,Node::NodeType::Satellite);
            

            std::cout << "[DEBUG] fetchNodesFromRegistry called. Total nodes: " << nodes.size() << std::endl;


        } else {
            node = std::make_shared<GroundNode>(name, ip, port, std::make_pair(x, y), *this,Node::NodeType::Ground);
            

            std::cout << "[DEBUG] fetchNodesFromRegistry called. Total nodes: " << nodes.size() << std::endl;
        }

        nodes.push_back(node);
        std::cout << "[DEBUG] Fetched node: " << node->getName() 
                  << " (" << node->getIP() << ":" << node->getPort() 
                  << ") of type " << type << " [" << x << ", " << y << "]" << std::endl;
    }

    std::cout << "[DEBUG] Total nodes in NetworkManager: " << nodes.size() << std::endl;
}


std::shared_ptr<Node> NetworkManager::getNodeByName(const std::string& name) const {
    for (const auto& node : nodes) {
        std::cout << "[DEBUG] Checking node: " << node->getName() << " against " << name << std::endl;
        if (node->getName() == name) {
            return node;
        }
    }
    std::cerr << "[ERROR] Target node not found: " << name << std::endl;
    return nullptr;
}



std::shared_ptr<Node> NetworkManager::getNextHop(const std::shared_ptr<Node>& current, const std::string& targetIP, int targetPort) {
    for (const auto& node : nodes) {
        if (node->getIP() == targetIP && node->getPort() == targetPort) {
            return node;
        }
    }
    return nullptr;
}


