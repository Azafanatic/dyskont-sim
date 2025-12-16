#include <unistd.h>
#include "klient.h"
#include "wiadomosc.h"
#include "shared.h"

void przywitaj_sie() {
    msg(COL_YELLOW, "Tu klient!\n");
};

int main() {
    przywitaj_sie();
}
