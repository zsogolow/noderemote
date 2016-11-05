#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "noderemote.h"

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10
RF24 radio(9, 10);

#define ID 1

int duty = GENERAL_DUINO;
int blinkPin = 8;
int relayPin = 7;

boolean timePassed;
unsigned long time;
long heartbeatInterval = 30000;


void initSelf()
{
    switch (duty)
    {
    case TEMP_DUINO:
        analogReference(INTERNAL);
        break;
    case RELAY_DUINO:
    case GENERAL_DUINO:
    case MOTION_DUINO:
    default:
        break;
    }

    
    Packet packet;
    packet.id = ID;
    packet.action = HEARTBEAT;
    packet.extra = 0;
    packet.type = duty;
    sendCallback(packet);
}

void setup(void)
{
    Serial.begin(9600);
    printf_begin();
    printf("\nRemote Switch Arduino\n\r");

    pinMode(blinkPin, OUTPUT);
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);

    time = millis();

    // Setup and configure rf radio
    radio.begin();
    radio.setRetries(15, 15);

    radio.openWritingPipe(pipes[ID]);
    radio.openReadingPipe(1, pipes[ID]);

    radio.startListening();
    radio.printDetails();

    initSelf();
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

void blink(int pin, long duration)
{
    digitalWrite(pin, HIGH); // turn the LED on (HIGH is the voltage level)
    delay(duration);         // wait for a second
    digitalWrite(pin, LOW);  // turn the LED off by making the voltage LOW
}

int getRelayState()
{
    boolean state = digitalRead(relayPin);
    return state == true ? 1 : 0;
}

void switchRelay(int state)
{
    if (state == 0)
    {
        // off
        digitalWrite(relayPin, LOW);
    }
    else if (state == 1)
    {
        // on
        digitalWrite(relayPin, HIGH);
    }
}

Packet handleAction(Packet packet)
{
    Packet handled;
    handled.id = packet.id;
    handled.action = packet.action;
    handled.type = duty;
    
    switch (packet.action)
    {
    case BLINK:
        blink(blinkPin, 1000);
        break;
    case RELAY_STATE:
        handled.extra = getRelayState();
        break;
    case RELAY_ON:
        // turn on relay
        switchRelay(1);
        handled.extra = getRelayState();
        break;
    case RELAY_OFF:
        // turn off relay
        switchRelay(0);
        handled.extra = getRelayState();
        break;
    default:
        handled.extra = 0;
        break;
    }

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
        packet.extra = 0;
        packet.type = duty;
        blink(blinkPin, 10);
        sendCallback(packet);
        time = millis();
        timePassed = false;
    }
    else
    {
        if (time + heartbeatInterval < now)
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