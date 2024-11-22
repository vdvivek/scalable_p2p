import struct
import time
from enum import Enum

# PacketType Enum

# Written by Hritika Rahul Mehta and Ziqi Zheng
class PacketType(Enum):
    DATA = 1
    ACK = 2
    HANDSHAKE = 3
    GROUND_COMM = 4
    FILE_TRANSFER = 5  

class Packet:
    def __init__(self, source_id, destination_id, packet_type, priority, sequence_number, payload):
        self.source_id = source_id
        self.destination_id = destination_id
        self.type = packet_type
        self.priority = priority
        self.sequence_number = sequence_number
        self.timestamp = int(time.time())
        self.payload = payload

    def serialize(self):
        buffer = bytearray()
        buffer.extend(struct.pack('!I', self.source_id))
        buffer.extend(struct.pack('!I', self.destination_id))
        buffer.append(self.type.value)
        buffer.append(self.priority)
        buffer.extend(struct.pack('!I', self.sequence_number))
        buffer.extend(struct.pack('!Q', self.timestamp))
        buffer.extend(self.payload)
        return buffer

    @staticmethod
    def deserialize(buffer):
        offset = 0
        source_id = struct.unpack_from('!I', buffer, offset)[0]
        offset += 4
        destination_id = struct.unpack_from('!I', buffer, offset)[0]
        offset += 4
        packet_type_value = buffer[offset]
        packet_type = PacketType(packet_type_value)
        offset += 1
        priority = buffer[offset]
        offset += 1
        sequence_number = struct.unpack_from('!I', buffer, offset)[0]
        offset += 4
        timestamp = struct.unpack_from('!Q', buffer, offset)[0]
        offset += 8
        payload = buffer[offset:]
        return Packet(source_id, destination_id, packet_type, priority, sequence_number, payload)
