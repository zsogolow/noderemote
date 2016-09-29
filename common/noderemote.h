#include <inttypes.h>
#include <stdlib.h>

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[6] = {
    0x7878787878LL,
    0xB3B4B5B6F1LL,
    0xB3B4B5B6CDLL,
    0xB3B4B5B6A3LL,
    0xB3B4B5B60FLL,
    0xB3B4B5B605LL,
};

struct Packet
{
    uint8_t id;
    uint8_t action;
};

#define EMPTY -1
#define PING 1
#define HEARTBEAT 2