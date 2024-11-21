#include "../src/Logger.h"
#include "../src/NexusRegistryServer.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "Usage: registry_service PORT" << std::endl;
    return 1;
  }

  logger.setLogLevel(LogLevel::INFO);
  logger.log(LogLevel::INFO, "Starting Nexus Registry Server...");

  NexusRegistryServer server(
      std::stoi(argv[1])); // Port for the registry server
  server.start();
  return 0;
}