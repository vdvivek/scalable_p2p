#include "../src/NexusRegistryServer.h"

int main() {
    NexusRegistryServer server(5001); // Port for the registry server
    server.start();
    return 0;
}