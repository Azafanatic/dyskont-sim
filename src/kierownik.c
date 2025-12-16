#include <unistd.h>
#include "kierownik.h"
#include "wiadomosc.h"
#include "shared.h"

void przywitaj_sie() {
    msg(COL_MAGENTA, "Tu kierownik!\n");
};

int main() {
    przywitaj_sie();
}
