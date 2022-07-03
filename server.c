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
int equipments[MAX_CONNECTIONS][2]; // Relação entre os identificadores de equipamentos e o descritor socket
int nextId = 1; // Armazena o próximo ID de equipamento. Não será implementada uma reinicialização desse valor, à princípio


int getEquipmentFreePosition() {
    for (int i=0; i < MAX_CONNECTIONS; i++) {
        if (equipments[i][0] == 0) {
            return i;
        }
    }
    return -1;
}

int getEquipmentIndex(int equipmentId) {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (equipments[i][0] == equipmentId) {
            return i;
        }
    }
    return -1;
}

// Envia lista de equipamentos conectados para um cliente que acabou de se conectar
void sendList(int clntSock) {
    
    // Cria mensagem
    Message msg;
    msg.id = RES_LIST;
    msg.originId = 0;
    msg.destinationId = 0;
    strcpy(msg.payload, "");


    char list[5];

    // Monta lista
    for (int i=0; i < MAX_CONNECTIONS; i++) {
        // Verifica se existe um equipamento na posição
        if (equipments[i][0] != 0) {
            sprintf(list, "%d,", equipments[i][0]);
            strcat(msg.payload, list);
        }
    }

    // Remove vírgula que ficou na última posição
    msg.payload[strlen(msg.payload) - 1] = '\0';

    // Envia mensagem
    sendMessage(clntSock, msg);
}

// Adiciona um novo equipamento
void addEquipment(int clntSock, Message msg) {

    Message resMsg;

    // Busca posição librvre na memória para inserir equipamento
    int position = getEquipmentFreePosition();

    // Se não tiver espaço disponível, envia mensagem de erro
    if (position == -1) {
        // Monta mensagem de erro
        resMsg.id = MSG_ERR;
        resMsg.originId = 0;
        resMsg.destinationId = 0;
        sprintf(resMsg.payload, "%d", ERR_EQUIP_LIMIT_EXCEEDED);
        
        // Retorna erro para o cliente
        sendMessage(clntSock, resMsg);

        return;
    }

    // Monta mensagem
    resMsg.id = RES_ADD;
    resMsg.originId = 0;
    resMsg.destinationId = 0;
    sprintf(resMsg.payload, "%d", nextId);

    // Adiciona à base
    equipments[position][0] = nextId;
    equipments[position][1] = clntSock;

    // Converte ID para string e imprime mensagem
    char* formattedId = getFormattedId(nextId);
    printf("Equipment %s added\n", formattedId);

    // Incrementa o próximo ID
    nextId++;

    // Simula um broadcast, enviando para todos os clientes
    for (int i=0; i < MAX_CONNECTIONS; i++) {
        if (equipments[i][0] != 0) {
            sendMessage(equipments[i][1], resMsg);
        }
    }

    // Envia lista de equipmentos para aquele que foi adicionado
    sendList(clntSock);

    free(formattedId);
}

// Trata solicitação de remoção de um equipamento
void removeEquipment(int clntSock, Message msg) {
    int equipmentId = msg.originId;
    Message resMsg;

    // Busca equipamento na lista
    int equipmentIndex = getEquipmentIndex(equipmentId);

    // Se não foi encontrado, envia mensagem de erro
    if (equipmentIndex == -1) {

        // Monta mensagem de erro
        resMsg.id = MSG_ERR;
        resMsg.originId = 0;
        resMsg.destinationId = equipmentId;
        sprintf(resMsg.payload, "%d", ERR_EQUIP_NOT_FOUND);

        // Envia mensagem para o cliente
        sendMessage(clntSock, resMsg);
        return;
    }

    // Remove cliente da base
    equipments[equipmentIndex][0] = 0;
    equipments[equipmentIndex][1] = 0;

    // Imprime mensagem
    char* formattedId = getFormattedId(equipmentId);
    printf("Equipment %s removed\n", formattedId);

    // Monta mensagem de resposta
    resMsg.id = MSG_OK;
    resMsg.originId = 0;
    resMsg.destinationId = equipmentId;
    strcpy(resMsg.payload, "");

    // Envia OK para o cliente
    sendMessage(clntSock, resMsg);

    // Simula um broadcast, repassando mensagem para todos os clientes conectados
    for (int i=0; i < MAX_CONNECTIONS; i++) {
        if (equipments[i][0] != 0) {
            sendMessage(equipments[i][1], msg);
        }
    }

    free(formattedId);
}

