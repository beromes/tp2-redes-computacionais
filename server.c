#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "common.h"

#define COMMAND_KILL    0
#define COMMAND_ADD     1
#define COMMAND_REMOVE  2
#define COMMAND_LIST    3
#define COMMAND_READ    4
#define INVALID_COMMAND -1

static const int MAXPENDING = 5; // Quantidade maxima de solicitacoes de conexao pendentes

// =========================================================================================
//  Métodos do protocolo
//

int sensorCount = 0; // Quantidade de sensores conectados

int equipmentSensors[4][4] = {0}; // Relacao entre sensores e equipamentos


int getSensorsCount(char *sensorIds) {

    int count = 0;
    char *token = (char *) malloc(strlen(sensorIds) * sizeof(char));

    strncpy(token, sensorIds, strlen(sensorIds));

    token = strtok(token, " ");  
    while(token != NULL) {
        count++;
        token = strtok(NULL, " ");
    }

    free(token);
    return count;
}

int* getSensorsIndexes(char *sensorIds, int size) {
    int* sensors = (int *) malloc(size * sizeof(int));
    int count = 0;
    char *token = (char *) malloc(strlen(sensorIds) * sizeof(char *));

    strncpy(token, sensorIds, strlen(sensorIds));

    token = strtok(token, " ");   
    while(token != NULL) {
        sensors[count] = atoi(token);
        count++;
        token = strtok(NULL, " ");
    }

    free(token);
    return sensors;
}

char* addSensor(char* sensorsIds, char* equipmentId) {

    char* message = (char *) malloc(BUFFER_SIZE * sizeof(char)); // Mensagem de retorno

    int newSensorsCount = getSensorsCount(sensorsIds);

    // Valida limite de sensores
    if (sensorCount + newSensorsCount > 15) {
        sprintf(message, "limit exceeded\n");
        return message;
    }

    int* newSensors = getSensorsIndexes(sensorsIds, newSensorsCount);

    int equipmentIndex = atoi(equipmentId);

    // Verifica se o sensor ja foi adicionado ao equipamento
    for (int i=0; i < newSensorsCount; i++) {
        int sensorIndex = newSensors[i];
        if (equipmentSensors[equipmentIndex][sensorIndex] != 0) {
            sprintf(message, "sensor 0%d already exists in %s\n", sensorIndex, equipmentId);
            return message;
        }
    }

    // Adiciona sensor ao equipamento
    for (int i=0; i < newSensorsCount; i++) {
        int sensorIndex = newSensors[i];
        equipmentSensors[equipmentIndex][sensorIndex] = 1;
        sensorCount++;
    }

    sprintf(message, "sensor %s added\n", sensorsIds);
    return message;
}

char* removeSensor(char* sensorId, char* equipmentId) {
    
    char *message = (char *) malloc(BUFFER_SIZE * sizeof(char)); // Mensagem de retorno

    int sensorIndex = atoi(sensorId);
    int equipmentIndex = atoi(equipmentId);

    // Verifica se sensor existe
    if (equipmentSensors[equipmentIndex][sensorIndex] == 0) {
        sprintf(message, "sensor %s does not exist in %s\n", sensorId, equipmentId);
        return message;
    }

    // Remove sensor do equipamento
    equipmentSensors[equipmentIndex][sensorIndex] = 0;
    sensorCount--;

    sprintf(message, "sensor %s removed\n", sensorId);
    return message;
}

char* listSensors(char* equipmentId) {    

    char* sensors = (char *) malloc(BUFFER_SIZE * sizeof(char));
    int equipmentIndex = atoi(equipmentId);

    for (int i = 0; i < 5; i++) {
        if (equipmentSensors[equipmentIndex][i] != 0) {
            char s[4];
            sprintf(s, "0%d ", i);
            strcat(sensors, s);
        }
    }

    if (strlen(sensors) == 0) {
        sprintf(sensors, "none\n");
    }

    sensors[strlen(sensors) - 1] = '\n';
    return sensors;
}

char* readSensors(char* sensorIds, char* equipmentId) {

    char* message = (char *) malloc(BUFFER_SIZE * sizeof(char));

    int ei = atoi(equipmentId);
    int sensorCount = getSensorsCount(sensorIds);
    int* sensors = getSensorsIndexes(sensorIds, sensorCount);

    for (int i = 0; i < sensorCount; i++) {
        int si = sensors[i];
        if (equipmentSensors[ei][si] == 0) {
            if (strlen(message) == 0) {
                strcat(message, "sensor(s)");
            }
            char s[4];
            sprintf(s, " 0%d", si);
            strcat(message, s);
        }
    }

    if (strlen(message) != 0) {
        strcat(message, " not installed\n");
        return message;
    }

    for (int i = 0; i < sensorCount; i++) {
        float measure = (float) rand() / (float) (RAND_MAX / 10.0);
        char s[10];
        sprintf(s, "%.2f ", measure);
        strcat(message, s);
    }

    message[strlen(message) - 1] = '\n';
    return message;

}

