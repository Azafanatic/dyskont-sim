#include "klient.h"
#include "wiadomosc.h"

void say_hello() {
    msg(COL_YELLOW, "Tu klient!\n");
};

int main() {
    say_hello();
}