// Envia solicitação de informação para o destino e devolve para a origem
void infoRequestOrResponse(int clntSocket, Message msg) {

    Message errorMsg;
    char* formattedId;

    // Verifica equipamento de origem
    formattedId = getFormattedId(msg.originId);
    int originIndex = getEquipmentIndex(msg.originId);
    if (originIndex == -1) {
        // Imprime erro
        printf("Equipment %s not found\n", formattedId);

        // Monta mensagem
        errorMsg.id = MSG_ERR;
        errorMsg.originId = 0;
        errorMsg.destinationId = msg.originId;
        sprintf(errorMsg.payload, "%d", ERR_SRC_EQUIP_NOT_FOUND);

        // Faz o envio
        sendMessage(clntSocket, errorMsg);
        return;
    }

    // Verifica equipamento de destino
    formattedId = getFormattedId(msg.destinationId);
    int destinationIndex = getEquipmentIndex(msg.destinationId);
    if (destinationIndex == -1) {
        // Imprime erro
        printf("Equipment %s not found\n", formattedId);

        // Monta mensagem
        errorMsg.id = MSG_ERR;
        errorMsg.originId = 0;
        errorMsg.destinationId = msg.originId;
        sprintf(errorMsg.payload, "%d", ERR_TAR_EQUIP_NOT_FOUND);

        // Faz o envio
        sendMessage(clntSocket, errorMsg);
        return;
    }

    // Repassa mensagem para o destino
    int destinationSock = equipments[destinationIndex][1];
    sendMessage(destinationSock, msg);

    free(formattedId);
}


//
// Fim dos métodos de protocolo 
// =========================================================================================

// Lida com um cliente específico
void handleTCPClient(int clntSocket) {

    // numConnections++;

    while(1) {

        Message rcvdMsg;

        // Aguarda mensagem do cliente
        if (!receiveMessage(clntSocket, &rcvdMsg)) {
            break;
        }

        // Identifica tipo de mensagem e realiza ação
        switch (rcvdMsg.id) {
            case REQ_ADD:
                addEquipment(clntSocket, rcvdMsg);
                break;
            case REQ_REM:
                removeEquipment(clntSocket, rcvdMsg);
                break;
            case REQ_INF:
            case RES_INF:
                infoRequestOrResponse(clntSocket, rcvdMsg);
                break;
            default:
                break;
        }
    }

    // Encerra a conexao com um cliente
    close(clntSocket);
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
        // Removendo print que pode atrapalhar correção automática
        // printf("Handling client %s/%d\n", clntName, ntohs(clntAddr.sin_port));
    } else {
        puts("Unable to get client address");
    }

    return clntSock;
}


// =========================================================================================
// Métodos para lidar com threads
//

// Programa principal de cada Thread
void *ThreadMain(void *args) {
    // Garante que os recursos da thread sejam desalocados no final
    pthread_detach(pthread_self());

    // Recupera o identificador socket dos parâmetros
    int clntSocket = ((ThreadArgs *) args)->sock;

    // Escuta solicitações do cliente
    handleTCPClient(clntSocket);

    free(args);
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

    // Escuta conexões
    if (listen(servSock, MAX_CONNECTIONS) < 0) {
        dieWithSystemMessage("listen() failed");
    }

    // Inicializa os equipamentos
    for (int i=0; i < MAX_CONNECTIONS; i++) {
        memset(equipments[i], 0, 2);
    }

    // Loop principal
    while(1) { 

        // Aceita conexao com um cliente
        int clntSock = connectToClient(servSock);

        // Cria espaço de memória para os argumentos do cliente
        ThreadArgs *threadArgs = (ThreadArgs *) malloc(sizeof(ThreadArgs));

        if (threadArgs == NULL) {
            dieWithSystemMessage("malloc() failed");
        }

        threadArgs->sock = clntSock;

        // Cria thread para o cliente
        pthread_t threadId;
        int pthreadResult = pthread_create(&threadId, NULL, ThreadMain, threadArgs);

        if (pthreadResult != 0) {
            dieWithUserMessage("pthread_create() failed", strerror(pthreadResult));
            printf("with thread %lu\n", (unsigned long) threadId);
        }
    }
}