#include <unistd.h>
#include "kasa_stacjonarna.h"
#include "shared.h"
#include "wiadomosc.h"

void test(){
    zapisz_wiadomosc(COL_YELLOW, "kasa_stacjonarna - test\n");
};

int main(int argc, char *argv[]) {
    test();
}
