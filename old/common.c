#include <stdio.h>
#include <stdlib.h>
#include "common.h"

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