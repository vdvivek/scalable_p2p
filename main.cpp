#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <random>
#include <thread>

#include "packet.hpp"

// #define DEBUG

void sendMessage(int fd, const std::string &addr, int port, const std::vector<uint8_t> &msg) {
  struct sockaddr_in6 sAddr;
  sAddr.sin6_family = AF_INET6;
  sAddr.sin6_port = htons(port);

  if (inet_pton(AF_INET6, addr.c_str(), &sAddr.sin6_addr) <= 0) {
    printf("\nInvalid address: %s\n", addr.c_str());
    return;
  }

  printf("Sending to %s:%d\n", addr.c_str(), port);
  int err = sendto(fd, msg.data(), msg.size(), 0, (struct sockaddr *)&sAddr, sizeof(sAddr));
  if (err < 0) {
    printf("sendto failed: `%s:%d`\n", addr.c_str(), port);
    return;
  }
}

void getMessage(int fd) {
  const int bufSize = alice::MAX_BUFFER_SIZE;
  char buffer[bufSize];
  std::fill(buffer, buffer + bufSize, 0);
  while (true) {
    sockaddr_in6 clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    int length =
        recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &clientAddrLen);

    if (length < 0) {
      perror("recvfrom failed");
      break;
    }

    char addrBuff[50];
    inet_ntop(AF_INET6, &(clientAddr.sin6_addr), addrBuff, INET6_ADDRSTRLEN);

    buffer[length] = '\0';
#ifdef DEBUG
    printf("Recieved `%s` from %s:%d\n", buffer, addrBuff, clientAddr.sin6_port);
#endif
    printf("Bytes recieved: '%d'\n", length);

    std::ofstream img_file("img_1/img.webp", std::ios::binary | std::ios::app);
    img_file.write(buffer, sizeof(buffer) / sizeof(buffer[0]));
    img_file.close();
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
#ifdef DEBUG
  printf("Socket creation successful\n");
#endif
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

#ifdef DEBUG
  printf("PORT in network order: %d\n", sAddr.sin6_port);
#endif

  err = bind(fd, (struct sockaddr *)&sAddr, sizeof(sAddr));
  if (err < 0) {
    printf("Error: %d\n", err);
    perror("Socket binding failed\n");
    return 1;
  }

#ifdef DEBUG
  printf("Socket binding successful\n");
#endif

  if (port == 313131) {
    std::thread server(getMessage, fd);
    server.join();
  } else {
    // std::thread client(sendMessage, fd);

    std::ifstream img_file("img_0/img.webp", std::ios::binary);
    std::vector<uint8_t> data(std::istreambuf_iterator<char>(img_file), {});

    img_file.close();

    for (int i = 0; i < data.size(); i += alice::MAX_BUFFER_SIZE) {
      // #ifdef DEBUG
      printf("Sending from %d to %d\n", i, i + alice::MAX_BUFFER_SIZE);
      // #endif
      auto start = i;
      auto end = i + alice::MAX_BUFFER_SIZE;
      std::vector<uint8_t> x(data.begin() + start, data.begin() + end);
#ifdef DEBUG
      printf("%ld\n", x.size());
#endif
      sendMessage(fd, "::1", 313131, x);

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  // client.join();
  close(fd);
}