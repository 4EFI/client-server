#include <cstddef>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>

#include "msg_info.h"

//-------------------------------------------------------------------

struct Client
{
    char name[MaxStrSize];
    int  pid;
};

const int MaxNumClients = 100;

//-------------------------------------------------------------------

void check_new_clients(Msg_t* msg, int server_id, Client* clients, int* num_clients);

int get_pid_by_name(Client* clients, int num_clients, char* name);

void send_msg(Msg_t* msg, int server_id, Client* clients, int num_clients);

void send_all_info(int server_id, Client* clients, int num_clients);

//-------------------------------------------------------------------

int main()
{
    umask(0);

    const char* file_name = "server.cpp";

    key_t key = ftok(file_name, 1); 

    int server_id = msgget(key, 0666 | IPC_CREAT);
    printf("server id = %d\n", server_id);

    Client clients[MaxNumClients] = {0};
    int num_clients = 0;

    while (1)
    {    
        Msg_t msg = {0};
        int rcv_size = msgrcv(server_id, &msg, MsgSize, SERVER_ID, IPC_NOWAIT);

        if (rcv_size <= 0) continue;
        
        check_new_clients(&msg, server_id, clients, &num_clients);
        send_msg         (&msg, server_id, clients,  num_clients);
    }
}

//-------------------------------------------------------------------

void check_new_clients(Msg_t* msg, int server_id, Client* clients, int* num_clients)
{
    if(msg->msg_type != CONNECT_TYPE) return;

    printf("\"%s\" has been connected.\n", msg->name);

    strcpy(clients[*num_clients].name, msg->name);
    clients[*num_clients].pid = msg->sender;

    msg->receiver = msg->sender;
    msg->sender   = SERVER_ID; 
    msgsnd(server_id, msg, MsgSize, 0);

    (*num_clients)++;
    
    send_all_info(server_id, clients, *num_clients);
}

//-------------------------------------------------------------------

int get_pid_by_name(Client* clients, int num_clients, char* name)
{
    for (int i = 0; i < num_clients; i++)
    {
        if (strcmp(name, clients[i].name) == 0)
        {
            return clients[i].pid;
        }
    }

    return -1;
}

//-------------------------------------------------------------------

void send_all_info(int server_id, Client* clients, int num_clients)
{
    Msg_t msg = {0};
    msg.msg_type = MSG_INFO;
    
    int size = 0;
    for(int i = 0; i < num_clients; i++)
    {
        int tmp_size = 0;
        sprintf(msg.data + size, "[%d] \"%s\" - %d%n", 
                i, 
                clients[i].name, 
                clients[i].pid, 
                &tmp_size);

        size += tmp_size;

        if(i != num_clients - 1) { sprintf(msg.data, "\n"); size++; }
    }

    printf("INFO:\n%s\n", msg.data);
    
    for(int i = 0; i < num_clients - 1; i++)
    {
        msg.receiver = clients[i].pid;
        msgsnd(server_id, &msg, MsgSize, 0);
    }
}

//-------------------------------------------------------------------

void send_msg(Msg_t* msg, int server_id, Client* clients, int num_clients)
{
    if (msg->msg_type != MSG_TYPE) return;

    int receiver = get_pid_by_name(clients, num_clients, msg->receiver_name);
    if (receiver == -1) 
    {
        printf("Client \"%s\" does not exist :(\n", msg->receiver_name);
        return;
    }
    
    msg->receiver = receiver;
    msgsnd(server_id, msg, MsgSize, 0); 
}

//-------------------------------------------------------------------