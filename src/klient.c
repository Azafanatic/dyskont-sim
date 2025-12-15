#include <unistd.h>
#include "klient.h"
#include "wiadomosc.h"

void przywitaj_sie() {
    msg(COL_YELLOW, "Tu klient!\n");
};

int main() {
    przywitaj_sie();
    while (1) {
        sleep(1);
    }
}
