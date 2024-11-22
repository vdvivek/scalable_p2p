import asyncio
import math
import struct
import time
import os
from packet import Packet, PacketType
from uuid import uuid4
import random

# Written by Hritika Rahul Mehta and Ziqi Zheng

class GroundNode:
    def __init__(self, node_id, port, x, y):
        self.node_id = node_id
        self.port = port
        self.x = x
        self.y = y
        self.satellites = [] 
        self.receiving_files = {}  

    def calculate_distance(self, satellite_x, satellite_y):
        """Calculate Euclidean distance to a satellite."""
        return math.sqrt((self.x - satellite_x) ** 2 + (self.y - satellite_y) ** 2)

    async def start_server(self):
        server = await asyncio.start_server(self.handle_connection, '127.0.0.1', self.port)
        print(f'GroundNode_{self.node_id} listening on port {self.port}')
        async with server:
            while True:
                await asyncio.sleep(3600)  

    async def handle_connection(self, reader, writer):
        try:
            data = await reader.read(4096)  
            if not data:
                print(f"GroundNode_{self.node_id} received an empty packet.")
                return

            packet = Packet.deserialize(data)
            if packet.type == PacketType.DATA:
                self.update_satellite_position(packet)
            elif packet.type == PacketType.GROUND_COMM:
                print(f"GroundNode_{self.node_id} received message from GroundNode_{packet.source_id}: {packet.payload.decode('utf-8')}")
            elif packet.type == PacketType.FILE_TRANSFER:
                await self.handle_file_transfer(packet)

        except Exception as e:
            print(f"Error handling connection in GroundNode_{self.node_id}: {e}")
        finally:
            writer.close()
            await writer.wait_closed()

    def update_satellite_position(self, packet):
        """Update satellite position based on received data."""
        try:
            satellite_id = packet.source_id
            x, y = map(float, packet.payload.decode().split(','))
            for i, (ip, port, sx, sy, node_id) in enumerate(self.satellites):
                if satellite_id == node_id:
                    self.satellites[i] = (ip, port, x, y, node_id)
                    print(f"Updated position for Satellite_{node_id}: ({x}, {y})")
                    return
            print(f"Unknown satellite position received: {satellite_id}")
        except Exception as e:
            print(f"Error updating satellite position: {e}")

    async def handle_file_transfer(self, packet):
        """Handle incoming file transfer packets."""
        try:
            
            if len(packet.payload) < 12:
                print(f"GroundNode_{self.node_id} received an invalid FILE_TRANSFER packet (too small).")
                return
            file_id, chunk_number, total_chunks = struct.unpack('!III', packet.payload[:12])
            file_data = packet.payload[12:]

            if file_id not in self.receiving_files:
                self.receiving_files[file_id] = {
                    'total_chunks': total_chunks,
                    'received_chunks': {},
                    'timestamp': time.time()
                }
                print(f"GroundNode_{self.node_id} started receiving file {file_id} with {total_chunks} chunks.")

            self.receiving_files[file_id]['received_chunks'][chunk_number] = file_data
            print(f"GroundNode_{self.node_id} received chunk {chunk_number}/{total_chunks} for file {file_id}.")

            
            if len(self.receiving_files[file_id]['received_chunks']) == total_chunks:
                
                chunks = [self.receiving_files[file_id]['received_chunks'][i] for i in range(1, total_chunks + 1)]
                file_content = b''.join(chunks)
                
                filename = f"received_file_{file_id}.bin"
                with open(filename, 'wb') as f:
                    f.write(file_content)
                print(f"GroundNode_{self.node_id} successfully reassembled and saved the file as {filename}.")
                
                del self.receiving_files[file_id]

        except Exception as e:
            print(f"Error handling file transfer in GroundNode_{self.node_id}: {e}")

    async def send_to_nearest_satellite(self, destination_id, payload, packet_type=PacketType.GROUND_COMM, max_retries=3):
        if not self.satellites:
            print(f"GroundNode_{self.node_id} has no satellites to communicate with.")
            return

       
        nearest_satellite = min(self.satellites, key=lambda sat: self.calculate_distance(sat[2], sat[3]))
        satellite_ip, satellite_port, satellite_x, satellite_y, satellite_id = nearest_satellite  # Unpack correctly

        retries = 0
        while retries < max_retries:
            try:
            
                await self.simulate_handover_delay()

                reader, writer = await asyncio.open_connection(satellite_ip, satellite_port)
                packet = Packet(
                    source_id=self.node_id,
                    destination_id=destination_id,
                    packet_type=packet_type,
                    priority=1,
                    sequence_number=0,
                    payload=payload
                )
                writer.write(packet.serialize())
                await writer.drain()
                print(f"GroundNode_{self.node_id} sent {'message' if packet_type != PacketType.FILE_TRANSFER else 'file transfer packet'} to nearest satellite (ID: {satellite_id}) at ({satellite_x:.2f}, {satellite_y:.2f})")
                writer.close()
                await writer.wait_closed()
                return
            except Exception as e:
                retries += 1
                print(f"Error communicating with Satellite_{satellite_id}: {e}. Retrying ({retries}/{max_retries})...")
                await asyncio.sleep(1)

        print(f"GroundNode_{self.node_id} failed to communicate with Satellite_{satellite_id} after {max_retries} retries.")

    async def simulate_handover_delay(self, min_delay=0.5, max_delay=2.0):
        """Simulate a delay in communication due to satellite handover."""
        delay = random.uniform(min_delay, max_delay)
        print(f"Simulating handover delay of {delay:.2f} seconds...")
        await asyncio.sleep(delay)

    def add_satellite(self, satellite_ip, satellite_port, satellite_x, satellite_y, satellite_id):
        self.satellites.append((satellite_ip, satellite_port, satellite_x, satellite_y, satellite_id))
        print(f"GroundNode_{self.node_id} added Satellite_{satellite_id} at ({satellite_x}, {satellite_y})")


    async def send_file_to_nearest_satellite(self, destination_id, file_path, chunk_size=1024, max_retries=3):
        if not self.satellites:
            print(f"GroundNode_{self.node_id} has no satellites to communicate with.")
            return

        if not os.path.isfile(file_path):
            print(f"File {file_path} does not exist.")
            return

        
        with open(file_path, 'rb') as f:
            file_data = f.read()

        
        total_size = len(file_data)
        total_chunks = (total_size + chunk_size - 1) // chunk_size
        print(f"GroundNode_{self.node_id} is sending file '{file_path}' ({total_size} bytes) in {total_chunks} chunks.")

        
        file_id = uuid4().int & (1 << 32) - 1  # 4-byte unique ID

        
        nearest_satellite = min(self.satellites, key=lambda sat: self.calculate_distance(sat[2], sat[3]))
        satellite_ip, satellite_port, satellite_x, satellite_y, satellite_id = nearest_satellite  # Unpack correctly

        for chunk_number in range(1, total_chunks + 1):
            start = (chunk_number - 1) * chunk_size
            end = start + chunk_size
            chunk_data = file_data[start:end]

           
            metadata = struct.pack('!III', file_id, chunk_number, total_chunks)
            payload = metadata + chunk_data

            packet = Packet(
                source_id=self.node_id,
                destination_id=destination_id,
                packet_type=PacketType.FILE_TRANSFER,
                priority=1,
                sequence_number=chunk_number,
                payload=payload
            )

            retries = 0
            while retries < max_retries:
                try:
                    
                    await self.simulate_handover_delay()

                    reader, writer = await asyncio.open_connection(satellite_ip, satellite_port)
                    writer.write(packet.serialize())
                    await writer.drain()
                    print(f"GroundNode_{self.node_id} sent chunk {chunk_number}/{total_chunks} to Satellite_{satellite_id}.")
                    writer.close()
                    await writer.wait_closed()
                    break
                except Exception as e:
                    retries += 1
                    print(f"Error sending chunk {chunk_number} to satellite {satellite_ip}:{satellite_port} - {e}. Retrying ({retries}/{max_retries})...")
                    await asyncio.sleep(1)
            else:
                print(f"Failed to send chunk {chunk_number} after {max_retries} retries.")


if __name__ == "__main__":
    import sys

    if len(sys.argv) < 6:
        print("Usage: <node_id> <port> <x> <y> [<satellite_ip> <satellite_port> <satellite_x> <satellite_y> <satellite_id>]...")
        sys.exit(1)

    node_id = int(sys.argv[1])
    port = int(sys.argv[2])
    x = float(sys.argv[3])
    y = float(sys.argv[4])

    ground_node = GroundNode(node_id, port, x, y)

    for i in range(5, len(sys.argv), 5):
        if i + 4 >= len(sys.argv):
            print("Insufficient arguments for satellites.")
            break
        satellite_ip = sys.argv[i]
        satellite_port = int(sys.argv[i + 1])
        satellite_x = float(sys.argv[i + 2])
        satellite_y = float(sys.argv[i + 3])
        satellite_id = int(sys.argv[i + 4])
        ground_node.add_satellite(satellite_ip, satellite_port, satellite_x, satellite_y, satellite_id)

    loop = asyncio.get_event_loop()
    try:
        loop.run_until_complete(ground_node.start_server())
    except KeyboardInterrupt:
        print(f"GroundNode_{node_id} shutting down.")
    finally:
        loop.close()
