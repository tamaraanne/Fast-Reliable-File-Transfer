# Fast-Reliable-File-Transfer

User level application to reliably transfer a 1GB file on a high latency lossy link using UDP protocol with NACK mechanism.

Multi-threading is used to implement the NACK mechanism to work parallely with the file transfer. The 1GB file is broken into packets of 1464 bytes. The packet is implemented as a structure that contains a buffer of 1464 bytes for the data, a flag bit (1 for last packet or else 0) and a sequence number. An array is created to keep a track of the packets received by order of their sequence number. The server sends a NACK for a packet that has not been received or if it is ready to receive the next packet. The client receives the NACKs and then sends the requested packet. When all the packets have been successfully received, the server sends a NACK of -1 to signal the the file has been received.

Follow the steps below to run the application:
1. Create a topology containing two nodes connected by a Duplex link using the basic.ns file provided.
2. Use the ssh command to connect to the two nodes via separate terminal windows.
3. Create a 1GB file on the client node and adjust the latency on the link to 200 ms with 20% loss.
4. Run the cliet_nack.c and server_nack.c files on the client and server node respectively.

Throughput on a 100 Mbps link with 200 ms delay and 20% loss = 35.79 Mbps
Transfer time : 240 seconds
