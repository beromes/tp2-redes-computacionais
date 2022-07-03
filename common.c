#include <stdio.h>
#include <stdlib.h>
#include "common.h"

// =========================================================================================
// Tratamento de erros
//

void dieWithUserMessage(const char *msg, const char *detail) {
    fputs(msg, stderr);
    fputs(": ", stderr);
    fputs(detail, stderr);
    fputc('\n', stderr);
    exit(1);
}

void dieWithSystemMessage(const char *msg) {
    perror(msg);
    exit(1);
}

//
// Fim dos métodos de tratamento de erros
// =========================================================================================


// =========================================================================================
// Métodos para conversão de structs em strings e vice-versa
//

void messageToStr(Message msg, char** str) {
    sprintf(*str, "%d|%d|%d|%s\n", msg.id, msg.originId, msg.destinationId, msg.payload);
}

void strToMessage(char* str, Message* message) {

    char* ptr = strtok(str, "|");

    message->id = atoi(ptr);

    ptr = strtok(NULL, "|");
    message->originId = atoi(ptr);

    ptr = strtok(NULL, "|");
    message->destinationId = atoi(ptr);

    ptr = strtok(NULL, "|");
    
    ptr[strlen(ptr) - 1] = '\0'; // Remove '\n' que indica o fim da mensagem
    
    strcpy(message->payload, ptr);
}

//
// Fim dos métodos para conversão de structs em strings e vice-versa
// =========================================================================================



// =========================================================================================
// Métodos de comunicação com o servidor
//

// Envia mensagem para o servidor/cliente
void sendMessage(int sock, Message msg) {
    
    char* strMsg = (char*) malloc(BUFFER_SIZE * sizeof(char));
    messageToStr(msg, &strMsg);

    size_t msgLen = strlen(strMsg);

    ssize_t numBytes = send(sock, strMsg, msgLen, 0);

    if (numBytes < 0) {
        dieWithSystemMessage("send() failed");    
    } else if (numBytes != msgLen) {
        dieWithUserMessage("send()", "sent unexpected number of bytes");
    }
    
    free(strMsg);
}


// Recebe mensagem do servidor/cliente
void receiveMessage(int sock, Message *message) {

    char strRcvd[BUFFER_SIZE]; // I/O buffer
    memset(strRcvd, BUFFER_SIZE, 0);
        
    ssize_t numBytesRcvd = recv(sock, strRcvd, BUFFER_SIZE, 0);

    if (numBytesRcvd < 0) {
        dieWithSystemMessage("recv() failed");
    } else if (numBytesRcvd == 0) {
        dieWithUserMessage("recv()", "connection closed prematurely");
    }

    printf("%s", strRcvd);

    strToMessage(strRcvd, message);
}

//
// Fim dos métodos de comunicação com o servidor
// =========================================================================================
