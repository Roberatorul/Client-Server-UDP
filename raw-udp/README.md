# Basic Client-Server

## Description
This project implements a simple client-server architecture over UDP. Client sends a file and server receives it and writes it into output/num_file_received.recv_filename (num_files_received is used for receiving multiple files with the same name and filename is sent by client as the first packet).

## Workflow
* server
	* creates an **UdpSocket** and binds it port 50000 (**bindToPort()** creates *struct sockaddr_in* and fills server fields accordingly before binding, see **../common/README.md**)
	* enters in an infinite loop (waits for client to send a file)
	* receives a packet from client
	* checks packet type (**Filename**, **EOF_PKT**, **DATA**) and acts accordingly
* client
	* takes server_ip and file_path from the command line (check **How to run** section)
	* fills server information(being into a LAN and knowing how the server looks, fields are hardcoded rather than using **getaddrinfo()**)
	* sends the first packet with filename
	* enters in an infinite loop (until it reaches EOF) and sends packets to server
	* when its done, it sends EOF to server so it can close the file descriptor opened to receive file from client

## How to run
First on the `server side` we need to run:
```bash
mkdir output/ # otherwise we won't have where to store received files
ifconfig # for server_ip (our device ip)
make server # or make
./server
```

Then on other computer (or other terminal) for `client`:
```bash
make client # or make
# To create a random 5MB file
# make test_file 
./client server_ip file_path
