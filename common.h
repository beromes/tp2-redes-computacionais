#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef COMMON_H
#define COMMON_H

#define BUFFER_SIZE 500
#define DELIMETER_END_OF_MESSAGE '\n'
#define DELIMTER_MESSAGE_SECTION '|'

// IDs das mensagens
#define REQ_ADD  1
#define REQ_REM  2
#define RES_ADD  3
#define RES_LIST 4
#define REQ_INF  5
#define RES_INF  6
#define ERROR    7
#define OK       8

typedef struct Message {
    int id;
    int originId;
    int destinationId;
    char payload[100];
} Message;

// Mensagens de erro
void dieWithUserMessage(const char *msg, const char *detail);
void dieWithSystemMessage(const char *msg);

// Conversão de mensagens em strings
void messageToStr(Message msg, char** str);
void strToMessage(char* str, Message* message);

// Envio/Recebimento de mensagens
void sendMessage(int sock, Message msg);
void receiveMessage(int sock, Message* msg);

#endif
