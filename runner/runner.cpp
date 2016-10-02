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
#include <sys/socket.h>
#include <sys/un.h>

using namespace std;

RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

void setup(void)
{
    //Prepare the radio module
    fprintf(stderr, "\nPreparing interface\n");
    radio.begin();
    radio.setRetries(15, 15);

    radio.openReadingPipe(0, pipes[0]);
    radio.openReadingPipe(1, pipes[1]);
    radio.openReadingPipe(2, pipes[2]);
    radio.openReadingPipe(3, pipes[3]);
    radio.openReadingPipe(4, pipes[4]);
    radio.openReadingPipe(5, pipes[5]);

    radio.startListening();

    // radio.printDetails();
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
        fprintf(stderr, "Oh gosh, it's not giving me any response...\n\r");
        return false;
    }
    else
    {
        //If we received the message in time, let's read it and print it
        radio.read(&ack, sizeof(ack));
        if (ack.action == action)
        {
            printf("%u", ack.extra);
            fprintf(stderr, "Yay! Got action %u from: 0x%" PRIx64 " (%u) with extra: %u.\n\r", ack.action, pipes[ack.id], ack.id, ack.extra);
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
    printf("%u", 0);

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
        printf("%u", heard.extra);
        fprintf(stderr, "Yay! Got action %u from: 0x%" PRIx64 " (%u) with extra: %u.\n\r", heard.action, pipes[heard.id], heard.id, heard.extra);
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
        fprintf(stderr, "failed...\n\r");
    else
        fprintf(stderr, "ok!\n\r");

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

char *socket_path = "/tmp/hidden";
void loop()
{
    struct sockaddr_un addr;
    char buf[100];
    int fd, rc;
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("socket error");
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("connect error");
        exit(-1);
    }

    while (true)
    {
        Packet heard;
        heard = listenForPackets();
        buf[0] = heard.id;
        if (heard.id > 0)
        {
            write(fd, buf, 1);
        }
    }
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

    if (tvalue == PING || tvalue == BLINK || tvalue == RELAY_STATE || tvalue == RELAY_ON || tvalue == RELAY_OFF)
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
        loop();
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