#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <string>
#include <memory>
#include <utility>
#include <netinet/in.h> // Required for sockaddr_in

class NetworkManager; // Forward declaration

class Node {
public:
    enum class NodeType {
        Ground,
        Satellite
    };

    // Constructor with initialization list
    Node(std::string name, const std::string &ip, int port, std::pair<double, double> coords, NetworkManager &networkManager,NodeType type);

    // Virtual destructor for polymorphism
    virtual ~Node();

    // Virtual methods for extensibility
    virtual void sendMessage(const std::string &targetName, const std::string &targetIP, int targetPort, const std::string &message);
    virtual void receiveMessage(std::string &message);
    virtual void printInfo() const; // Virtual function for runtime type identification (RTTI)
    std::string getNodeTypeAsString() const;

    // Utility methods
    bool bind();
    std::string getId() const;
    std::string getName() const;
    std::string getIP() const;
    int getPort() const;
    std::pair<double, double> getCoords() const;
    void setCoords(const std::pair<double, double> &newCoords);
    static std::string extractMessage(const std::string &payload, std::string &senderName, std::string &targetIP, int &targetPort);

protected:
    // Data members accessible to derived classes
    std::string id;                         // Node ID (UUID)
    std::string name;                       // Node name
    std::string ip;                         // IP address
    int port;                               // Port number
    std::pair<double, double> coords;       // Coordinates of the node
    NetworkManager &networkManager;         // Reference to the network manager
    NodeType nodeType;

private:
    // Data members for internal use
    int socket_fd;                          // File descriptor for the socket
    struct sockaddr_in addr;                // Address structure for the node

    // Utility function to generate a UUID
    static std::string generateUUID();
};

#endif // NODE_H
