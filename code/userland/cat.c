#include "syscall.h"
#include "lib.c"

#define NULL  ((void *) 0)

int main(int argc, char *argv[]) {
  if (argc != 2) {
    ourPuts("Error en el numero de argumentos\n");
    return 1;
  }

  OpenFileId file = Open(argv[1]);
  if (file == -1) {
    ourPuts("Error al abrir el archivo\n");
    return 1;
  }
  
  char caracter[1];
  while (Read(caracter, 1, file) != 0) {
    Write(caracter, 1, CONSOLE_OUTPUT);
  }

  Close(file);
  return 0;
}
