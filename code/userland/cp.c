#include "syscall.h"
#include "lib.c"

int main(int argc, char *argv[]) {
  // Verificar que se proporcionen los argumentos necesarios
  if (argc != 3) {
    ourPuts("Error en el numero de argumentos\n");
    return 1;
  }

  // Abrir el archivo de origen
  OpenFileId source = Open(argv[1]);
  if (source == -1) {
    ourPuts("Error al abrir el archivo\n");
    return 1;
  }

  // Create y abrir el archivo de destino
  Create(argv[2]);
  OpenFileId dest = Open(argv[2]);
  if (dest == -1) {
    ourPuts("Error al abrir el archivo\n");
    return 1;
  }

  // Copiar el contenido del archivo de origen al archivo de destino
  char caracter[1];
  while (Read(caracter, 1, source) != 0) {
    Write(caracter, 1, dest);
  }

  // Cerrar los archivos
  Close(source);
  Close(dest);

  return 0;
}