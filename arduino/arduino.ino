#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

int ID = 1;
// int ID = 2;
// ...
// int ID = 5;

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10
RF24 radio(9, 10);

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[6] = {
    0x7878787878LL,
    0xB3B4B5B6F1LL,
    0xB3B4B5B6CDLL,
    0xB3B4B5B6A3LL,
    0xB3B4B5B60FLL,
    0xB3B4B5B605LL,
};


typedef struct {
    int id;
    int action;
    char *msg;
}
Packet;

#define PING 1
#define MSG 2

char *convertNumberIntoArray(unsigned short number, unsigned short length)
{

    char *arr = (char *)malloc(length * sizeof(char)), *curr = arr;
    do
    {
        *curr++ = number % 10;
        number /= 10;
    } while (number != 0);
    return arr;
}

unsigned short getId(char *rawMessage, unsigned short length)
{
    unsigned short i = 0;
    unsigned short id = 0;
    for (i = 1; i < length; i++)
    {
        id += rawMessage[i] * pow(10, i - 1);
    }
    return id;
}

unsigned short getMessage(char *rawMessage)
{
    unsigned short message = rawMessage[0];
    return (unsigned short)message;
}
unsigned short getLength(unsigned int rudeMessage)
{
    unsigned short length = (unsigned short)(log10((float)rudeMessage)) + 1;
    return length;
}

void setup(void)
{
    Serial.begin(9600);
    printf_begin();
    printf("\nRemote Switch Arduino\n\r");

    // Setup and configure rf radio
    radio.begin();
    radio.setRetries(15, 15);

    radio.openWritingPipe(pipes[ID]);
    radio.openReadingPipe(1, pipes[ID]);

    radio.startListening();
    radio.printDetails();
}

void sendCallback(Packet callback)
{
    // First, stop listening so we can talk
    radio.stopListening();

    // Send the final one back.
    radio.write(&callback, sizeof(callback));
    printf("Sent response.\n\r");

    // Now, resume listening so we catch the next packets.
    radio.startListening();
}

Packet packet;

void loop(void)
{
    // if there is data ready
    if (radio.available())
    {
        // Dump the payloads until we've gotten everything
        unsigned short message;
        bool done;

        done = false;
        while (radio.available())
        {
            // Fetch the payload, and see if this was the last one.
            radio.read(&packet, sizeof(packet));

            // Spew it
            printf("Got message %d...", packet.msg);

            Packet cb;

            switch (packet.action)
            {
            case PING:
                cb.id = packet.id;
                cb.action = packet.action;
                cb.msg = "pong";
                break;
            case MSG:
                cb.id = packet.id;
                cb.action = packet.action;
                cb.msg = packet.msg;
                break;
            }

            sendCallback(cb);

            delay(10);
        }
    }
}