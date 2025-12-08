#include <unistd.h>
#include <stdio.h>

#include "wiadomosc.h"

void msg(enum Color color, const char *message) {
    const char *color_code = "\033[0m";

    switch (color) {
        case COL_RED:
            color_code = "\033[91m";
            break;
        case COL_GREEN:
            color_code = "\033[92m";
            break;
        case COL_BLUE:
            color_code = "\033[94m";
            break;
        case COL_YELLOW:
            color_code = "\033[93m";
            break;
        case COL_CYAN:
            color_code = "\033[96m";
            break;
        case COL_MAGENTA:
            color_code = "\033[95m";
            break;
        case COL_DEFAULT:
            color_code = "\033[0m";
            break;
    }

    char formatted_message[1024];
    int len = snprintf(formatted_message, sizeof(formatted_message), "%s%s\033[0m", color_code, message);

    write(STDOUT_FILENO, formatted_message, len);
}
