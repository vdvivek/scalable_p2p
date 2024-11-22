#include <atomic>
#include <cstring>
#include <iostream>
#include <thread>

#include <cstring>

#include "../src/Logger.h"
#include "../src/NetworkManager.h"
#include "../src/Node.h"
#include "../src/NodeType.h"

// #define DEBUG

const int UPDATE_INTERVAL = 30;

NetworkManager networkManager("http://127.0.0.1:5001");
std::atomic<bool> isRunning{true};

void printUsage() {
  std::cout << "[USAGE] ./nexus -node [ground|satellite] -name <NODE_NAME> -ip "
               "<IP_ADDRESS> -port "
               "<PORT> -x <X_COORD> -y <Y_COORD>"
            << std::endl;
}

void printCommands() {
  std::cout << "[USAGE] Available commands:\n";
  std::cout << "[USAGE] message - Send a message.\n";
  std::cout << "[USAGE] file    - Send a file.\n";
  std::cout << "[USAGE] list    - List nodes in the current p2p network.\n";
  std::cout << "[USAGE] help    - Display help.\n";
  std::cout << "[USAGE] q       - Quit the application.\n";
}

void receiverFunction(const std::shared_ptr<Node> &node) {
  while (isRunning) {
    std::string receivedMessage;
    node->receiveMessage(receivedMessage);
    if (!receivedMessage.empty()) {
      logger.log(LogLevel::INFO, "[NEXUS] Message received.");
      // Reprint the prompt after receiving a message
      std::cout << node->getName() << " prompt: " << std::flush;
    }
  }
}

void handleInput(const std::shared_ptr<Node> &node) {
  std::string command, targetName, targetIP, message;
  int targetPort;

  while (true) {
    std::cout << node->getName() << " prompt: ";
    std::cin >> command;

    // Available commands: message, list, file, quit
    if (command == "message") {
      std::cout << "Enter target node name: ";
      std::cin >> targetName;

      // Clear the input buffer before reading the message
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

      std::cout << "Enter message: ";
      std::getline(std::cin, message);

      if (!message.empty()) {
        node->sendMessage(targetName, message);
      } else {
        logger.log(LogLevel::ERROR, "Message cannot be empty.");
      }
    } else if (command == "file") {
      std::cout << "Enter target node name: ";
      std::cin >> targetName;

      // Clear the input buffer before reading the message
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

      std::cout << "Enter file name: ";
      std::cin >> message;

      if (message.empty()) {
        logger.log(LogLevel::ERROR, "File name cannot be empty.");
        continue;
      }

      node->sendFile(targetName, message);
    } else if (command == "list") {
      networkManager.listNodes();
    }
    // Handle "q" for quitting
    else if (command == "q") {
      logger.log(LogLevel::INFO, "[NEXUS] Exiting ...");
      isRunning = false; // Signal shutdown
      break;
    } else if (command == "help") {
      printCommands();
    } else {
      logger.log(LogLevel::ERROR, "Unknown command.");
      printCommands();
    }
    std::cout << std::endl;
  }
}

int main(int argc, char **argv) {
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

  NodeType::Type nodeTypeEnum = NodeType::fromString(nodeType);
  logger.log(LogLevel::INFO, "[NEXUS] NODE: " + nodeType);

  if (nodeTypeEnum != NodeType::GROUND && nodeTypeEnum != NodeType::SATELLITE) {
    logger.log(LogLevel::ERROR,
               "Invalid node type. Must be 'ground' or 'satellite'.");
    printUsage();
    return 3;
  }

  std::shared_ptr<Node> node;
  std::thread positionUpdateThread;

  if (nodeTypeEnum == NodeType::GROUND) {
    logger.log(LogLevel::INFO, "[NEXUS] Creating a Ground Node ...");
    node = std::make_shared<Node>(nodeTypeEnum, name, ip, port, coords,
                                  networkManager);
    node->updatePosition();
  } else {
    logger.log(LogLevel::INFO, "[NEXUS] Creating a Satellite Node ...");
    auto satelliteNode = std::make_shared<Node>(nodeTypeEnum, name, ip, port,
                                                coords, networkManager);
    node = satelliteNode;

    // Create a thread to update position periodically
    positionUpdateThread = std::thread([satelliteNode]() {
      while (isRunning) {
        satelliteNode->updatePosition();
        std::this_thread::sleep_for(std::chrono::seconds(
            UPDATE_INTERVAL)); // Update position every 2 second
      }
    });
  }

  networkManager.registerNodeWithRegistry(node);

  if (!node->bind()) {
    logger.log(LogLevel::ERROR, "Failed to bind the node. Exiting.");
    return 4;
  }

  logger.log(LogLevel::INFO, "[NEXUS] Node is ready for UDP communication at " +
                                 node->getIP() + ":" +
                                 std::to_string(node->getPort()));
  logger.log(LogLevel::INFO, "[NEXUS] Node is running. Press q to terminate.");

  std::thread receiverThread(receiverFunction, node);

  // Move to a function later
  std::thread fetchNodeThread([node]() {
    while (isRunning) {
#ifdef DEBUG
      logger.log(LogLevel::INFO,
                 "[NEXUS] Refreshing local network manager every " +
                     std::to_string(UPDATE_INTERVAL) + " seconds ...");
#endif
      networkManager.fetchNodesFromRegistry();
      networkManager.updateRoutingTable(node);
      std::this_thread::sleep_for(std::chrono::seconds(UPDATE_INTERVAL));
    }
  });

  handleInput(node);

  isRunning = false;
  networkManager.deregisterNodeWithRegistry(node);

  if (receiverThread.joinable()) {
    receiverThread.join();
  }

  if (positionUpdateThread.joinable()) {
    positionUpdateThread.join();
  }

  if (fetchNodeThread.joinable()) {
    fetchNodeThread.join();
  }

  return 0;
}