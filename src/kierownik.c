#include <unistd.h>
#include "kierownik.h"
#include "wiadomosc.h"

void przywitaj_sie() {
    msg(COL_MAGENTA, "Tu kierownik!\n");
};

int main() {
    przywitaj_sie();
    while (1) {
        sleep(1);
    }
}
