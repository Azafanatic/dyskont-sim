#include <unistd.h>
#include <sys/wait.h>   // waitpid
#include <unistd.h>     // usleep, pause, fork
#include <sys/types.h>  // pid_t
#include "kierownik.h"

int main(int argc, char *argv[]) {

    // TODO: dodać parsowanie argumentów

    say_hello();
    return 0;
}
