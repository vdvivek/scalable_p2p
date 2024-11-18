#ifndef NODE_H
#define NODE_H

#include "NetworkManager.h"
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

class Node {
public:
  Node(std::string name, const std::string &ip, int port, std::pair<double, double> coords,
       NetworkManager &networkManager);
  virtual ~Node();

  std::string getId() const;
  std::string getName() const;
  std::string getIP() const;
  int getPort() const;
  std::pair<double, double> getCoords() const;
  void setCoords(const std::pair<double, double> &newCoords);

  virtual bool bind();
  virtual void updatePosition();

  virtual void receiveMessage(std::string &message);
  virtual void sendMessage(const std::string &targetName, const std::string &targetIP,
                           int targetPort, const std::string &message);

  virtual void sendTo(const std::string &targetIP, int targetPort, Packet &pkt);

  virtual void sendFile(const std::string &targetIP, int targetPort, const std::string &fileName);

  virtual std::string extractMessage(const std::string &payload, std::string &senderName,
                                     std::string &targetIP, int &targetPort);

protected:
  std::string id;
  std::string name;
  std::string ip;
  int port;
  std::pair<double, double> coords; // Coordinates (x, y)

  NetworkManager &networkManager;

  int socket_fd;
  struct sockaddr_in addr {};

private:
  static std::string generateUUID();
  void processMessage(Packet &pkt);
  void writeToFile(Packet &pkt);
  void reassembleFile(Packet &pkt);
};

#endif
