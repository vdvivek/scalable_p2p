import asyncio
from ground_node import GroundNode

# Written by Hritika Rahul Mehta and Ziqi Zheng
async def send_file():
    
    ground_node_sender = GroundNode(1, 35001, 10.0, 20.0)

    ground_node_sender.add_satellite("127.0.0.1", 36001, 25.0, 30.0, 101) 

    asyncio.create_task(ground_node_sender.start_server())

    await asyncio.sleep(1)

    file_path = "Screenshot_2024.jpg"  

    await ground_node_sender.send_file_to_nearest_satellite(3, file_path)

async def receive_file():
    
    ground_node_receiver = GroundNode(3, 35003, 50.0, 60.0)

    ground_node_receiver.add_satellite("127.0.0.1", 36002, 45.0, 50.0, 102)  

    await ground_node_receiver.start_server()


async def main():
    await asyncio.gather(
        send_file(),
        receive_file()
    )

if __name__ == "__main__":
    asyncio.run(main())