int getCommand(char message[]) {

    if (strstr(message, "kill") != NULL) {
        return COMMAND_KILL;
    }
    if (strstr(message, "add") != NULL) {
        return COMMAND_ADD;
    }

    if (strstr(message, "remove") != NULL) {
        return COMMAND_REMOVE;
    }

    if (strstr(message, "list") != NULL) {
        return COMMAND_LIST;
    }

    if (strstr(message, "read") != NULL) {
        return COMMAND_READ;
    }

    return INVALID_COMMAND;

}

char* getEquipmentId(char* message) {

    char *message_in = strstr(message, "in");

    if (message_in == NULL) {
        return NULL;
    }

    int size = strlen(message_in) - 4;

    if (size < 0) {
        return NULL;
    }

    char* equipmentId = (char *) malloc(size * sizeof(char));

    strncpy(equipmentId, message_in + 3, size);

    int equipmentIdInt = atoi(equipmentId);

    if (equipmentIdInt < 1 || equipmentIdInt > 4) {
        return NULL;
    }

    return equipmentId;
}


char* getSensorsIds(char* message, int command) {

    int start, end;

    // Get start
    if (command == COMMAND_ADD) {
        start = 11;
    } else if (command == COMMAND_REMOVE) {
        start = 14;
    } else if (command == COMMAND_READ) {
        start = 5;
    } else {
        return NULL;
    }

    // Get end
    char *message_in = strstr(message, " in");
    if (message_in == NULL) {
        return NULL;
    }
    end = message_in - message;

    if (start > end) {
        return NULL;
    }

    // Calcula tamanho
    size_t size = end - start;

    char* sensorsIds = (char *) malloc(size * sizeof(char));
    strncpy(sensorsIds, message + start, size);

    // Verifica se os indexes dos sensores sao validos
    int sensorsCount = getSensorsCount(sensorsIds);
    int* sensorsIndexes = getSensorsIndexes(sensorsIds, sensorsCount);

    if (sensorsCount > 4 || (sensorsCount > 1 && command == COMMAND_REMOVE)) {
        free(sensorsIds);
        return NULL;
    }

    for (int i = 0; i < sensorsCount; i++) {
        if (sensorsIndexes[i] < 1 || sensorsIndexes[i] > 4) {
            free(sensorsIds);
            free(sensorsIndexes);
            return NULL;
        }
    }

    free(sensorsIndexes);
    return sensorsIds;
}

char* execCommand(int clntSocket, char message[]) {

    // Identifica o comando
    int command = getCommand(message);
    
    // Se for invalido retorno NULL
    if (command == INVALID_COMMAND) {
        return NULL;
    }

    // Se for Kill, fecha a conecao e encerra o servidor
    if (command == COMMAND_KILL) {
        close(clntSocket);
        exit(1);
    }

    char* response = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char* equipmentId = getEquipmentId(message);
    char* sensorsIds = getSensorsIds(message, command);

    if (sensorsIds == NULL && command != COMMAND_LIST) {
        strncpy(response, "invalid sensor\n", BUFFER_SIZE);
        return response;
    }

    if (equipmentId == NULL) {
        strncpy(response, "invalid equipment\n", BUFFER_SIZE);
        return response;
    }

    switch (command) {
        case COMMAND_ADD:
            response = addSensor(sensorsIds, equipmentId);
            break;
        case COMMAND_REMOVE:
            response = removeSensor(sensorsIds, equipmentId);
            break;
        case COMMAND_LIST:
            response = listSensors(equipmentId);
            break;
        case COMMAND_READ:
            response = readSensors(sensorsIds, equipmentId);
            break;
        default:
            break;
    }

    free(equipmentId);
    free(sensorsIds);

    return response;
}


//
// Fim dos métodos de protocolo 
// =========================================================================================

// Lida com um cliente especifoco
void handleTCPClient(int clntSocket) {
    char messageRcvd[BUFFER_SIZE]; // Buffer para mensagem recebida

    // Recebe mensagem do cliente
    ssize_t numBytesRcvd = recv(clntSocket, messageRcvd, BUFFER_SIZE, 0);
    if (numBytesRcvd < 0) {
        dieWithSystemMessage("recv() failed");
    }

    // Send received string and receive again until end of stream
    while (numBytesRcvd > 0) {

        printf("%s", messageRcvd);

        char* returnMessage = execCommand(clntSocket, messageRcvd);

        if (returnMessage == NULL) {
            break;
        }

        size_t returnMessageSize = strlen(returnMessage) + 1;

        ssize_t numBytesSent = send(clntSocket, returnMessage, returnMessageSize, 0);

        if (numBytesSent < 0) {
            dieWithSystemMessage("send() failed");
        }

        // See if there is more data to receive
        numBytesRcvd = recv(clntSocket, messageRcvd, BUFFER_SIZE, 0);
        if (numBytesRcvd < 0) {
            dieWithSystemMessage("recv() failed");
        }

        free(returnMessage);
    }

    close(clntSocket); // Encerra a conexao com um cliente
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
    if (listen(servSock, MAXPENDING) < 0) {
        dieWithSystemMessage("listen() failed");
    }

    // Loop principal
    while(1) { 

        // Aceita conexao com um cliente
        int clntSock = connectToClient(servSock);
        
        // Lida com um cliente especifoco
        handleTCPClient(clntSock);
    }
}