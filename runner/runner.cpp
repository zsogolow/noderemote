#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <RF24/RF24.h>
#include <inttypes.h>
#include <string>
#include <noderemote.h>

using namespace std;

RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

void setup(void)
{
    //Prepare the radio module
    printf("\nPreparing interface\n");
    radio.begin();
    radio.setRetries(15, 15);

    radio.openReadingPipe(0, pipes[0]);
    radio.openReadingPipe(1, pipes[1]);
    radio.openReadingPipe(2, pipes[2]);
    radio.openReadingPipe(3, pipes[3]);
    radio.openReadingPipe(4, pipes[4]);
    radio.openReadingPipe(5, pipes[5]);

    radio.startListening();

    radio.printDetails();
}

Packet ack; // issue with radio library, hack to workaround segfault
bool listenForACK(int action)
{
    //Listen for ACK
    radio.startListening();
    //Let's take the time while we listen
    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while (!radio.available() && !timeout)
    {
        //printf("%d", !radio.available());
        if (millis() - started_waiting_at > 1000)
        {
            timeout = true;
        }
    }

    if (timeout)
    {
        //If we waited too long the transmission failed
        printf("Oh gosh, it's not giving me any response...\n\r");
        return false;
    }
    else
    {
        //If we received the message in time, let's read it and print it
        radio.read(&ack, sizeof(ack));
        if (ack.action == action)
        {
            printf("Yay! Got action %u from: 0x%" PRIx64 " (%u).\n\r", ack.action, pipes[ack.id], ack.id);
            return true;
        }
        else
        {
            return false;
        }
    }
}

Packet heard;
Packet listenForPackets()
{
    //Listen for ACK
    radio.startListening();
    //Let's take the time while we listen
    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while (!radio.available() && !timeout)
    {
        //printf("%d", !radio.available());
        if (millis() - started_waiting_at > 1000)
        {
            timeout = true;
        }
    }

    if (timeout)
    {
        Packet empty;
        empty.id = -1;
        empty.action = EMPTY;
        return empty;
    }
    else
    {
        //If we received the message in time, let's read it and print it
        radio.read(&heard, sizeof(heard));
        printf("Yay! Got action %u from: 0x%" PRIx64 " (%u).\n\r", heard.action, pipes[heard.id], heard.id);
        return heard;
    }
}

bool sendAction(int id, int action)
{
    Packet packet;
    packet.id = id;
    packet.action = action;

    bool ok = radio.write(&packet, sizeof(packet));
    if (!ok)
        printf("failed...\n\r");
    else
        printf("ok!\n\r");

    return listenForACK(action);
}

bool send(int id, int action, char *msg)
{
    //Returns true if ACK package is received
    //Stop listening
    radio.stopListening();

    radio.openWritingPipe(pipes[id]);

    return sendAction(id, action);
}

void loop()
{
    listenForPackets();
}

int main(int argc, char **argv)
{
    int dflag = 0;
    int tflag = 0;
    char *cvalue = NULL;
    int dvalue = 0;
    int tvalue = 0;
    int index;
    int c;

    opterr = 0;
    while ((c = getopt(argc, argv, "d:t:c:")) != -1)
    {
        switch (c)
        {
        case 'd':
            dflag = 1;
            dvalue = atoi(optarg);
            break;

        case 't':
            tflag = 1;
            tvalue = atoi(optarg);
            break;

        case 'c':
            cvalue = optarg;
            break;

        case '?':
            if (optopt == 'c')
            {
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            }
            else if (isprint(optopt))
            {
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            }
            else
            {
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            }
            return 1;

        default:
            abort();
        }
    }

    setup();

    bool success = false;
    int maxtries = 5;
    int numtries = 0;

    if (tvalue == PING || tvalue == BLINK)
    {
        while (success == false && numtries < maxtries)
        {
            success = send(dvalue, tvalue, cvalue);
            numtries++;
            usleep(10);
        }

        if (success == true)
            return 0;
        else
            return 1;
    }
    else if (tvalue == HEARTBEAT)
    {
        while (true)
        {
            loop();
        }

        return 0;
    }

    // printf("dflag = %d, tflag = %d, cvalue = %s\n", dflag, tflag, cvalue);
    // printf("dvalue = %d, tvalue = %d, cvalue = %s\n", dvalue, tvalue, cvalue);

    // for (index = optind; index < argc; index++)
    // {
    //     printf("Non-option argument %s\n", argv[index]);
    // }

    // return 0;
}