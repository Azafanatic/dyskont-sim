#include <unistd.h>
#include "kasa_samoobslugowa.h"
#include "shared.h"
#include "wiadomosc.h"

void test(){
    zapisz_wiadomosc(COL_CYAN, "kasa_samoobslugowa - test\n");
};

int main(int argc, char *argv[]) {
    test();
}
