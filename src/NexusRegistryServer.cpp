#include "NodeType.h"
#include "NexusRegistryServer.h"

NexusRegistryServer::NexusRegistryServer(int port) : port(port), isRunning(false) {}

NexusRegistryServer::~NexusRegistryServer() { stop(); }

void NexusRegistryServer::start() {
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket < 0) {
    logger.log(LogLevel::ERROR, "Failed to create socket.");
    return;
  }

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(port);

  if (bind(serverSocket, reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr)) <
      0) {
    logger.log(LogLevel::ERROR, "Failed to bind socket.");
    close(serverSocket);
    return;
  }

  if (listen(serverSocket, 10) < 0) {
    logger.log(LogLevel::ERROR, "Failed to listen on socket.");
    close(serverSocket);
    return;
  }

  isRunning = true;
  logger.log(LogLevel::INFO, "NexusRegistryServer is running on port " + std::to_string(port) + ".");

  while (isRunning) {
    sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);
    int clientSocket =
        accept(serverSocket, reinterpret_cast<struct sockaddr *>(&clientAddr), &clientLen);

    if (clientSocket < 0) {
      logger.log(LogLevel::ERROR, "Failed to accept connection.");
      continue;
    }

    std::thread(&NexusRegistryServer::handleClient, this, clientSocket).detach();
  }
}

void NexusRegistryServer::stop() {
  isRunning = false;
  close(serverSocket);
  logger.log(LogLevel::INFO, "NexusRegistryServer stopped.");
}

void NexusRegistryServer::processRequest(const std::string &request, std::string &response) {
  Json::Reader reader;
  Json::Value root;
  if (!reader.parse(request, root)) {
    response = R"({"error": "Invalid JSON format"})";
    return;
  }

  std::string action = root["action"].asString();

  if (action == "register") {
    std::pair<double, double> coords = {roundToTwoDecimalPlaces(std::stod(root["x"].asString())),
                                        roundToTwoDecimalPlaces(std::stod(root["y"].asString()))};

    NodeInfo node = {root["type"].asString(), root["name"].asString(), root["ip"].asString(),
                     coords, root["port"].asInt()};
    registerNode(node);
    response = R"({"message": "Node registered successfully"})";
  } else if (action == "deregister") {
    deregisterNode(root["name"].asString());
    response = R"({"message": "Node deregistered successfully"})";
  } else if (action == "list") {
    response = getNodeList();
  } else if (action == "update") {
    NodeInfo node = {
        root["type"].asString(),
        root["name"].asString(),
        root["ip"].asString(),
        {root["x"].asDouble(), root["y"].asDouble()},
        root["port"].asInt()};
    updateNode(node);
    response = R"({"message":"Node updated successfully"})";
  } else {
    response = R"({"error": "Unknown action"})";
  }
}

void NexusRegistryServer::registerNode(const NodeInfo &node) {
  std::lock_guard<std::mutex> lock(nodesMutex);
  nodes.push_back(node);
  logger.log(LogLevel::INFO, "Registered node: " + node.type + " " + node.name +
                             " (" + node.ip + ":" + std::to_string(node.port) + ")" +
                             " at [" + std::to_string(node.coords.first) + ", " +
                             std::to_string(node.coords.second) + "].");
}

void NexusRegistryServer::deregisterNode(const std::string &name) {
  std::lock_guard<std::mutex> lock(nodesMutex);
  nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
                             [&name](const NodeInfo &node) { return node.name == name; }),
              nodes.end());
  logger.log(LogLevel::INFO, "Deregistered node: " + name);
}

void NexusRegistryServer::updateNode(const NodeInfo &node) {
  std::lock_guard<std::mutex> lock(nodesMutex);
  bool nodeUpdated = false;

  for (auto &existingNode : nodes) {
    if (existingNode.name == node.name) {
      existingNode.type = node.type;
      existingNode.ip = node.ip;
      existingNode.coords = node.coords;
      existingNode.port = node.port;
      nodeUpdated = true;
      logger.log(LogLevel::INFO, "Updated node: " + node.name + " (" + node.ip +
                                 ":" + std::to_string(node.port) + ") at [" +
                                 std::to_string(node.coords.first) + ", " +
                                 std::to_string(node.coords.second) + "].");
      break;
    }
  }

  if (!nodeUpdated) {
    logger.log(LogLevel::WARNING, "Node not found for update: " + node.name + ".");
  }
}

std::string NexusRegistryServer::getNodeList() {
  std::lock_guard<std::mutex> lock(nodesMutex);
  Json::Value root;
  try {
    for (const auto &node : nodes) {
      Json::Value n;
      n["type"] = node.type;
      n["name"] = node.name;
      n["ip"] = node.ip;
      n["port"] = node.port;
      n["x"] = node.coords.first;
      n["y"] = node.coords.second;
      root.append(n);
    }
  } catch (const std::exception &e) {
    logger.log(LogLevel::ERROR, "Exception while building node list: " + std::string(e.what()));
    return "{}"; // Return empty JSON object in case of error
  }
  Json::StreamWriterBuilder writer;
  return Json::writeString(writer, root);
}

std::string NexusRegistryServer::readFromSocket(int socket) {
  char buffer[4096]; // Increased buffer size for larger requests
  int bytesRead = recv(socket, buffer, sizeof(buffer) - 1, 0);
  if (bytesRead <= 0)
    return "";

  buffer[bytesRead] = '\0'; // Null-terminate the string
  std::string request(buffer);

  // Find the double CRLF that separates headers from the body
  size_t bodyPos = request.find("\r\n\r\n");
  if (bodyPos != std::string::npos) {
    return request.substr(bodyPos + 4); // Extract the body
  }

  return ""; // No request body found
}

void NexusRegistryServer::writeToSocket(int socket, const std::string &response) {
  std::string httpResponse = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: application/json\r\n"
                             "Content-Length: " +
                             std::to_string(response.size()) +
                             "\r\n"
                             "\r\n" +
                             response;

  send(socket, httpResponse.c_str(), httpResponse.size(), 0);
}

void NexusRegistryServer::handleClient(int clientSocket) {
  std::string request = readFromSocket(clientSocket);
  std::string response;
  processRequest(request, response);
  writeToSocket(clientSocket, response);
  close(clientSocket);
}
