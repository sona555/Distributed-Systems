/***
*The Program parallel writes and reads from the DHT.
*Creates and saves key / value pairs in an OpenDHT. The Program then puts these data together again with construction. 
*Values in OpenDHT can only be of a limited size, the data therefore is broken down into small chunks. 
*For each chunk is then the SHA1 fingerprint to be calculated, which is then used as a key and stored in the DHT. 
*The SHA1 fingerprint will be saved in an ".odht" file. 
*The original files can be restored by reading the individual fingerprints.
* Please compile it with g++ Milestone2.cpp -o Milestone -lopendht -lgnutls -lpthread -lnettle -largon2 -lcrypto command.
*/

#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <string>
#include <opendht.h>
#include <openssl/sha.h>
#include <vector>
#include <memory>
#include <fstream>


using namespace std;

dht::DhtRunner node;

std::string host;
int port;
#define CHUNKSIZE 4096

static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

std::string enc;

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';

    }

    return ret;
}

void* runner(void* threadid) {
   
    node.run(port, dht::crypto::generateIdentity(), true);
    node.bootstrap(host, std::to_string(port));
    return NULL;
}

int main(int argc, char** argv) {



    std::string eingabe_host, eingabe_port = "";

    cout << "Host: ";

    cin >> eingabe_host;

    cout << "\nPort: ";

    cin >> eingabe_port;

    if (eingabe_host.empty()) {
        host = "bootstrap.ring.cx";
    }
    else {
        host = eingabe_host;
    }

    if (eingabe_port.empty()) {
        port = 4222;
    }
    else {
        port = std::stoi(eingabe_port);
    }

    cout << "host: " << host << std::endl;
    cout << "port: " << port << std::endl;

    pthread_t threads;
    int rc;
    cout << "Enter 'store' 'restore' or 'quit' " << endl;
    rc = pthread_create(&threads, NULL, &runner, NULL);

    if (rc)
    {
        cout << "Error:unable to create thread," << rc << endl;
        exit(-1);
    }

    std::string eingabe = "";
    std::string put_key, put_value = "";
    std::string get_key = "";



    for (;;) {

        cin >> eingabe;

        if (eingabe == "store")
        {
            cout << "\nEnter the input file " << endl;

            cin >> put_value;

            cout << "Enter the output file " << endl;

            cin >> put_key;

            // (filename, als binary oeffnen);
            std::ifstream ifs(put_value, std::ifstream::binary);
            if (!ifs.is_open()) {
                std::cerr << "usage: " << put_value << " ERROR: Cannot open file: " << " --- EXIT" << std::endl;

            }

            std::ofstream ofs(put_key, std::ofstream::binary);
            if (!ofs.is_open()) {
                std::cerr << "usage: " << put_key << " ERROR: Cannot open file: " << " --- EXIT" << std::endl;
            }
            char* buffer = new char[CHUNKSIZE];

            unsigned char hash[SHA_DIGEST_LENGTH];

            int length = 0;

            while (ifs.good())
            {
                ifs.read(buffer, CHUNKSIZE);
                length = ifs.gcount();

                //hash
                SHA_CTX shactx;
                SHA1_Init(&shactx);
                SHA1_Update(&shactx, buffer, length);
                SHA1_Final(hash, &shactx);

                //write to the file
                ofs.write((char*)&length, sizeof(int));
                ofs.write((char*)hash, SHA_DIGEST_LENGTH); //write will imer char hash


                //store in DHT
                enc = base64_encode(hash, SHA_DIGEST_LENGTH);
                node.put(dht::InfoHash::get(enc.c_str()), buffer);

                std::cout << "Hash: " << enc << std::endl;

                if (length != CHUNKSIZE) {    // weniger bytes gelesen als ich wollte,
                                              // indikator für dateiende
                                              // dann ende erreicht
                    break;
                }

            }
            ofs.close();
            ifs.close();

            std::cout << "Okay, file stored as " << put_key << endl;

            put_key.clear();
            put_value.clear();
            eingabe.clear();

            continue;
        }

        if (eingabe == "restore")
        {
            string input, output;
            cout << "Please give the path of INPUT file" << endl;
            cin >> input;
            cout << "Please give the path of OUTPUT file" << endl;
            cin >> output;

            std::ifstream ifs(input, std::ifstream::binary);
            if (!ifs.is_open()) {
                std::cerr << " ERROR: Cannot open file: " << input << std::endl;

            }

            std::ofstream ofs(output, std::ofstream::binary);
            if (!ofs.is_open()) {
                std::cerr << " ERROR: Cannot open file: " << output << std::endl;
            }


            char* buffer = new char[CHUNKSIZE];
            unsigned char hash[SHA_DIGEST_LENGTH];
            int length;
            string wert;
            string prevHash = "";

            while (ifs.good())
            {

                ifs.read((char*)&length, sizeof(int));

                ifs.read((char*)hash, SHA_DIGEST_LENGTH);

                if (enc.compare(prevHash) == 0) break;
                prevHash = enc;

                //get the Value from DHT
                node.get(enc.c_str(), [&](const std::vector<std::shared_ptr<dht::Value>>& values)
                    {
                        for (const auto& value : values)
                            wert = value->unpack<string>();

                        return true;
                    });

                this_thread::sleep_for(5s);

                ofs << wert;

            }
            ifs.close();
            ofs.close();
            cout << "The file has been restored as" << output << "----" << endl;

            eingabe.clear();
            continue;
        }

        if (eingabe == "quit")
        {
            std::cout << "Quitting now" << std::endl;
            break;
        }

        eingabe.clear();
    }

    node.join();
    pthread_exit(NULL);
    return 0;
}