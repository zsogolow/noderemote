#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "noderemote.h"

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10
RF24 radio(9, 10);

#define ID 1
int blinkPin = 8;

boolean timePassed;
unsigned long time;

void setup(void)
{
    Serial.begin(9600);
    printf_begin();
    printf("\nRemote Switch Arduino\n\r");

    pinMode(blinkPin, OUTPUT);

    time = millis();

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

Packet handleAction(Packet packet)
{
    Packet handled;

    switch (packet.action)
    {
    case BLINK:
        digitalWrite(blinkPin, HIGH); // turn the LED on (HIGH is the voltage level)
        delay(1000);                  // wait for a second
        digitalWrite(blinkPin, LOW);  // turn the LED off by making the voltage LOW
        break;
    default:
        break;
    }

    handled.id = packet.id;
    handled.action = packet.action;
    return handled;
}

void loop(void)
{
    unsigned long now = millis();
    if (timePassed)
    {
        Packet packet;
        packet.id = ID;
        packet.action = HEARTBEAT;
        sendCallback(packet);
        time = millis();
        timePassed = false;
    }
    else
    {
        if (time + 10000 < now)
        {
            timePassed = true;
        }
    }

    // if there is data ready
    if (radio.available())
    {
        // Dump the payloads until we've gotten everything
        unsigned short message;

        while (radio.available())
        {
            // Fetch the payload, and see if this was the last one.
            Packet packet;
            radio.read(&packet, sizeof(packet));

            // Spew it
            Serial.println(packet.action);

            Packet cb;

            cb = handleAction(packet);

            sendCallback(cb);

            delay(10);
        }
    }
}