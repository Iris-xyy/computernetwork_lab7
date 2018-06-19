//
// Created by apple on 2018/6/13.
//

//Author: Ken
//file: MyProtocol.cpp
// Created by Ken on 2018/6/13. 
//QAQ
#ifndef CN_SERVER_CLIENT_MYPROTOCOL_H
#define CN_SERVER_CLIENT_MYPROTOCOL_H
#include <iostream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <ctime>
#include <cstring>

#define MAX_CLIENT_NUM 10
#define READ_BUFFER_SIZE 1024
#define DATA_LENGTH 256
typedef struct {
    int num;
    char ip[15];
    int port;
    int isThisMyfd= 0;
}client_info;
enum Operation{
    CONNECT=1,CLOSE,TIME,NAME,ACTIVE_LIST,MESSAGE,EXIT,ERROR
};
class PacketHeader{
public:
    int source;//source fd
    int destination;//destination fd
    int length;//total length of data needed to pass; 
    int type;//1:require 0:reply 2:error
    Operation op;//illustrate the operation(get name/time...) of this packet
    
    PacketHeader(){}
    PacketHeader(int sourece,int destination,int length,int type, Operation op):source(sourece),destination(destination),length(length),type(type),op(op){

    }
};

typedef struct {

    unsigned char data[DATA_LENGTH];// = static_cast<unsigned char *>(malloc(1500 * sizeof(unsigned char)));
    //time_t t;
    client_info list[MAX_CLIENT_NUM];
}PacketData;

class Packet{
public:
    PacketHeader header;

    PacketData body;
    Packet(){}
    Packet(int sourece,int destination,int length,int type, Operation op,unsigned char* in_data):header(sourece,destination,length,type,op){
        memset(body.data,0,DATA_LENGTH);

    }
};
#endif //CN_SERVER_CLIENT_MYPROTOCOL_H
