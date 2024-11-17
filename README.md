# Nexus - A Scalable P2P

## Libraries needed - make sure to install them beforehand.
1. jsoncpp
2. curl

## Build.
```bash
make all
```

## Run.
1. Run the Nexus Registry Server first. By default, it runs on port 5001. Server is configured to accept HTTP requests on 127.0.0.1:5001 for these actions : ["register", "deregister", "list"].
```bash
$ ./registry_server
[INFO] NexusRegistryServer is running on port 5001
```
2. To test the server is configured properly, use curl on command-line.
```bash
$ curl -X POST -H "Content-Type: application/json" -d '{"action":"list"}' http://127.0.0.1:5001 | jq     
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100    21  100     4  100    17   3018  12830 --:--:-- --:--:-- --:--:-- 21000
null
```
3. Run multiple nodes using following command.
On Terminal 1:
```bash
$ ./nexus -node ground -name g1 -ip 127.0.0.1 -port 5001 -x 1 -y 0 -z 2
```
On Terminal 2:
```bash
$ ./nexus -node ground -name g2 -ip 127.0.0.1 -port 5006 -x 6 -y 12 -z 2
```
On Terminal 3:
```bash
$ ./nexus -node ground -name sat1 -ip 127.0.0.1 -port 5004 -x 6 -y 12 -z 2
```
4. A prompt should appear for each of the nexus process with something like this. Now messages can be sent across the nodes.
```bash
$ ./nexus -node ground -name sat1 -ip 127.0.0.1 -port 5004 -x 6 -y 12 -z 2                          
[INFO] Creating a Node...
[DEBUG] Registry server is present at http://127.0.0.1:5001
{"message": "Node registered successfully"}[NEXUS] Node sat1 successfully bound to 127.0.0.1:5004
[NEXUS] sat1 is ready for UDP communication at 127.0.0.1:5004
[NEXUS] Node is running. Press Ctrl+C OR q to terminate.
sat1 prompt: send
Enter target node name: g2
Enter target IP: 127.0.0.1
Enter target port: 5006
Enter message: hello i need help
[NEXUS] Sent message to g2 at 127.0.0.1:5006
sat1 prompt: list
[INFO] Added node: g2 (0d7a6826-a403-4d91-8daf-10280d26d4db) at 127.0.0.1:5006 to the network.
[INFO] Added node: g1 (1171c750-f82a-4c1f-8738-ea85b0aec0b5) at 127.0.0.1:5001 to the network.
[INFO] Added node: sat1 (96fc5f42-a899-49d9-8a68-a933bbb6293d) at 127.0.0.1:5004 to the network.
[INFO] Current nodes in the network:
g2 (0d7a6826-a403-4d91-8daf-10280d26d4db) at 127.0.0.1:5006 [0, 0, 0]
g1 (1171c750-f82a-4c1f-8738-ea85b0aec0b5) at 127.0.0.1:5001 [0, 0, 0]
sat1 (96fc5f42-a899-49d9-8a68-a933bbb6293d) at 127.0.0.1:5004 [0, 0, 0]
sat1 prompt:
```
```bash
(base) prkaaviya@MacKayBook ~/CLionProjects/scalable_p2p (prk_feature_branch+*?) $ ./nexus -node ground -name g2 -ip 127.0.0.1 -port 5006 -x 6 -y 12 -z 2
[INFO] Creating a Node...
[DEBUG] Registry server is present at http://127.0.0.1:5001
{"message": "Node registered successfully"}[NEXUS] Node g2 successfully bound to 127.0.0.1:5006
[NEXUS] g2 is ready for UDP communication at 127.0.0.1:5006
[NEXUS] Node is running. Press Ctrl+C OR q to terminate.
g2 prompt: [NEXUS] Received message from 127.0.0.1:5004 - "hello i need help"
[MESSAGE RECEIVED]
g2 prompt: send
Enter target node name: g1
Enter target IP: 127.0.0.1
Enter target port: 5001
Enter message: what do you need?
[NEXUS] Sent message to g1 at 127.0.0.1:5001
g2 prompt:
```
```bash
$ ./nexus -node ground -name g1 -ip 127.0.0.1 -port 5001 -x 1 -y 0 -z 2 
[INFO] Creating a Node...
[DEBUG] Registry server is present at http://127.0.0.1:5001
{"message": "Node registered successfully"}[NEXUS] Node g1 successfully bound to 127.0.0.1:5001
[NEXUS] g1 is ready for UDP communication at 127.0.0.1:5001
[NEXUS] Node is running. Press Ctrl+C OR q to terminate.
g1 prompt: [NEXUS] Received message from 127.0.0.1:5006 - "what do you need?"
[MESSAGE RECEIVED]
g1 prompt:
```
