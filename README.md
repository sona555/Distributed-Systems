# Distributed-Systems
The Program parallel writes and reads from the DHT.
Creates and saves key / value pairs in an OpenDHT. The Program then puts these data together again with construction. 
Values in OpenDHT can only be of a limited size, the data therefore is broken down into small chunks. 
For each chunk is then the SHA1 fingerprint to be calculated, which is then used as a key and stored in the DHT. 
The SHA1 fingerprint will be saved in an ".odht" file. 
The original files can be restored by reading the individual fingerprints.
Please compile it with g++ Milestone2.cpp -o Milestone -lopendht -lgnutls -lpthread -lnettle -largon2 -lcrypto command.
