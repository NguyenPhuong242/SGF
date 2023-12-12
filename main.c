
/************************************************************
 Utilisation du SGF
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "sgf-header.h"


int main() {
    init_sgf();
    
    printf("\nLISTE DES FICHIERS\n\n");
    list_directory();
    
    printf("\nCONTENU DE essai.txt\n\n");
    OFILE *file = sgf_open("essai.txt", READ_MODE);
    for (int c; ((c = sgf_getc(file)) > 0); ) {
        putchar(c);
    }
    sgf_close(file);
    
    return (EXIT_SUCCESS);
}
