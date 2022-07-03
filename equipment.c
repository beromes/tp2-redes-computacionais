#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "common.h"

int equipments[MAX_CONNECTIONS];
int myId = 0;

// Cria um IPv4 para o equipamento
struct sockaddr_in makeIPv4Address(char *ip, in_port_t port) {
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;

    // Converte endereco
    int rtnVal = inet_pton(servAddr.sin_family, ip, &servAddr.sin_addr.s_addr);
    if (rtnVal == 0) {
        dieWithUserMessage("inet_pton() failed", "invalid address string");
    } else if (rtnVal < 0) {
        dieWithSystemMessage("inet_pton() failed");
    }
    
    // Coloca porta do servidor
    servAddr.sin_port = htons(port);

    return servAddr;
}

int createSocketConnection(char *servIP, in_port_t servPort) {

    // Cria socket utilizando TCP
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (sock < 0) {
        dieWithSystemMessage("socket() failed");
    }

    // Cria endereço para o equipamento
    struct sockaddr_in servAddr = makeIPv4Address(servIP, servPort);

    // Tenta conectar-se ao servidor
    int connectStatus = connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0;

    if (connectStatus < 0) {
        dieWithSystemMessage("connect() failed");
    }

    return sock;
}

// Imprime mensagem de erro no terminal
void printError(int errorCode) {
    switch (errorCode) {
        case ERR_EQUIP_NOT_FOUND:
            printf("Equipment not found\n");
            break;
        case ERR_SRC_EQUIP_NOT_FOUND:
            printf("Source equipment not found\n");
            break;
        case ERR_TAR_EQUIP_NOT_FOUND:
            printf("Target equipment not found\n");
            break;
        case ERR_EQUIP_LIMIT_EXCEEDED:
            printf("Equipment limit exceeded\n");
            break;
        default:
            break;
    }
}

// Manda REQ_ADD para o servidor para obter o identificador
void getEquipmentId(int sock) {

    // Monta mensagem
    Message reqMsg;
    reqMsg.id = REQ_ADD;
    reqMsg.destinationId = 0;
    reqMsg.originId = 0;
    strcpy(reqMsg.payload, "");

    // Inicializa lista de IDs dos outros equipamentos
    memset(equipments, 0, MAX_CONNECTIONS);

    // Envia mensagem para o servidor
    sendMessage(sock, reqMsg);

    // Aguarda resposta
    Message resMsg;
    receiveMessage(sock, &resMsg);

    // Verifica se ocorreu erro
    if (resMsg.id == MSG_ERR) {
        int errorCode = atoi(resMsg.payload);
        printError(errorCode);
        return;
    }

    if (resMsg.id != RES_ADD) {
        dieWithSystemMessage("Esperava RES_ADD, mas outra mensagem foi recebida");
    }

    // Salva ID do cliente
    myId = atoi(resMsg.payload);
    
    // Converte ID para string e imprime mensagem
    char* formattedId = getFormattedId(myId);
    printf("New ID: %s\n", formattedId);
    
    free(formattedId);

    printf("SAIUU\n");
}

// Recebe RES_ADD e adiciona o novo equipamento à sua lista
void saveNewEquipment(Message msg) {

    // Procura por espaço vazio
    for (int i=0; i < MAX_CONNECTIONS; i++) {
        if (equipments[i] == 0) {
            
            // Salva ID do novo equipamento
            equipments[i] = atoi(msg.payload);
            
            // Formata ID e imprime mensagem
            char* formattedId = getFormattedId(equipments[i]);
            printf("Equipment %s added\n", formattedId);

            free(formattedId);
            return;
        }
    }
}

// Recebe RES_LIST e atualiza lita de equipamentos
void updateEquipmentsList(Message msg) {

    // Reinicia lista de equipamentos
    memset(equipments, 0, MAX_CONNECTIONS);

    // Inicializa contador
    int i = 0;

    char* ptr = strtok(msg.payload, ",");

    while(ptr != NULL) {
        equipments[i] = atoi(ptr);
        ptr = strtok(NULL, ",");
        i++;
    }

    for(int i=0; i < MAX_CONNECTIONS; i++) {
        printf("%d: %d\n", i, equipments[i]);
    }
}


int main(int argc, char *argv[]) {
    
    if (argc != 3) {
        dieWithUserMessage("Parameter(s)", "<Server Address> <Server Port>");
    }
        
    char *servIP = argv[1]; // Primeiro parametro: endereco IP do servidor
    in_port_t servPort = atoi(argv[2]); // Segundo parametro: porta do servidor
        
    // Cria socket usando o TCP
    int sock = createSocketConnection(servIP, servPort);

    // Envia requisição REQ_ADD para obter o seu ID
    getEquipmentId(sock);

    // Loop Principal
    while(1) {

        // Aguarda resposta
        Message resMsg;
        receiveMessage(sock, &resMsg);

        printf("RECEIVED MSG ID: %d\n", resMsg.id);

        // Realiza ação de acordo com o tipo da mensagem
        switch (resMsg.id) {
            case RES_ADD:
                saveNewEquipment(resMsg);
                break;
            case RES_LIST:
                updateEquipmentsList(resMsg);
                break;
            default: 
                break;
        }

    }

    close(sock);
    exit(0);
}