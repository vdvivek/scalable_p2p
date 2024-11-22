# Nexus - A Scalable P2P

## Contributors
1. Vivek Denny
2. Kaaviya Ramkumar
3. Hritika Mehta
4. Ziqi Zheng
   
## Libraries needed - make sure to install them beforehand.
1. jsoncpp
```bash
cd ~
git clone https://github.com/open-source-parsers/jsoncpp.git
cd jsoncpp
mkdir build
cd build/
cmake .. -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/local
make -j$(nproc)
make install
ls ~/local/include/
export PKG_CONFIG_PATH=~/local/lib/pkgconfig:$PKG_CONFIG_PATH
export PATH=~/local/bin:$PATH
export LD_LIBRARY_PATH=~/local/lib:$LD_LIBRARY_PATH
```
3. curl
```bash
wget https://curl.se/download/curl-8.9.1.tar.gz
tar -xvzf curl-8.9.1.tar.gz
cd curl-8.9.1/
./configure --prefix=$HOME/local --disable-shared --without-ssl
make -j$(nproc)
make install
ls $HOME/local/include/curl/curl.h
export LD_LIBRARY_PATH=$HOME/local/lib:$LD_LIBRARY_PATH
```

## Build.
```bash
make clean; make code;
```
## Run in one go.
```bash
bash runme.sh
```

## Run individually.
1. Run the Nexus Registry Server first. By default, it runs on port 5001. Server is configured to accept HTTP requests on 127.0.0.1:5001 for these actions : ["register", "deregister", "list"].
```bash
$ ./registry_server 127.0.0.1 5001
[INFO] NexusRegistryServer is running on port 127.0.0.1:5001
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
$ ./nexus -node ground -name g1 -ip 127.0.0.1 -port 5001 -x 1 -y 0
```
On Terminal 2:
```bash
$ ./nexus -node ground -name g2 -ip 127.0.0.1 -port 5006 -x 6 -y 12
```
On Terminal 3:
```bash
$ ./nexus -node ground -name sat1 -ip 127.0.0.1 -port 5004 -x 6 -y 12
```
A prompt should appear for each of the nexus process with something like this. Now messages can be sent across the nodes.
