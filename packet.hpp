#pragma once
#ifndef PACKET_HPP
#define PACKET_HPP

#include <array>
#include <stdint.h>
#include <vector>

namespace alice {
const int MAX_BUFFER_SIZE = 50 * 1000; // 50kb

struct packet {
  uint8_t version;
  uint32_t source;      // IPV4 Address of original sender
  uint32_t destination; // IPV4 Address of final reciever
  uint8_t flags;
  uint8_t length;
  uint16_t fragmentNumber;
  uint32_t errorCorrectionCode;

  std::array<uint8_t, MAX_BUFFER_SIZE> data;
};

std::vector<uint8_t> serialize(packet &pkt);

packet deserialize(std::vector<uint8_t>);

} // namespace alice
#endif