import asyncio
from packet import Packet, PacketType
import math
import time
import struct

#written by Hritika Rahul Mehta and Ziqi Zheng

class SatelliteNode:
    def __init__(self, node_id, port, x, y):
        self.node_id = node_id
        self.port = port
        self.x = x
        self.y = y
        self.ground_nodes = []  
        self.peer_satellites = [] 

    def calculate_distance(self, ground_x, ground_y):
        """Calculate Euclidean distance to a ground node."""
        return math.sqrt((self.x - ground_x) ** 2 + (self.y - ground_y) ** 2)

    async def start_server(self):
        server = await asyncio.start_server(self.handle_connection, '127.0.0.1', self.port)
        print(f'SatelliteNode_{self.node_id} listening on port {self.port}')
        async with server:
            await server.serve_forever()

    async def update_position(self):
        """Simulate satellite movement."""
        while True:
            self.x += math.cos(time.time()) * 2 
            self.y += math.sin(time.time()) * 2  
            print(f"SatelliteNode_{self.node_id} moved to new position: ({self.x:.2f}, {self.y:.2f})")
            await asyncio.sleep(5)  

    def add_peer_satellite(self, satellite_ip, satellite_port, satellite_id):
        self.peer_satellites.append((satellite_ip, satellite_port, satellite_id))
        print(f"SatelliteNode_{self.node_id} added peer SatelliteNode_{satellite_id}")

    async def broadcast_position(self, ground_nodes):
        """Broadcast updated position to ground nodes."""
        while True:
            for ground_ip, ground_port, _, _, ground_id in ground_nodes: 
                try:
                    reader, writer = await asyncio.open_connection(ground_ip, ground_port)
                    position_packet = Packet(
                        source_id=self.node_id,
                        destination_id=0,  
                        packet_type=PacketType.DATA,
                        priority=1,
                        sequence_number=0,
                        payload=f"{self.x},{self.y}".encode()  
                    )
                    writer.write(position_packet.serialize())
                    await writer.drain()
                    writer.close()
                    await writer.wait_closed()
                except Exception as e:
                    print(f"Error broadcasting position to GroundNode_{ground_id} at {ground_ip}:{ground_port} - {e}")
            await asyncio.sleep(5)  

    async def handle_connection(self, reader, writer):
        try:
            data = await reader.read(4096)  
            if not data:
                print(f"SatelliteNode_{self.node_id} received an empty packet.")
                return

            packet = Packet.deserialize(data)

            if packet.type == PacketType.GROUND_COMM or packet.type == PacketType.FILE_TRANSFER:
                print(f"SatelliteNode_{self.node_id} received {'file transfer' if packet.type == PacketType.FILE_TRANSFER else 'message'} from GroundNode_{packet.source_id}")

            
                target_ground = next(
                    (node for node in self.ground_nodes if node[4] == packet.destination_id),  # Match ground_id
                    None
                )

                if target_ground and packet.type != PacketType.FILE_TRANSFER:
                    ground_ip, ground_port, _, _, ground_id = target_ground
                    print(f"SatelliteNode_{self.node_id} forwarding message to GroundNode_{ground_id} at {ground_ip}:{ground_port}")
                    await self.forward_packet(ground_ip, ground_port, packet)
                elif target_ground and packet.type == PacketType.FILE_TRANSFER:
                    
                    ground_ip, ground_port, _, _, ground_id = target_ground
                    print(f"SatelliteNode_{self.node_id} forwarding file transfer to GroundNode_{ground_id} at {ground_ip}:{ground_port}")
                    await self.forward_packet(ground_ip, ground_port, packet)
                else:
                    print(f"SatelliteNode_{self.node_id} could not find GroundNode_{packet.destination_id}. Forwarding to peers.")
                    await self.forward_to_peers(packet)

        except Exception as e:
            print(f"Error handling connection in SatelliteNode_{self.node_id}: {e}")

        finally:
            writer.close()
            await writer.wait_closed()

    async def forward_packet(self, ip, port, packet):
        """Forward the packet to a specific ground node or satellite."""
        try:
            reader, writer = await asyncio.open_connection(ip, port)
            writer.write(packet.serialize())
            await writer.drain()
            writer.close()
            await writer.wait_closed()
            print(f"SatelliteNode_{self.node_id} forwarded packet to {ip}:{port}")
        except Exception as e:
            print(f"Error forwarding packet to {ip}:{port} - {e}")

    async def forward_to_peers(self, packet):
        """Forward the packet to all peer satellites."""
        for peer_ip, peer_port, peer_id in self.peer_satellites:
            try:
                await self.forward_packet(peer_ip, peer_port, packet)
                print(f"SatelliteNode_{self.node_id} forwarded packet to SatelliteNode_{peer_id}")
            except Exception as e:
                print(f"Error forwarding packet to SatelliteNode_{peer_id}: {e}")

    def add_ground_node(self, ground_ip, ground_port, ground_x, ground_y, ground_id):
        self.ground_nodes.append((ground_ip, ground_port, ground_x, ground_y, ground_id))
        print(f"SatelliteNode_{self.node_id} added GroundNode_{ground_id} at ({ground_x}, {ground_y})")


if __name__ == "__main__":
    import sys

    if len(sys.argv) < 6:
        print("Usage: <node_id> <port> <x> <y> [<ground_ip> <ground_port> <ground_x> <ground_y> <ground_id>] ... peer <peer_ip> <peer_port> <peer_id> ...")
        sys.exit(1)

    node_id = int(sys.argv[1])
    port = int(sys.argv[2])
    x = float(sys.argv[3])
    y = float(sys.argv[4])

    satellite_node = SatelliteNode(node_id, port, x, y)

 
    i = 5
    while i < len(sys.argv):
        if sys.argv[i] == "peer":
            break  
        try:
            ground_ip = sys.argv[i]
            ground_port = int(sys.argv[i + 1])
            ground_x = float(sys.argv[i + 2])
            ground_y = float(sys.argv[i + 3])
            ground_id = int(sys.argv[i + 4])
            satellite_node.add_ground_node(ground_ip, ground_port, ground_x, ground_y, ground_id)
            i += 5
        except IndexError:
            print("Error: Incorrect number of arguments provided for ground nodes.")
            print("Expected format: <ground_ip> <ground_port> <ground_x> <ground_y> <ground_id>")
            sys.exit(1)
        except ValueError as e:
            print(f"Error parsing ground node arguments: {e}")
            sys.exit(1)

    
    while i < len(sys.argv):
        if sys.argv[i] == "peer":
            i += 1  
            continue
        try:
            peer_ip = sys.argv[i]
            peer_port = int(sys.argv[i + 1])
            peer_id = int(sys.argv[i + 2])
            satellite_node.add_peer_satellite(peer_ip, peer_port, peer_id)
            i += 3
        except IndexError:
            print("Error: Incorrect number of arguments provided for peer satellites.")
            print("Expected format: peer <peer_ip> <peer_port> <peer_id>")
            sys.exit(1)
        except ValueError as e:
            print(f"Error parsing peer satellite arguments: {e}")
            sys.exit(1)

    loop = asyncio.get_event_loop()
    try:
        loop.run_until_complete(
            asyncio.gather(
                satellite_node.start_server(),
                satellite_node.update_position(),
                satellite_node.broadcast_position(satellite_node.ground_nodes)
            )
        )
    except KeyboardInterrupt:
        print(f"SatelliteNode_{node_id} shutting down.")
    finally:
        loop.close()