#include <memory>

#include "Logger.h"
#include "Node.h"
#include "Utility.h"

Node::Node(NodeType::Type nodeType, std::string name, const std::string &ip,
           int port, std::pair<double, double> coords,
           const NetworkManager &networkManager)
    : type(nodeType), id(generateUUID()), name(std::move(name)), ip(ip),
      port(port), coords(std::move(coords)), networkManager{networkManager},
      cryptoManager(std::unique_ptr<CryptoManager>(new CryptoManager())) {
  socket_fd = -1;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
  delay = 0;
}

std::string Node::getId() const { return id; }
std::string Node::getName() const { return name; }
std::string Node::getIP() const { return ip; }
int Node::getPort() const { return port; }

std::pair<double, double> Node::getCoords() const { return coords; }
void Node::setCoords(const std::pair<double, double> &newCoords) {
  coords = newCoords;
}

NodeType::Type Node::getType() const { return type; }

bool Node::bind() {
  // Create a UDP socket
  socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd < 0) {
    logger.log(LogLevel::ERROR, "Failed to create socket for Node " + name +
                                    ": " + strerror(errno));
    return false;
  }

  // Bind the socket to the IP and Port
  if (::bind(socket_fd, reinterpret_cast<struct sockaddr *>(&addr),
             sizeof(addr)) < 0) {
    logger.log(LogLevel::ERROR, "Failed to bind socket for Node " + name +
                                    ": " + strerror(errno));
    close(socket_fd);
    socket_fd = -1;
    return false;
  }

  logger.log(LogLevel::INFO, "[NEXUS] Node " + name +
                                 " successfully bound to " + ip + ":" +
                                 std::to_string(port));
  return true;
}

void Node::updatePosition() {
  if (type == NodeType::Type::SATELLITE) {
    // Satellite nodes move based on below logic
    coords.first = roundToTwoDecimalPlaces(coords.first + 0.05);
    coords.second = roundToTwoDecimalPlaces(coords.second + 0.075);
    logger.log(LogLevel::INFO, "[NEXUS] Satellite " + name +
                                   " new position: (" +
                                   std::to_string(coords.first) + ", " +
                                   std::to_string(coords.second) + ")");
    networkManager.updateNodeInRegistry(shared_from_this());
  } else if (type == NodeType::Type::GROUND) {
    // Ground nodes are stationary
    logger.log(LogLevel::INFO, "[NEXUS] Ground node " + name +
                                   " remains stationary at (" +
                                   std::to_string(coords.first) + ", " +
                                   std::to_string(coords.second) + ")");
  }
}

void Node::receiveMessage(std::string &message) {
  if (socket_fd < 0) {
    logger.log(LogLevel::ERROR,
               "Socket is not initialized for receiving messages.");
    return;
  }

  // Added extra 32 for non payload data
  constexpr size_t bufferSize = MAX_BUFFER_SIZE + 32;
  char buffer[bufferSize];

  struct sockaddr_in senderAddr {};
  socklen_t senderAddrLen = sizeof(senderAddr);

  ssize_t bytesReceived = recvfrom(
      socket_fd, buffer, bufferSize - 1, 0,
      reinterpret_cast<struct sockaddr *>(&senderAddr), &senderAddrLen);

  if (bytesReceived < 0) {
    logger.log(LogLevel::ERROR,
               "Failed to receive message: " + std::string(strerror(errno)));
    return;
  }

  // Null-terminate the buffer to treat it as a string
  buffer[bytesReceived] = '\0';
  std::vector<uint8_t> receivedMessage(buffer, buffer + bufferSize);
  logger.log(LogLevel::INFO,
             "[NEXUS] Buffer size: " + std::to_string(receivedMessage.size()));

  char senderIP[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &senderAddr.sin_addr, senderIP, sizeof(senderIP));
  int senderPort = ntohs(senderAddr.sin_port);

  std::string senderName, targetIP;
  int targetPort = 0;
  logger.log(LogLevel::INFO, "[NEXUS] Received message from " +
                                 std::string(senderIP) + ":" +
                                 std::to_string(senderPort));
  Packet pkt = pkt.deserialize(receivedMessage);

  if ((pkt.tAddress == addr.sin_addr.s_addr) && (pkt.tPort == htons(port))) {
    processMessage(pkt);
  } else {
    char nextIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(pkt.tAddress), nextIP, sizeof(nextIP));
    int nextPort = ntohs(pkt.tPort);

    logger.log(LogLevel::INFO, "[NEXUS] Forwarding to " + std::string(nextIP) +
                                   ":" + std::to_string(nextPort));
    sendTo(nextIP, nextPort, pkt);
  }
}

