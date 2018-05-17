#include <stdlib.h>
#include <string.h>
#include "parser.h"

/*
 * Parse a data command. Legal commands:
 * PUT key text
 * GET key
 * COUNT
 * DELETE key
 * EXISTS key
 */
int parse_d(char* buf, enum DATA_CMD *cmd, char **key, char **text) {
    const char* commands[] = {"PUT", "GET", "COUNT", "DELETE", "EXISTS", NULL};
    const int args[] =       {2,     1,     0,       1,        1,        -1  }; 

    *key = NULL;
    *text = NULL;

    /* Find the first word. */
    char *s;
    char *end = buf + LINE - 1;

    int nWords = 0;
    for (s = buf; s < end; s++) {
        if (*s >= 'a' && *s <= 'z') { *s += 'A' - 'a'; }
        if (*s == ' ') {
            nWords = 2;
            *s = '\0';
            s++;
            break;
        } else if (*s == '\n' || *s == '\r') {
            nWords = 1;
            *s = '\0';
            s++;
            break;
        } else if (*s == '\0') {
            /* no EOL yet ... overlong line */
            *cmd = D_ERR_OL;
            return 1;
        }
    }
    if (nWords == 0) {
        /* can this even happen? */
        *cmd = D_ERR_OL;
        return 1;
    }

    if (buf[0] == '\0') {
        *cmd = D_END;
        return 0;
    }

    /* buf now holds the first word, s the rest */
    *cmd = D_ERR_INVALID;
    for (int command = 0; commands[command] != NULL; command++) {
        if (!strcmp(commands[command], buf)) {
            *cmd = command;
        }
    }
    if (*cmd == D_ERR_INVALID) { return 2; }
    int nArgs = args[*cmd];
    if (nArgs == 0 && nWords == 1) { return 1; }
    if (nArgs == 0) {
        *cmd = D_ERR_LONG;
        return 1;
    }
    if (nWords == 1) {
        *cmd = D_ERR_SHORT;
        return 1;
    }
    *key = s;
    for (; s < end; s++) {
        if (*s == '\n' || *s == '\r') {
            *s = '\0';
            nWords = 2;
            break;
        } else if (*s == ' ') {
            *s = '\0';
            s++;
            nWords = 3;
            break;
        }
    }
    *text = s;

    for (; s < end; s++) {
        if(*s == '\r' || *s == '\n') {
            *s = '\0';
        }
    }

    if (nWords == 2 && nArgs == 1) {
        *text = NULL;
        return 0;
    }
    if (nWords == 3 && nArgs == 2) {
        return 0;
    }
    if (nWords > nArgs + 1) {
        *key = *text = NULL;
        *cmd = D_ERR_LONG;
        return 1;
    }
    if (nWords < nArgs + 1) {
        *key = *text = NULL;
        *cmd = D_ERR_SHORT;
        return 1;
    }
    /* why are we here ? */
    *key = *text = NULL;
    *cmd = D_ERR_INVALID;
    return 3;
}

enum CONTROL_CMD parse_c(char* buffer) {
    char *s;
    for (s = buffer; s < buffer + LINE; s++) {
        if (*s >= 'a' && *s <= 'z') {
                *s += ('A' - 'a');
        }
        if (*s < 'A' || *s > 'Z') {
            *s = '\0';
            break;
        }
    }
    if (!strcmp(buffer, "SHUTDOWN")) {
        return C_SHUTDOWN;
    } else if (!strcmp(buffer, "COUNT")) {
        return C_COUNT;
    } else {
        return C_ERROR;
    }
}
