//
// Created by ken on 6/17/18.
//

#include "MyProtocol.h"
#include <vector>
#include <queue>
#include <pthread.h>
#include <arpa/inet.h>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>

#define MAX_LISTEN_SIZE 10 //max size of requst queue

using namespace std;
int debug = 0;
char *server_name = "ken";
void *clientThread(void *);

static int conn_fd;
static int listen_fd;

vector<client_info> client_list;

int main(int argc, char *argv[])
{
    int pId, portNo;
    socklen_t len; //store size of the address

    struct sockaddr_in server_address, client_address;

    pthread_t thread_pool[MAX_CLIENT_NUM];

    if (argc < 2)
    {
        cerr << "Syntax : ./server <port>" << endl;
        return 0;
    }

    portNo = atoi(argv[1]);

    if ((portNo > 65535) || (portNo < 2000))
    {
        cerr << "Please enter a port number between 2000 - 65535" << endl;
        return 0;
    }

    //create socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_fd < 0)
    {
        cerr << "Cannot open socket" << endl;
        return 0;
    }

    memset((char *)&server_address,0,sizeof(server_address));//initial

    server_address.sin_family = AF_INET;
    //server_addresssin_addr.s_addr = inet_addr("127.0.0.1");//bind to localhost only

    //When we use this value as the address when calling bind(), the socket accepts connections to all the IPs of the machine.
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(portNo);

    //bind socket
    //in mac, you should add "::"  before bind or you will get an error
    if (::bind(listen_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        cerr << "Cannot bind" << endl;
        return 0;
    }

    listen(listen_fd, 10);

    len = sizeof(client_address);

    int thread_num = 0;

    while (thread_num < MAX_CLIENT_NUM)
    {
        cout << "Listening" << endl;

        //this is where client connects. svr will hang in this mode until client conn
        int conn_fd = accept(listen_fd, (struct sockaddr *)&client_address, &len);
        if (conn_fd < 0)
        {
            cerr << "Cannot accept connection" << endl;
            return 0;
        }
        else
        {
            cout << "Connection successful" << endl;
        }

        printf("IP address is: %s\n", inet_ntoa(client_address.sin_addr));
        printf("port is: %d\n", (int)ntohs(client_address.sin_port));
        client_info temp;
        temp.num = conn_fd;
        char* ip_address = inet_ntoa(client_address.sin_addr);
        memset(temp.ip, 0, 15);
        strncpy(temp.ip,ip_address,strlen(ip_address));
       
        temp.port = (int)ntohs(client_address.sin_port);
        client_list.push_back(temp);
        
        int *args = &conn_fd;
        pthread_create(&thread_pool[thread_num], NULL, clientThread, (void *)args);

        thread_num++;
    }

    for (int i = 0; i < thread_num; i++)
    {
        pthread_join(thread_pool[i], NULL);
    }
    close(listen_fd); //关闭监听套接字
}

void *clientThread(void *args)
{
    cout << "Thread No: " << pthread_self() << endl;
    int conn_fd = *(int *)args;
    cout << "connect id No: " << conn_fd << endl;
    int cnt = 0;

    while (1)
    {

        unsigned char test[READ_BUFFER_SIZE];
        memset(test, 0, READ_BUFFER_SIZE);

        int packet_size = sizeof(Packet);
        int received = 0;
        int bytes = 0;
        do
        {
            bytes = recv(conn_fd, test + received, packet_size - received,0);
            if (bytes < 0)  cerr<<"ERROR reading recv_buffer from socket";
            if (bytes == 0) //the server is closed
                break;
            received += bytes;
        } while (received < packet_size);

        if(debug) cout << "l is :" << bytes << endl;
        if (bytes == 0)
        {
            cout << "The client is shut down\n"
                 << client_list.size() << endl;
            cout << client_list.begin()->num << "asd\n";

            auto iter = client_list.begin();
            for (; iter != client_list.end();)
            {
                if (conn_fd == iter->num)
                {
                    iter = client_list.erase(iter);
                }
                else
                    iter++;
            }
            break;
        }
        Packet *phead = (Packet *)test;
        int data_length = phead->header.length - sizeof(PacketHeader);
        if(debug) cout << "time le" << data_length << endl;

        printf("datais:%s\n", phead->body.data);
        if (data_length > 0 && data_length > sizeof(Packet) - sizeof(PacketHeader))
        { //need extra read; not used 
            received = 0;
            cout << "extra read\n";
            do
            {
                bytes = recv(conn_fd, test + packet_size + received, data_length - received,0);

                if (bytes < 0) cerr << "ERROR reading recv123_buffer from socket";
                if (bytes == 0) //the server is closed
                {
                    break;
                }
                received += bytes;

            } while (received < data_length);
        }

        if (phead->header.op == TIME)
        {
            cout << "Get time request\n";
            time_t nowtime;
            nowtime = time(NULL); //获取日历时间
            Packet to_send(listen_fd, conn_fd, sizeof(Packet), 0, TIME, nullptr);
            strncpy((char *)to_send.body.data, (const char *)&nowtime, sizeof(nowtime)); //copy data

            send(conn_fd, &to_send, sizeof(to_send),0); 
            
            //debug info
            cout << "Send to client success " << sizeof(to_send) << endl;
            cout << "Send Time to client success " << endl;
            printf("raw data is : %s\n", to_send.body.data);
            printf("data is : %ld\n", *((time_t *)to_send.body.data));
            cnt++;
            cout << ctime(&nowtime);
            cout << "cnt:" << cnt << endl;
        }
        else if (phead->header.op == NAME)
        {
            cout << "Get name request\n";

            Packet to_send(listen_fd, conn_fd, sizeof(Packet), 0, NAME, nullptr);

            strncpy((char *)to_send.body.data, (const char *)server_name, strlen(server_name) + 1); //copy data

            send(conn_fd, &to_send, sizeof(to_send),0); 

            cout << "Send to client success " << sizeof(to_send) << endl;
            cout << "Send name to client success "  << endl;
            printf("raw data is : %s\n", (char *)to_send.body.data);
            cout << ((char *)to_send.body.data) << endl;
            cnt++;
            cout << "cnt:" << cnt << endl;
    
        }
        else if (phead->header.op == ACTIVE_LIST)
        { //use an vector
            cout << "Get active list request\n";

            Packet to_send(listen_fd, conn_fd, sizeof(Packet), 0, ACTIVE_LIST, nullptr);
            for (int i = 0; i < client_list.size(); ++i)
            {   
                
                to_send.body.list[i].port = client_list[i].port;
                to_send.body.list[i].num = client_list[i].num;
                strncpy(to_send.body.list[i].ip, client_list[i].ip, 15);
                if(conn_fd==to_send.body.list[i].num) {
                    to_send.body.list[i].isThisMyfd = 1;
                    cout<<"done\n";
                    }
                else to_send.body.list[i].isThisMyfd = 0;
                
            }
            int n = client_list.size();

            strncpy((char *)to_send.body.data, (const char *)&n, sizeof(int)); //copy data

            send(conn_fd, &to_send, sizeof(to_send),0); 

            //debug info
            cout << "Send to client success " << sizeof(to_send) << endl;
            cout << "Send name to client success " << sizeof(to_send) - sizeof(PacketHeader) << endl;
            printf("raw data is : %s\n", (char *)to_send.body.data);
            printf("num of data is : %d\n", *(int *)to_send.body.data);
            for (int i = 0; i < *(int *)to_send.body.data; ++i)
            {   

                cout << to_send.body.list[i].port << endl;
                cout << to_send.body.list[i].ip << endl;
                cout << sizeof(to_send.body.list[i].ip) << endl;
            }

            cnt++;
            cout << "cnt:" << cnt << endl;
        }
        else if (phead->header.op == MESSAGE && phead->header.type == 1)
        { //require message
            cout << "Get message request\n";

            Packet to_send = *phead;

            int isExist = 0;
            for (auto i : client_list)
            {
                if (i.num == to_send.header.destination)
                {
                    isExist = 1;
                    break;
                }
            } 
            //error handle
            if (isExist == 0)
            {
                perror("No such client!\n");
                Packet error_info(listen_fd, conn_fd, sizeof(Packet), 2, ERROR, nullptr);
                send(conn_fd, &error_info, sizeof(error_info),0); //maybe bug
                cout << "Send error message to client success " << conn_fd << endl;
            }
            else
            {
                cout << "req:" << to_send.header.source << " to " << to_send.header.destination << endl;
                send(to_send.header.destination, &to_send, sizeof(to_send),0); //maybe bug

                cout << "Send message request to client success " << sizeof(to_send) << endl;
                printf("raw data is : %s\n", (char *)to_send.body.data);
                printf("fd is : %d\n", to_send.header.destination);
            }

            cnt++;
            cout << "cnt:" << cnt << endl;
        }
        else if (phead->header.op == MESSAGE && phead->header.type == 0)
        { 
            cout << "Get message reply\n";

            Packet to_send = *phead;

            cout << to_send.header.source << " send to:" << to_send.header.destination << endl;
            send(to_send.header.destination, &to_send, sizeof(to_send),0); //maybe bug

            cout << "Send message reply to client success " << sizeof(to_send) << endl;
            printf("raw data is : %s\n", (char *)to_send.body.data);
            printf("num of data is : %d\n", *(int *)to_send.body.data);

            cnt++;
            cout << "cnt:" << cnt << endl;
        }
    }
}