void Node::sendMessage(const std::string &targetName,
                       const std::string &message) {
  auto targetNode = networkManager.findNode(targetName);

  if (targetNode == nullptr) {
    logger.log(LogLevel::ERROR, "[NEXUS] Node " + targetName + " not found.");
    return;
  }

  logger.log(LogLevel::INFO, "[NEXUS] Node " + name + " sending message from " +
                                 ip + ":" + std::to_string(port));

  struct sockaddr_in targetAddr = {};
  targetAddr.sin_family = AF_INET;
  targetAddr.sin_port = htons(targetNode->getPort());
  if (inet_pton(AF_INET, targetNode->getIP().c_str(), &targetAddr.sin_addr) <=
      0) {
    logger.log(LogLevel::ERROR,
               "Invalid target IP address: " + targetNode->getIP());
    return;
  }

  Packet pkt(addr.sin_addr.s_addr, addr.sin_port, targetAddr.sin_addr.s_addr,
             targetAddr.sin_port, packetType::TEXT);

  std::copy(message.begin(), message.end(), pkt.data.begin());

  auto packet_data = pkt.serialize();
  logger.log(LogLevel::INFO,
             "[NEXUS] Packet size: " + std::to_string(packet_data.size()));

  if (packet_data.empty()) {
    logger.log(LogLevel::ERROR, "Failed to serialize packet. No data to send.");
    return;
  }

  auto nextHop = networkManager.getNextHop(targetName);
  if (!nextHop) {
    logger.log(LogLevel::ERROR, "No path to target found");
    return;
  }
  std::cout << "Sending to " << nextHop->name << " " << nextHop->ip << ":"
            << nextHop->port << std::endl;

  sendTo(nextHop->getIP(), nextHop->getPort(), pkt);
}

void Node::sendTo(const std::string &targetIP, int targetPort, Packet &pkt) {
  struct sockaddr_in targetAddr = {};
  targetAddr.sin_family = AF_INET;
  targetAddr.sin_port = htons(targetPort);
  inet_pton(AF_INET, targetIP.c_str(), &targetAddr.sin_addr);

  auto pkt_data = pkt.serialize();
  auto p2 = pkt.deserialize(pkt_data);

  const int bytesSent = sendto(socket_fd, pkt_data.data(), pkt_data.size(), 0,
                               reinterpret_cast<struct sockaddr *>(&targetAddr),
                               sizeof(targetAddr));

  if (bytesSent < 0) {
    logger.log(LogLevel::ERROR,
               "Failed to send data: " + std::string(strerror(errno)));
  } else {
    logger.log(LogLevel::INFO, "[NEXUS] Sent " + std::to_string(bytesSent) +
                                   " to " + targetIP + ":" +
                                   std::to_string(targetPort));
  }
}

void Node::sendFile(const std::string &targetName,
                    const std::string &fileName) {
  auto targetNode = networkManager.findNode(targetName);
  if (targetNode == nullptr) {
    logger.log(LogLevel::ERROR, "[NEXUS] Node " + targetName + " not found.");
    return;
  }
  logger.log(LogLevel::INFO, "[NEXUS] Node " + name + " sending message from " +
                                 ip + ":" + std::to_string(port));

  struct sockaddr_in targetAddr = {};
  targetAddr.sin_family = AF_INET;
  targetAddr.sin_port = htons(targetNode->getPort());
  if (inet_pton(AF_INET, targetNode->getIP().c_str(), &targetAddr.sin_addr) <=
      0) {
    logger.log(LogLevel::ERROR,
               "Invalid target IP address: " + targetNode->getIP());
    return;
  }

  std::ifstream fileHandle(fileName, std::ios::binary | std::ios::ate);
  size_t file_size = fileHandle.tellg();
  int fragCount = std::ceil(static_cast<double>(file_size) / MAX_BUFFER_SIZE);

  Packet pkt(addr.sin_addr.s_addr, addr.sin_port, targetAddr.sin_addr.s_addr,
             targetAddr.sin_port, packetType::FILE);
  pkt.fragmentCount = fragCount;

  logger.log(LogLevel::INFO,
             "[NEXUS] Fragment Count: " + std::to_string(pkt.fragmentCount));

  fileHandle.seekg(0, std::ios::beg);
  int fragNumber = 1;
  std::array<uint8_t, MAX_BUFFER_SIZE> buffer;

  for (int i = 0; i < fragCount; i++) {
    fileHandle.read(reinterpret_cast<char *>(buffer.data()), MAX_BUFFER_SIZE);
    auto byteCount = fileHandle.gcount();
    logger.log(LogLevel::INFO,
               "[NEXUS] Read:  " + std::to_string(byteCount) + " bytes.");
    logger.log(LogLevel::INFO,
               "[NEXUS] Sending fragment: " + std::to_string(fragNumber));

    pkt.fragmentNumber = fragNumber++;
    pkt.data = buffer;

    auto nextHop = networkManager.getNextHop(targetName);
    sendTo(nextHop->getIP(), nextHop->getPort(), pkt);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  fileHandle.close();
  logger.log(LogLevel::INFO, "[NEXUS] File sent.");
}

std::string Node::extractMessage(const std::string &payload,
                                 std::string &senderName, std::string &targetIP,
                                 int &targetPort) {
  std::istringstream iss(payload);
  std::string portStr;
  std::string actualMessage;

  // Parse the payload (format: senderName targetIP targetPort message)
  if (iss >> senderName >> targetIP >> portStr) {
    try {
      targetPort = std::stoi(portStr); // Convert port to integer
    } catch (const std::exception &e) {
      logger.log(LogLevel::ERROR,
                 "Invalid port in message payload: " + portStr);
      return "";
    }

    // Extract the remaining part of the payload as the actual message
    std::getline(iss, actualMessage);
    actualMessage.erase(
        0, actualMessage.find_first_not_of(" ")); // Trim leading spaces
  } else {
    logger.log(LogLevel::ERROR, "Malformed message payload: " + payload);
  }

  return actualMessage;
}

std::string Node::generateUUID() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 15);
  std::uniform_int_distribution<> dis2(8, 11);

  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (int i = 0; i < 8; i++)
    ss << dis(gen);
  ss << "-";
  for (int i = 0; i < 4; i++)
    ss << dis(gen);
  ss << "-4"; // UUID version 4
  for (int i = 0; i < 3; i++)
    ss << dis(gen);
  ss << "-";
  ss << dis2(gen);
  for (int i = 0; i < 3; i++)
    ss << dis(gen);
  ss << "-";
  for (int i = 0; i < 12; i++)
    ss << dis(gen);
  return ss.str();
}

