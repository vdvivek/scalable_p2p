#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <functional>
#include <iostream>
#include <random>
#include <thread>

void sendMessage(int fd) { // Need address and port to send message
  struct sockaddr_in6 sAddr;
  sAddr.sin6_family = AF_INET6;
  std::string addr;
  int port;

  std::cout << "Enter ADDRESS PORT" << std::endl;
  std::cin >> addr >> port;
  std::cin.clear();
  sAddr.sin6_port = htons(port);

  if (inet_pton(AF_INET6, addr.c_str(), &sAddr.sin6_addr) <= 0) {
    printf("\nInvalid address: %s\n", addr.c_str());
    return;
  }

  std::string input;
  while (true) {
    std::getline(std::cin, input);

    int err = sendto(fd, input.c_str(), input.size(), 0, (struct sockaddr *)&sAddr, sizeof(sAddr));
    if (err < 0) {
      perror("sendto failed");
      break;
    }
    printf("Sent message: `%s`\n", input.c_str());

    if (input[0] == 'q') {
      break;
    }
    // std::this_thread::sleep_for(std::chrono::milliseconds{random_number(100, 300)});
  }
}

void getMessage(int fd) {
  char buffer[1000];
  std::fill(buffer, buffer + 1000, 0);
  while (true) {
    printf("Waiting...\n");
    int length = recvfrom(fd, buffer, sizeof(buffer), 0, NULL, 0);

    if (length < 0) {
      perror("recvfrom failed");
      break;
    }

    buffer[length] = '\0';
    printf("Recieved message: `%s`\tBytes recieved: '%d'\n", buffer, length);

    if (buffer[0] == 'q') {
      break;
    }
  }
}

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Missing arguments\nUsage: main ADDRESS PORT\n");
    return 1;
  }

  std::string addr(argv[1]);
  int port = std::stoi(argv[2]);

  int fd;
  if ((fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
    printf("Socket creation failed\n");
    return 1;
  }
  printf("Socket creation successful\n");

  struct sockaddr_in6 sAddr;
  sAddr.sin6_family = AF_INET6;
  sAddr.sin6_port = htons(port);
  sAddr.sin6_flowinfo = 0;
  sAddr.sin6_scope_id = if_nametoindex("eth0");

  int err = inet_pton(AF_INET6, addr.c_str(), &sAddr.sin6_addr);
  if (err < 1) {
    printf("Error: %d\n", err);
    printf("Invalid address Address not supported \n");
    return -1;
  }

  err = bind(fd, (struct sockaddr *)&sAddr, sizeof(sAddr));
  if (err < 0) {
    printf("Error: %d\n", err);
    perror("Socket binding failed\n");
    return 1;
  }
  printf("Socket binding successful\n");

  std::thread server(getMessage, fd);
  std::thread client(sendMessage, fd);

  client.join();
  server.join();
  close(fd);
}