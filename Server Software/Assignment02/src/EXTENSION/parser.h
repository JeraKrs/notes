#ifndef _parser_h_
#define _parser_h_

#define LINE 255
enum DATA_CMD    { D_PUT = 0, D_GET, D_COUNT, D_DELETE, D_EXISTS, D_END,
                   D_ERR_OL = 100, D_ERR_INVALID, D_ERR_SHORT, D_ERR_LONG };

int parse_d(char* buf, enum DATA_CMD *cmd, char **key, char **text);

enum CONTROL_CMD { C_SHUTDOWN, C_COUNT, C_ERROR };

enum CONTROL_CMD parse_c(char* buffer);

#endif

