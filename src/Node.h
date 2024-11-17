#ifndef NODE_H
#define NODE_H

#include <string>
#include <netinet/in.h>

#include "NetworkManager.h"

class Node {
public:
    Node(std::string name, const std::string &ip, int port, std::pair<double, double> coords, NetworkManager &networkManager);
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
    virtual void sendMessage(const std::string &targetName, const std::string &targetIP, int targetPort, const std::string &message);

    static std::string extractMessage(const std::string &payload, std::string &senderName, std::string &targetIP, int &targetPort);

protected:
    std::string id;
    std::string name;
    std::string ip;
    int port;
    std::pair<double, double> coords; // Coordinates (x, y)

    NetworkManager &networkManager;

    int socket_fd;
    struct sockaddr_in addr{};

private:
    static std::string generateUUID();
};

#endif
