#include "packet.hpp"

namespace alice {
std::vector<uint8_t> serialize(packet &pkt) { return std::vector<uint8_t>(); }

packet deserialize(std::vector<uint8_t>) { return packet(); }
} // namespace alice