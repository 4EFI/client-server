#include <stdio.h>
#include <stdlib.h>
#include <cstddef>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>

#include "msg_info.h"

//-------------------------------------------------------------------

void connect_to_server(Msg_t *msg_connect, int server_id, uid_t client_pid);

//-------------------------------------------------------------------

int main()
{
    umask(0);

    const char* file_name = "server.cpp";

    key_t key = ftok(file_name, 1); 

    int server_id = msgget(key, 0666 | IPC_CREAT);
    printf("server id = %d\n", server_id);

    uid_t client_pid = getpid();

    Msg_t msg_connect = {0};
    connect_to_server(&msg_connect, server_id, client_pid);

    // Start sending/receiving massages 
    while (1) 
    {
        // Check new massages
        Msg_t msg_rcv = {0};
        int rcv_size = msgrcv(server_id, &msg_rcv, MsgSize, client_pid, IPC_NOWAIT);

        if (rcv_size > 0)
        {
            if /**/ (msg_rcv.msg_type == MSG_TYPE)
            {
                printf("%s: %s\n", msg_rcv.name, msg_rcv.data);
            }
            else if (msg_rcv.msg_type == INFO_TYPE)
            {
                printf("----------SERVER INFO----------\n");
                printf("%s\n", msg_rcv.data);
                printf("-------------------------------\n");
            }

            continue; // for checking new messages
        }

        clear_buffer();

        int mode = -1;
        printf("Choose mode:\n\t[s] - send massage\n\t[c] - check massage\t\n\t[q] - quit\n");
        mode = getchar();

        Msg_t msg_snd = {0};
        msg_snd.receiver = SERVER_ID;
        strcpy(msg_snd.name, msg_connect.name);

        char text[MaxStrSize] = "";

        bool is_break = false;
        bool is_continue = false;
        switch(mode)
        {
            case 's': 
                printf("Enter \"client's name: massage\"\n");
                scanf(" %[^\n]", text);

                sscanf(
                    text,
                    " %[^:]%*c %[^\n]%*c", 
                    msg_snd.receiver_name, 
                    msg_snd.data // Read before '\n', and %*c reads this '\n' symbol and skip it
                    );

                msg_snd.msg_type = MSG_TYPE;

                msgsnd(server_id, &msg_snd, MsgSize, 0);

                break;
            
            case 'q':
                is_break = true;
                break;

            case 'c':
                is_continue = true;
                break;
            
            default:
                printf ("Incorrect mode, try again...\n");
                is_continue = true;
        }

        if (is_break) 
        {
            Msg_t msg_leave = {0};
            msg_leave.msg_type = LEAVE_TYPE;
            msg_leave.receiver = SERVER_ID;
            msg_leave.sender   = client_pid;
            msgsnd(server_id, &msg_leave, MsgSize, 0);

            break;
        }
        if (is_continue) continue;
    }
}

//-------------------------------------------------------------------

void connect_to_server(Msg_t *msg_connect, int server_id, uid_t client_pid)
{
    msg_connect->receiver = SERVER_ID;  
    msg_connect->sender   = client_pid;
    msg_connect->msg_type = CONNECT_TYPE;
  
    printf("Enter your name...\n");
    scanf("%s", msg_connect->name); 

    msgsnd(server_id, msg_connect, MsgSize, 0);
    msgrcv(server_id, msg_connect, MsgSize, client_pid, 0);
    
    printf("You successfully connected, your PID is %d!\n", client_pid);
}

//-------------------------------------------------------------------
