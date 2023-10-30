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
    bool is_available;
};

const int MaxNumClients = 100;

//-------------------------------------------------------------------

void check_new_clients  (Msg_t* msg, int server_id, Client* clients, int* num_clients);
void check_leave_clients(Msg_t* msg, int server_id, Client* clients, int* num_clients);

int get_pid_by_name(Client* clients, char* name);
int get_id_by_pid  (Client* clients, int   pid);

void send_msg(Msg_t* msg, int server_id, Client* clients, int num_clients);

void send_all_info(int server_id, Client* clients, int num_clients, int type, int id);

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
        
        check_new_clients  (&msg, server_id, clients, &num_clients);
        check_leave_clients(&msg, server_id, clients, &num_clients);
        send_msg           (&msg, server_id, clients,  num_clients);
    }
}

//-------------------------------------------------------------------

void check_new_clients(Msg_t* msg, int server_id, Client* clients, int* num_clients)
{
    if (msg->msg_type != CONNECT_TYPE) return;

    printf("\"%s\" has been connected.\n", msg->name);

    strcpy(clients[*num_clients].name, msg->name);
    clients[*num_clients].pid          = msg->sender;
    clients[*num_clients].is_available = true;

    msg->receiver = msg->sender;
    msg->sender   = SERVER_ID; 
    msgsnd(server_id, msg, MsgSize, 0);

    (*num_clients)++;

    send_all_info(server_id, clients, *num_clients, CONNECT_TYPE, get_id_by_pid(clients, clients[*num_clients - 1].pid));
}

//-------------------------------------------------------------------

void check_leave_clients(Msg_t* msg, int server_id, Client* clients, int* num_clients)
{
    if (msg->msg_type != LEAVE_TYPE) return;

    int client_id = get_id_by_pid(clients, msg->sender);
    clients[client_id].is_available = false;

    (*num_clients)--;

    send_all_info(server_id, clients, *num_clients, LEAVE_TYPE, client_id);
}

//-------------------------------------------------------------------

int get_pid_by_name(Client* clients, char* name)
{
    for (int i = 0; i < MaxNumClients; i++)
    {
        if(!clients[i].is_available) continue;

        if (strcmp(name, clients[i].name) == 0)
        {
            return clients[i].pid;
        }
    }

    return -1;
}

//-------------------------------------------------------------------

int get_id_by_pid(Client* clients, int pid)
{
    for (int i = 0; i < MaxNumClients; i++)
    {
        if(!clients[i].is_available) continue;
        
        if (pid == clients[i].pid)
        {
            return i;
        }
    }

    return -1;
}

//-------------------------------------------------------------------

void send_all_info(int server_id, Client* clients, int num_clients, int type, int id)
{
    Msg_t msg = {0};
    msg.msg_type = INFO_TYPE;
    msg.sender   = SERVER_ID;
    
    int size = 0;
    if /**/ (type == CONNECT_TYPE)
    {
        sprintf(msg.data, "Client \"%s\" connected chat!\n%n", clients[id].name, &size);
    }
    else if (type == LEAVE_TYPE)
    {
        sprintf(msg.data, "Client \"%s\" left chat!\n%n", clients[id].name, &size);
    }
    
    for (int i = 0, k = 0; i < MaxNumClients; i++)
    {
        if (!clients[i].is_available) continue;

        int tmp_size = 0;
        sprintf(msg.data + size, "[%d] \"%s\" \t%d%n", 
                i, 
                clients[i].name, 
                clients[i].pid, 
                &tmp_size);

        size += tmp_size;

        if(k != num_clients - 1) { sprintf(msg.data + size, "\n"); size++; }
        k++;
    }

    printf("INFO:\n%s\n", msg.data);
    
    for (int i = 0; i < MaxNumClients; i++)
    {
        if (!clients[i].is_available || i == id) continue;
        
        msg.receiver = clients[i].pid;
        msgsnd(server_id, &msg, MsgSize, 0);
    }
}

//-------------------------------------------------------------------

void send_msg(Msg_t* msg, int server_id, Client* clients, int num_clients)
{
    if (msg->msg_type != MSG_TYPE) return;

    int receiver = get_pid_by_name(clients, msg->receiver_name);
    if (receiver == -1) 
    {
        printf("Client \"%s\" does not exist :(\n", msg->receiver_name);
        return;
    }
    
    msg->receiver = receiver;
    msgsnd(server_id, msg, MsgSize, 0); 
}

//-------------------------------------------------------------------