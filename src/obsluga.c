#include <unistd.h>
#include "obsluga.h"
#include "shared.h"
#include "wiadomosc.h"

void test(){
    zapisz_wiadomosc(COL_MAGENTA, "obsluga - test\n");
};

int main(int argc, char *argv[]) {
    test();
}
