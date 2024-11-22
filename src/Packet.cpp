#include "Packet.hpp"
#include <arpa/inet.h> // For htonl, ntohl, etc.
#include <stdexcept>

Packet::Packet() : version{PKT_VERSION} {
  std::fill(data.begin(), data.end(), 0);
}

Packet::Packet(uint32_t sAddr, uint16_t sPort, uint32_t tAddr, uint16_t tPort,
               packetType type)
    : version{PKT_VERSION}, sAddress{sAddr}, sPort{sPort}, tAddress{tAddr},
      tPort{tPort}, type{type}, errorCorrectionCode(0) {
  std::fill(data.begin(), data.end(), 0);
}

std::vector<uint8_t> Packet::serialize() {
  std::vector<uint8_t> buffer;
  buffer.reserve(sizeof(Packet));

  buffer.push_back(version);

  uint32_t sAddr = htonl(sAddress);
  buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&sAddr),
                reinterpret_cast<const uint8_t *>(&sAddr) + sizeof(sAddr));

  uint16_t sPrt = htons(sPort);
  buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&sPrt),
                reinterpret_cast<const uint8_t *>(&sPrt) + sizeof(sPrt));

  uint32_t tAddr = htonl(tAddress);
  buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&tAddr),
                reinterpret_cast<const uint8_t *>(&tAddr) + sizeof(tAddr));

  uint16_t tPrt = htons(tPort);
  buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&tPrt),
                reinterpret_cast<const uint8_t *>(&tPrt) + sizeof(tPrt));

  uint8_t typeByte = static_cast<uint8_t>(type);
  buffer.push_back(typeByte);

  uint16_t fragNum = htons(fragmentNumber);
  buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&fragNum),
                reinterpret_cast<const uint8_t *>(&fragNum) + sizeof(fragNum));

  uint16_t fragCount = htons(fragmentCount);
  buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&fragCount),
                reinterpret_cast<const uint8_t *>(&fragCount) +
                    sizeof(fragCount));

  uint32_t crc = htonl(errorCorrectionCode);
  buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&crc),
                reinterpret_cast<const uint8_t *>(&crc) + sizeof(crc));

  buffer.insert(buffer.end(), data.begin(), data.end());

  return buffer;
}

Packet Packet::deserialize(const std::vector<uint8_t> &buffer) {

  Packet packet;
  size_t offset = 0;

  packet.version = buffer[offset++];
  std::memcpy(&packet.sAddress, &buffer[offset], sizeof(packet.sAddress));
  packet.sAddress = ntohl(packet.sAddress);
  offset += sizeof(packet.sAddress);

  std::memcpy(&packet.sPort, &buffer[offset], sizeof(packet.sPort));
  packet.sPort = ntohs(packet.sPort);
  offset += sizeof(packet.sPort);

  std::memcpy(&packet.tAddress, &buffer[offset], sizeof(packet.tAddress));
  packet.tAddress = ntohl(packet.tAddress);
  offset += sizeof(packet.tAddress);

  std::memcpy(&packet.tPort, &buffer[offset], sizeof(packet.tPort));
  packet.tPort = ntohs(packet.tPort);
  offset += sizeof(packet.tPort);

  packet.type = static_cast<packetType>(buffer[offset++]);

  std::memcpy(&packet.fragmentNumber, &buffer[offset],
              sizeof(packet.fragmentNumber));
  packet.fragmentNumber = ntohs(packet.fragmentNumber);
  offset += sizeof(packet.fragmentNumber);

  std::memcpy(&packet.fragmentCount, &buffer[offset],
              sizeof(packet.fragmentCount));
  packet.fragmentCount = ntohs(packet.fragmentCount);
  offset += sizeof(packet.fragmentCount);

  std::memcpy(&packet.errorCorrectionCode, &buffer[offset],
              sizeof(packet.errorCorrectionCode));
  packet.errorCorrectionCode = ntohl(packet.errorCorrectionCode);
  offset += sizeof(packet.errorCorrectionCode);

  std::fill(packet.data.begin(), packet.data.end(), 0);
  std::memcpy(packet.data.data(), &buffer[offset], packet.data.size());

  return packet;
}

uint32_t Packet::calculateCRC(const std::vector<uint8_t> &data) {
  uint32_t crc = 0xFFFFFFFF;
  for (auto byte : data) {
    crc ^= byte;
    for (int i = 0; i < 8; ++i) {
      crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
    }
  }
  return ~crc;
}

void Packet::computeCRC() {
  auto serializedData = serialize();
  errorCorrectionCode = calculateCRC(serializedData);
}

bool Packet::verifyCRC() {
  auto serializedData = serialize();
  uint32_t computedCRC = calculateCRC(serializedData);
  return computedCRC == errorCorrectionCode;
}
