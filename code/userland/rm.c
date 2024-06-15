#include "syscall.h"
#include "lib.c"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    ourPuts("Error en el numero de argumentos\n");
    return 0;
  }
  
  if (Remove(argv[1]) != 1) {
    ourPuts("No se pudo eliminar el archivo.\n");
  }
  return 0;
}