void Node::simulateSignalDelay() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(
      0.1, 2.0); // Random delay between 0.1s and 2.0s
  delay = dis(gen);
  std::this_thread::sleep_for(
      std::chrono::milliseconds(static_cast<int>(delay * 1000)));
}

void Node::processMessage(Packet &pkt) {
  char IP[INET_ADDRSTRLEN + 1];
  inet_ntop(AF_INET, &(pkt.sAddress), IP, sizeof(IP));
  int PORT = ntohs(pkt.tPort);
  IP[INET_ADDRSTRLEN] = '\0';

  if (pkt.type == packetType::FILE) {
    logger.log(LogLevel::INFO, "[NEXUS] Writing to file, Fragment: " +
                                   std::to_string(pkt.fragmentNumber) + "/" +
                                   std::to_string(pkt.fragmentCount));
    writeToFile(pkt);
    reassembleFile(pkt);
  } else {
    logger.log(LogLevel::INFO,
               "[NEXUS] Message:" +
                   std::string(pkt.data.begin(), pkt.data.end()));
  }
}

void Node::writeToFile(Packet &pkt) {
  std::ostringstream oss;
  oss << "/tmp/" << pkt.sAddress << "_" << pkt.sPort << "_"
      << pkt.fragmentNumber;
  std::string filename = oss.str();

  std::ofstream ofs(filename, std::ios::binary);
  ofs.write(reinterpret_cast<char *>(pkt.data.data()), pkt.data.size());
  ofs.close();
}

void Node::reassembleFile(Packet &pkt) {
  std::ostringstream oss;
  oss << pkt.sAddress << "_" << pkt.sPort;
  std::string prefix = oss.str();

  std::string path = "/tmp/";
  DIR *dir = opendir(path.c_str());
  if (dir == nullptr) {
    logger.log(LogLevel::ERROR,
               "[REASSEMBLY] Failed to open directory: " + path);
    return;
  }

  int count = 0;
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    std::string filename(entry->d_name);
    if (filename.rfind(prefix, 0) == 0) {
      count++;
    }
  }
  closedir(dir);

  if (count != pkt.fragmentCount) {
    logger.log(LogLevel::WARNING,
               "[REASSEMBLY] Missing fragments: " +
                   std::to_string(pkt.fragmentCount - count));
    return;
  }

  std::ofstream output(path + "final_" + prefix, std::ios::binary);
  for (int i = 1; i <= count; i++) {
    std::string filename = path + prefix + "_" + std::to_string(i);
    logger.log(LogLevel::INFO, "[REASSEMBLY] Reading fragment: " + filename);
    std::ifstream fragment(filename, std::ios::binary);
    output << fragment.rdbuf();
    fragment.close();
    logger.log(LogLevel::INFO, "[REASSEMBLY] Deleting fragment: " + filename);
    remove(filename.c_str());
  }
  output.close();
}

Node::~Node() {
  if (socket_fd >= 0) {
    close(socket_fd);
    socket_fd = -1;
  }
}
