#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <thread>
#include <ctype.h>
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

char *socket_path = "/tmp/hidden";
struct sockaddr_un addr;
char buf[100];
int fd, rc;
bool isConfigured = false;

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
            fprintf(stderr, "%u", ack.extra);
            buf[0] = ack.id;
            buf[1] = ack.action;
            buf[2] = ack.type;
            buf[3] = ack.extra;
            write(fd, buf, 4);
            fprintf(stderr, "Yay! Got action %u from: 0x%" PRIx64 " (%u) with extra: %u.\n\r", ack.action, pipes[ack.id], ack.id, ack.extra);
            return true;
        }
        else
        {
            return false;
        }
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

void prepareSocket()
{
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
}

void handleSocketMessage(int rc, char buf[])
{
    bool success = false;
    int maxtries = 5;
    int numtries = 0;
    if (!isConfigured)
    {
        prepareSocket();
        isConfigured = true;
    }

    if (rc == 2)
    {
        int dvalue = buf[0] - '0';
        int tvalue = buf[1] - '0';
        char *cvalue = "";

        while (success == false && numtries < maxtries)
        {
            success = send(dvalue, tvalue, cvalue);
            numtries++;
            usleep(10);
        }

        if (success == false)
        {
            // needed for when we get no response from duino
            fprintf(stderr, "%d", 0);
            buf[0] = dvalue; // id
            buf[1] = tvalue; // action
            buf[2] = 0;      // type
            buf[3] = 0;      // extra
            write(fd, buf, 4);
        }
    }
    else
    {
        // ignore it
    }
}

void listenOnUnixSocket()
{
    struct sockaddr_un addr;
    char buf[100];
    int fd, cl, rc;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("socket error");
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (*socket_path == '\0')
    {
        *addr.sun_path = '\0';
        strncpy(addr.sun_path + 1, socket_path + 1, sizeof(addr.sun_path) - 2);
    }
    else
    {
        strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
        unlink(socket_path);
    }

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind error");
        exit(-1);
    }

    if (listen(fd, 5) == -1)
    {
        perror("listen error");
        exit(-1);
    }

    while (1)
    {
        if ((cl = accept(fd, NULL, NULL)) == -1)
        {
            perror("accept error");
            continue;
        }

        while ((rc = read(cl, buf, sizeof(buf))) > 0)
        {
            printf("read %u bytes: %.*s\n", rc, rc, buf);
            handleSocketMessage(rc, buf);
        }
        if (rc == -1)
        {
            perror("read");
            exit(-1);
        }
        else if (rc == 0)
        {
            printf("EOF\n");
            close(cl);
        }
    }
}

int main(int argc, char *argv[])
{
    setup();

    std::thread t1(listenOnUnixSocket);

    t1.join();
    return 0;
}
