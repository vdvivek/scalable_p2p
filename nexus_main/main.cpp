#include <iostream>
#include <thread>
#include <atomic>
#include <cstring>

#include "../src/Node.h"
#include "../src/NetworkManager.h"
#include "../src/GroundNode.h"
#include "../src/SatelliteNode.h"

NetworkManager networkManager("http://127.0.0.1:5001");
std::atomic<bool> isRunning{true};

void printUsage() {
    std::cout << "[USAGE] ./nexus -node [ground|satellite] -name <NODE_NAME> -ip <IP_ADDRESS> -port <PORT> -x <X_COORD> -y <Y_COORD>" << std::endl;
}

void printCommands() {
    std::cout << "[USAGE] Available commands:\n";
    std::cout << "[USAGE] send  - Send a message.\n";
    std::cout << "[USAGE] list  - List nodes in the current p2p network.\n";
    std::cout << "[USAGE] help  - Display help.\n";
    std::cout << "[USAGE] q     - Quit the application.\n";
}

void receiverFunction(const std::shared_ptr<Node>& node) {
    while (isRunning) {
        std::string receivedMessage;
        node->receiveMessage(receivedMessage);
        if (!receivedMessage.empty()) {
            std::cout << "[MESSAGE RECEIVED]" << std::endl;
            // Reprint the prompt after receiving a message
            std::cout << node->getName() << " prompt: " << std::flush;
        }
    }
}

void handleInput(const std::shared_ptr<Node>& node) {
    std::string command, targetName, targetIP, message;
    int targetPort;

    while (true) {
        std::cout << node->getName() << " prompt: ";
        std::cin >> command;

        // Handle "send" command
        if (command == "send") {
            std::cout << "Enter target node name: ";
            std::cin >> targetName;

            std::cout << "Enter target IP: ";
            std::cin >> targetIP;

            std::cout << "Enter target port: ";
            std::cin >> targetPort;

            // Clear the input buffer before reading the message
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            std::cout << "Enter message: ";
            std::getline(std::cin, message);

            if (!message.empty()) {
                std::string fullMessage = node->getName() + " " + targetIP + " " + std::to_string(targetPort) + " " + message;
                node->sendMessage(targetName, targetIP, targetPort, fullMessage);
            } else {
                std::cerr << "[ERROR] Message cannot be empty." << std::endl;
            }
        }
        else if (command == "list") {
            networkManager.listNodes(); // Call listNodes to print node details
        }
        // Handle "q" for quitting
        else if (command == "q") {
            std::cout << "[INFO] Goodbye..." << std::endl;
            isRunning = false; // Signal shutdown
            break;
        } else if (command == "help") {
            printCommands();
        }
        // Handle unknown commands
        else {
            std::cerr << "[ERROR] Unknown command.\n";
            printCommands();
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 13) {
        printUsage();
        return 1;
    }

    std::string nodeType;
    std::string name;
    std::string ip;
    int port = 0;
    std::pair<double, double> coords{0.0, 0.0};

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-node") == 0) {
            nodeType = argv[++i];
        } else if (strcmp(argv[i], "-name") == 0) {
            name = argv[++i];
        } else if (strcmp(argv[i], "-ip") == 0) {
            ip = argv[++i];
        } else if (strcmp(argv[i], "-port") == 0) {
            port = std::stoi(argv[++i]);
        } else if (strcmp(argv[i], "-x") == 0) {
            coords.first = std::stod(argv[++i]);
        } else if (strcmp(argv[i], "-y") == 0) {
            coords.second = std::stod(argv[++i]);
        } else {
            printUsage();
            return 2;
        }
    }

    if (nodeType != "ground" && nodeType != "satellite") {
        std::cerr << "[ERROR] Invalid node type. Must be 'ground' or 'satellite'." << std::endl;
        printUsage();
        return 3;
    }

    std::shared_ptr<Node> node;
    std::thread positionUpdateThread;

    if (nodeType == "ground") {
        node = std::make_shared<GroundNode>(name, ip, port, coords, networkManager);
        std::cout << "[INFO] Creating a GroundNode..." << std::endl;
    } else if (nodeType == "satellite") {
        auto satelliteNode = std::make_shared<SatelliteNode>(name, ip, port, coords, networkManager);
        node = satelliteNode;

        // Create a thread to update position periodically
        positionUpdateThread = std::thread([satelliteNode]() {
            while (isRunning) {
                satelliteNode->updatePosition();
                std::this_thread::sleep_for(std::chrono::seconds(2)); // Update position every second
            }
        });


        std::cout << "[INFO] Creating a SatelliteNode..." << std::endl;
    }

    networkManager.registerNodeWithRegistry(node);

    if (!node->bind()) {
        std::cerr << "[ERROR] Failed to bind the node. Exiting." << std::endl;
        return 4;
    }

    std::cout << "[NEXUS] " << node->getName() << " is ready for UDP communication at " << node->getIP() << ":" << node->getPort() << "" << std::endl;
    std::cout << "[NEXUS] Node is running. Press Ctrl+C OR q to terminate." << std::endl;

    std::thread receiverThread(receiverFunction, node);

    handleInput(node);

    if (!networkManager.registerNodeWithRegistry(node)) {
        std::cerr << "[ERROR] Failed to register node with the registry server." << std::endl;
        return 5;
    }

    networkManager.fetchNodesFromRegistry();

    isRunning = false;

    if (receiverThread.joinable()) {
        receiverThread.join();
    }

    if (positionUpdateThread.joinable()) {
        positionUpdateThread.join();
    }

    return 0;
}
