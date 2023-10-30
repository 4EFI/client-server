#ifndef MSG_INFO_H
#define MSG_INFO_H

#include <stdio.h>

//-------------------------------------------------------------------

#define MaxStrSize 1000
#define MaxNameSize 20
#define SERVER_ID 1

struct Msg_t
{
    long receiver;
    long sender;

    char receiver_name[MaxNameSize];

    long msg_type;

    char name[MaxNameSize];
    char data[MaxStrSize];
};

const int MsgSize = sizeof(Msg_t) - sizeof(long);

enum MsgType
{
    CONNECT_TYPE = 1,
    LEAVE_TYPE   = 2,

    MSG_TYPE     = 3,
    INFO_TYPE    = 4
};

//-------------------------------------------------------------------

void clear_buffer()
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {;}
}

//-------------------------------------------------------------------

#endif