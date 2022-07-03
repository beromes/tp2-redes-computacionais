#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "common.h"

// =========================================================================================
//  Métodos do protocolo
//

const int MAX_CONNECTIONS = 15; // Quantidade maxima de conexões simultâneas
int numConnections = 0; 
int equipments[15][2];

void addEquipment(int clntSock, Message msg) {

    Message resMsg;

    if (numConnections > MAX_CONNECTIONS) {
        
        // Decrementa número de conexões
        numConnections--;

        // Monta mensagem de erro
        resMsg.id = MSG_ERR;
        sprintf(resMsg.payload, "%d", ERR_EQUIP_LIMIT_EXCEEDED);
        
        // Retorna erro para o cliente
        sendMessage(clntSock, resMsg);

        return;
    }



}



//
// Fim dos métodos de protocolo 
// =========================================================================================

// Lida com um cliente específico
void handleTCPClient(int clntSocket) {

    numConnections++;

    while(1) {

        Message rcvdMsg;

        // Aguarda mensagem do cliente
        receiveMessage(clntSocket, &rcvdMsg);

        // Identifica tipo de mensagem e realiza ação
        switch (rcvdMsg.id) {
            case REQ_ADD:
                addEquipment(clntSocket, rcvdMsg);
                break;
            default:
                break;
        }
    }

    // Encerra a conexao com um cliente
    close(clntSocket);

    // char* messageRcvd; // Buffer para mensagem recebida

    // // Recebe mensagem do cliente
    // ssize_t numBytesRcvd = recv(clntSocket, messageRcvd, BUFFER_SIZE, 0);
    // if (numBytesRcvd < 0) {
    //     dieWithSystemMessage("recv() failed");
    // }

    // // Send received string and receive again until end of stream
    // while (numBytesRcvd > 0) {

    //     printf("%s", messageRcvd);

    //     char* returnMessage = messageRcvd; //execCommand(clntSocket, messageRcvd);

    //     if (returnMessage == NULL) {
    //         break;
    //     }

    //     size_t returnMessageSize = strlen(returnMessage);

    //     ssize_t numBytesSent = send(clntSocket, returnMessage, BUFFER_SIZE, 0);

    //     if (numBytesSent < 0) {
    //         dieWithSystemMessage("send() failed");
    //     }

    //     // See if there is more data to receive
    //     numBytesRcvd = recv(clntSocket, messageRcvd, BUFFER_SIZE, 0);
    //     if (numBytesRcvd < 0) {
    //         dieWithSystemMessage("recv() failed");
    //     }

    //     // free(returnMessage);
    // }

    // close(clntSocket); // Encerra a conexao com um cliente
}

// Cria um endereco IPv4 para o servidor
struct sockaddr_in makeIPv4Address(in_port_t port) {
    struct sockaddr_in servAddr;                    // Local address
    memset(&servAddr, 0, sizeof(servAddr));         // Zero out structure
    servAddr.sin_family = AF_INET;                  // Address family
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);   // Any incoming interface
    servAddr.sin_port = htons(port);                // Local port
    return servAddr;
}

// Cria socket 
int createSocket() {
    int servSock;
    if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        dieWithSystemMessage("socket() failed");
    }
    return servSock;
}

// Faz o bind utilizando protocolo IPv4
void bindAddr(in_port_t servPort, int servSock) {
    struct sockaddr_in ipv4 = makeIPv4Address(servPort);
    int bindStatus = bind(servSock, (struct sockaddr *) &ipv4, sizeof(ipv4));
    
    if (bindStatus < 0) {
        dieWithSystemMessage("bind() failed");
    }
}

// Aceita conexao com clientes
int connectToClient(int servSock) {
    struct sockaddr_in clntAddr; // Endereco do cliente        
    socklen_t clntAddrLen = sizeof(clntAddr); // Tamanho do endereco do cliente
    
    // Espera por um cliente para conectar
    int clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen);
    if (clntSock < 0) {
        dieWithSystemMessage("accept() failed");
    }
    
    char clntName[INET_ADDRSTRLEN]; // String para armazenar o endereco do cliente
    
    if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName, sizeof(clntName)) != NULL) {
        printf("Handling client %s/%d\n", clntName, ntohs(clntAddr.sin_port));
    } else {
        puts("Unable to get client address");
    }

    return clntSock;
}


// =========================================================================================
// Métodos para lidar com threads
//

// Thread Args
struct ThreadArgs {
    int clntSock;
};


// Programa principal de cada Thread
void *ThreadMain(void *args) {
    // Garante que os recursos da thread sejam desalocados no final
    pthread_detach(pthread_self());

    // Recupera o identificador socket dos parâmetros
    int clntSocket = ((struct ThreadArgs *) args)->clntSock;
    
    free(args);

    handleTCPClient(clntSocket);

    return NULL;
}

//
// Fim dos métodos de thread 
// =========================================================================================


int main(int argc, char *argv[]) {

    // Valida a quantidade de parametros do programa
    if (argc != 2) {
        dieWithUserMessage("Parameter(s)", "<Server Port>");
    }

    // Converte parâmetro <Server Port> para valor inteiro
    in_port_t servPort = atoi(argv[1]);

    // Cria socket
    int servSock = createSocket();

    // Vincula o endereco de acordo com a familia do protoclo (v4 ou v6)
    bindAddr(servPort, servSock);

    // Mark the socket so it will listen for incoming connections
    if (listen(servSock, MAX_CONNECTIONS) < 0) {
        dieWithSystemMessage("listen() failed");
    }

    // Loop principal
    while(1) { 

        // Aceita conexao com um cliente
        int clntSock = connectToClient(servSock);

        // Cria espaço de memória para os argumentos do cliente
        struct ThreadArgs *threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs));

        if (threadArgs == NULL) {
            dieWithSystemMessage("malloc() failed");
        }

        threadArgs->clntSock = clntSock;

        // Cria thread para o cliente
        pthread_t threadId;
        int pthreadResult = pthread_create(&threadId, NULL, ThreadMain, threadArgs);

        if (pthreadResult != 0) {
            dieWithUserMessage("pthread_create() failed", strerror(pthreadResult));
            printf("with thread %lu\n", (unsigned long) threadId);
        }
    }
}