#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
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
}

// Recebe REQ_REM e remove equipmaneto da lista
void removeEquipment(Message msg) {

    for(int i=0; i < MAX_CONNECTIONS; i++) {
        if (equipments[i] == msg.originId) {
            // Imprime mensagem
            char* formattedId = getFormattedId(msg.originId);
            printf("Equipment %s removed\n", formattedId);

            // Libera ponteiro
            free(formattedId);
            return;
        }
    }
}

// Envia um REQ_REM para o servidor solicitando o fechamento da conexão
void closeConnection(int sock) {
    
    // Monta mensagem
    Message msg;
    msg.id = REQ_REM;
    msg.originId = myId;
    msg.destinationId = 0;
    strcpy(msg.payload, "");

    // Envia mensagem para o servidor
    sendMessage(sock, msg);

    // Aguarda por mensagem de retorno
    Message resMsg;
    receiveMessage(sock, &resMsg);

    // Verifica se ocorreu um erro e imprime mensagem
    if (resMsg.id == MSG_ERR) {
        int errorCode = atoi(resMsg.payload);
        printError(errorCode);
        return;
    }

    if (resMsg.id == MSG_OK) {
        printf("Successful removal\n");
        close(sock);
        exit(1);
    }

}

// =========================================================================================
// Thread responsável por escutar entrada do teclado e enviar solicitações para o servidor
//

void *InputThread(void *args) {
    // Garante que os recursos da thread sejam desalocados no final
    pthread_detach(pthread_self());

    // Recupera o identificador socket dos parâmetros
    int sock = ((ThreadArgs *) args)->sock;

    while (1) {
        
        // Lê string do teclado
        char *line = NULL;
        size_t lineLen = 0;
        getline(&line, &lineLen, stdin);

        if (strcmp(line, "close connection\n") == 0) {
            closeConnection(sock);
            break;
        } else if (strcmp(line, "list equipment\n") == 0) {
            //listEquipments();
        } else {
            // TODO: verificar string
        }
    }

    free(args);
    return NULL;
}

//
// Fim dos métodos de thread 
// =========================================================================================

int main(int argc, char *argv[]) {
    
    if (argc != 3) {
        dieWithUserMessage("Parameter(s)", "<Server Address> <Server Port>");
    }
        
    char *servIP = argv[1]; // Primeiro parametro: endereco IP do servidor
    in_port_t servPort = atoi(argv[2]); // Segundo parametro: porta do servidor
        
    // Cria socket usando o TCP
    int sock = createSocketConnection(servIP, servPort);


    // Aloca espaço de memória para argumento da thread
    ThreadArgs *threadArgs = (ThreadArgs *) malloc(sizeof(ThreadArgs));

    if (threadArgs == NULL) {
        dieWithSystemMessage("malloc() failed");
    }

    threadArgs->sock = sock;

    // Cria thread para ler entradas do teclado
    pthread_t threadId;
    int pthreadResult = pthread_create(&threadId, NULL, InputThread, threadArgs);

    // Envia requisição REQ_ADD para obter o seu ID
    getEquipmentId(sock);

    // Loop Principal
    while(1) {

        // Aguarda resposta
        Message resMsg;
        
        if (!receiveMessage(sock, &resMsg)) {
            break;
        }

        // Realiza ação de acordo com o tipo da mensagem
        switch (resMsg.id) {
            case RES_ADD:
                saveNewEquipment(resMsg);
                break;
            case RES_LIST:
                updateEquipmentsList(resMsg);
                break;
            case REQ_REM:
                removeEquipment(resMsg);
                break;
            default: 
                break;
        }

    }

    close(sock);
    exit(0);
}