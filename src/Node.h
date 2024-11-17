#ifndef NODE_H
#define NODE_H

#include "NetworkManager.h"

#include <arpa/inet.h>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <netinet/in.h>
#include <random>
#include <sstream>
#include <string>
#include <unistd.h> // For close()

class Node {
public:
  Node(std::string name, const std::string &ip, int port, double x, double y, double z,
       NetworkManager &networkManager);
  virtual ~Node();

  std::string getId() const;
  std::string getName() const;
  std::string getIP() const;
  int getPort() const;
  double getX() const;
  double getY() const;
  double getZ() const;

  virtual bool bind();
  virtual void updatePosition();

  virtual void receiveMessage(std::string &message);
  virtual void sendMessage(const std::string &targetName, const std::string &targetIP,
                           int targetPort, const std::string &message);

  virtual void sendFile(const std::string &targetName, const std::string &targetIP, int targetPort,
                        const std::string &fileName);

  static std::string extractMessage(const std::string &payload, std::string &senderName,
                                    std::string &targetIP, int &targetPort);

protected:
  std::string id;
  std::string name;
  std::string ip;
  int port;
  double x, y, z;

  NetworkManager &networkManager;

  int socket_fd;
  struct sockaddr_in addr {};

private:
  static std::string generateUUID();
};

#endif
