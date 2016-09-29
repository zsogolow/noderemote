#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "noderemote.h"

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10
RF24 radio(9, 10);

#define ID 1
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

Packet handlePing(Packet packet)
{
    Packet pong;
    pong.id = packet.id;
    pong.action = packet.action;
    return pong;
}

Packet handleAction(Packet packet)
{
    Packet handled;
    handled.id = packet.id;
    handled.action = packet.action;
    return handled;
}

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
            Packet packet;
            radio.read(&packet, sizeof(packet));

            // Spew it
            Serial.println(packet.action);

            Packet cb;

            switch (packet.action)
            {
            case PING:
                cb = handlePing(packet);
                break;
            default:
                cb = handleAction(packet);
                break;
            }

            sendCallback(cb);

            delay(10);
        }
    }
}