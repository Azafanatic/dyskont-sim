#ifndef WIADOMOSC_H
#define WIADOMOSC_H

enum Color {
    COL_RED,
    COL_GREEN,
    COL_BLUE,
    COL_YELLOW,
    COL_CYAN,
    COL_MAGENTA,
    COL_DEFAULT
};

void msg(enum Color color, const char *message);

#endif
