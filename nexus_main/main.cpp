#include <atomic>
#include <iostream>
#include <thread>

#include <string.h>

#include "../src/NetworkManager.h"
#include "../src/Node.h"

NetworkManager networkManager("http://127.0.0.1:5001");
std::atomic<bool> isRunning{true};

void receiverFunction(const std::shared_ptr<Node> &node) {
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

      std::cout << "Enter target IP: ";
      std::cin >> targetIP;

      std::cout << "Enter target port: ";
      std::cin >> targetPort;

      // Clear the input buffer before reading the message
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

      std::cout << "Enter message: ";
      std::getline(std::cin, message);

      if (!message.empty()) {
        std::string fullMessage =
            node->getName() + " " + targetIP + " " + std::to_string(targetPort) + " " + message;
        node->sendMessage(targetName, targetIP, targetPort, fullMessage);
      } else {
        std::cerr << "[ERROR] Message cannot be empty." << std::endl;
      }
    } else if (command == "file") {
      std::cout << "Enter target IP: ";
      std::cin >> targetIP;

      std::cout << "Enter target port: ";
      std::cin >> targetPort;

      std::cout << "Enter file name: ";
      std::cin >> message;

      if (message.empty()) {
        std::cerr << "[ERROR] File name cannot be empty." << std::endl;
        continue;
      }

      node->sendFile(targetIP, targetPort, message);

    } else if (command == "list") {
      networkManager.listNodes(); // Call listNodes to print node details
    }
    // Handle "q" for quitting
    else if (command == "q") {
      std::cout << "[INFO] Goodbye..." << std::endl;
      isRunning = false; // Signal shutdown
      break;
    }
    // Handle unknown commands
    else {
      std::cerr << "[ERROR] Unknown command.\n";
      std::cout << "[INFO] Available commands:\n";
      std::cout << "[USAGE] send - Send a message\n";
      std::cout << "[USAGE] q - Quit the application\n";
    }
  }
}

void printUsage() {
  std::cout << "[USAGE] ./nexus -node [ground|satellite] -name <NODE_NAME> -ip <IP_ADDRESS> -port "
               "<PORT> -x <X_COORD> -y <Y_COORD> -z <Z_COORD>"
            << std::endl;
}

int main(int argc, char **argv) {
  if (argc != 15) {
    printUsage();
    return 1;
  }

  std::string nodeType;
  std::string name;
  std::string ip;
  int port = 0;
  double x = 0.0, y = 0.0, z = 0.0;

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
      x = std::stod(argv[++i]);
    } else if (strcmp(argv[i], "-y") == 0) {
      y = std::stod(argv[++i]);
    } else if (strcmp(argv[i], "-z") == 0) {
      z = std::stod(argv[++i]);
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

  std::shared_ptr<Node> node = std::make_shared<Node>(name, ip, port, x, y, z, networkManager);
  std::cout << "[INFO] Creating a Node..." << std::endl;
  networkManager.registerNodeWithRegistry(node);

  if (!node->bind()) {
    std::cerr << "[ERROR] Failed to bind the node. Exiting." << std::endl;
    return 4;
  }

  std::cout << "[NEXUS] " << node->getName() << " is ready for UDP communication at "
            << node->getIP() << ":" << node->getPort() << "" << std::endl;
  std::cout << "[NEXUS] Node is running. Press Ctrl+C OR q to terminate." << std::endl;

  std::thread receiverThread(receiverFunction, node);

  handleInput(node);

  if (!networkManager.registerNodeWithRegistry(node)) {
    std::cerr << "[ERROR] Failed to register node with the registry server." << std::endl;
    return 5;
  }

  networkManager.fetchNodesFromRegistry();
  // Wait for receiver thread to finish
  if (receiverThread.joinable()) {
    receiverThread.join();
  }

  return 0;
}
