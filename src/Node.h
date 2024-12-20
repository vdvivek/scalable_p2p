#ifndef NODE_H
#define NODE_H

#include "CryptoManager.h"
#include "NetworkManager.h"
#include "NodeType.h"
#include "Packet.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <netinet/in.h>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h> // For close()

class Node : public std::enable_shared_from_this<Node> {
public:
  Node(NodeType::Type nodeType, std::string name, const std::string &ip,
       int port, std::pair<double, double> coords,
       const NetworkManager &networkManager);
  ~Node();

  std::string getId() const;
  std::string getName() const;
  std::string getIP() const;
  int getPort() const;

  std::pair<double, double> getCoords() const;
  void setCoords(const std::pair<double, double> &newCoords);

  NodeType::Type getType() const;

  bool bind();
  void updatePosition();

  void receiveMessage(std::string &message);
  void sendMessage(const std::string &targetName, const std::string &message);

  void sendTo(const std::string &targetIP, int targetPort, Packet &pkt);

  void sendFile(const std::string &targetName, const std::string &fileName);

  static std::string extractMessage(const std::string &payload,
                                    std::string &senderName,
                                    std::string &targetIP, int &targetPort);

  std::string getPublicKey() const { return cryptoManager->getPublicKey(); }

  std::vector<uint8_t>
  encryptMessage(const std::string &message,
                 const std::string &recipientPublicKey) const {
    return cryptoManager->encrypt(message, recipientPublicKey);
  }

  std::string decryptMessage(const std::vector<uint8_t> &ciphertext) const {
    return cryptoManager->decrypt(ciphertext);
  }

protected:
  NodeType::Type type;

  std::string id;
  std::string name;
  std::string ip;
  int port;
  std::pair<double, double> coords; // Coordinates (x, y)

  const NetworkManager &networkManager;

  int socket_fd;
  struct sockaddr_in addr {};
  double delay; // delay in seconds

private:
  std::unique_ptr<CryptoManager> cryptoManager;

  void simulateSignalDelay();

  static std::string generateUUID();
  static void processMessage(Packet &pkt);
  static void writeToFile(Packet &pkt);
  static void reassembleFile(Packet &pkt);
};

#endif
