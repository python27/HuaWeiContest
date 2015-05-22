#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <sstream>
#include "message.h"
using namespace std; 

#define SIZE 1024

char client_msg[SIZE];
char server_msg[SIZE];
int sockfd; 

int main(int argc, char *argv[])
{


    if (argc != 6)
    {
        printf("Usage %s <Server IP> <Server Port> <Client IP> <Client Port> <PID>\n", argv[0]);
        return 1;
    } 

    char SERVER_IP[1024];
    strcpy(SERVER_IP, argv[1]);
    int SERVER_PORT = atoi(argv[2]);
    char CLIENT_IP[1024];
    strcpy(CLIENT_IP, argv[3]);
    int CLIENT_PORT = atoi(argv[4]);
    int PID = atoi(argv[5]);

    printf("Server ip: %s\n", SERVER_IP);
    printf("Server port: %d \n", SERVER_PORT);

    // specify client addr
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_addr.s_addr = inet_addr(CLIENT_IP);
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(CLIENT_PORT);

    // create socket descriptor
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 
    
    // bind client addr & socket descriptor
    if (bind(sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr)) == -1)
    {
        printf("\n Error: Client Bind Failed !\n");
        printf("Client IP:%s\n", CLIENT_IP);
        printf("Client Port:%d\n", CLIENT_PORT);
        return 1;
    }

    printf("socked fd:%d\n", sockfd);

    // specify server addr
    struct sockaddr_in serv_addr; 
    memset(&serv_addr, 0, sizeof(serv_addr)); 
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT); 


    // connect to server
    while (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        usleep(100);
        printf("\n Error : Connect Failed \n");
    }
    
    // send reg_msg to server
    memset(client_msg, 0, SIZE);
    sprintf(client_msg, "reg: %d pname \n", PID);
    printf("reg message: %s\n", client_msg);
    send(sockfd, client_msg, strlen(client_msg), 0);
    printf("send reg message successfully !\n");

    // receive msg from server
    int recv_length = 0;
    int process_msg_flag = 0;
    while (1)
    {
        memset(server_msg, 0, SIZE);
        recv_length = recv(sockfd, server_msg, SIZE, 0);
        if (recv_length < 0)
        {
            printf("Receive Data From Server %s Failed", SERVER_IP);
            break;
        }

        printf(">>>>>>>>>>>> Received Data start <<<<<<<<<<<<<<\n");
        printf("%s", server_msg);
        printf(">>>>>>>>>>>> Received Data end <<<<<<<<<<<<<<\n");

        printf("I am handling message:\n");
        
        process_msg_flag = ProcessReceivedMsg(server_msg);
        
        // game has been over, sock has been closed
        if (process_msg_flag == 1)
        {
            break;
        }      
    }

    // game is not over, remember close socket
    if (process_msg_flag == 0)
    {
        close(sockfd);
    }


    return 0;
}
