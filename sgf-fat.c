
/* ============================================================================
 _   _ _____   ____   _    ____    __  __  ___  ____ ___ _____ ___ _____ ____  
| \ | | ____| |  _ \ / \  / ___|  |  \/  |/ _ \|  _ \_ _|  ___|_ _| ____|  _ \ 
|  \| |  _|   | |_) / _ \ \___ \  | |\/| | | | | | | | || |_   | ||  _| | |_) |
| |\  | |___  |  __/ ___ \ ___) | | |  | | |_| | |_| | ||  _|  | || |___|  _ < 
|_| \_|_____| |_| /_/   \_\____/  |_|  |_|\___/|____/___|_|   |___|_____|_| \_\
                                                                                                                                   
============================================================================ */

/************************************************************
 *
 *  FONCTIONS DE MANIPULATION DE LA FAT (File Allocation Table)
 *
 ************************************************************/


#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sgf-header.h"


#define PAR_EXCES(n,d)          (((n) + (d) - 1) / (d))


/************************************************************
 *  Définition de la FAT en mémoire centrale.
 ************************************************************/

typedef struct {
    int    in_memory;       /* la FAT est-elle en mémoire ?     */
    int    size_in_blocks;  /* taille de la FAT en blocs        */
    int    disk_size;       /* taille du disque en blocs        */
    int*   tab;             /* la FAT en mémoire                */
    BLOCK* blocks;          /* la FAT vu comme un tab. de blocs */
    int*   dirty;           /* pour chaque bloc un bit de modif */
} FAT;

static FAT  fat = {0, 0, 0, NULL, NULL, NULL};


/************************************************************
 *  Création d'une FAT vide en mémoire
 ************************************************************/

static void create_memory_fat (void) {
    if (fat.tab != NULL) {
        free(fat.tab);
        free(fat.dirty);
        fat.in_memory = 0;
    }
    
    fat.disk_size = get_disk_size();
    int fat_size_in_bytes = (fat.disk_size * sizeof(int));
    fat.size_in_blocks = PAR_EXCES(fat_size_in_bytes, BLOCK_SIZE);
    fat_size_in_bytes = (fat.size_in_blocks * BLOCK_SIZE);
    
    fat.tab = malloc(fat_size_in_bytes);
    fat.blocks = (BLOCK*) fat.tab;
    fat.dirty = malloc(fat.size_in_blocks * sizeof(int));
    if (fat.tab == NULL || fat.dirty == NULL) {
        panic("impossible d'allouer la FAT en mémoire.");
    }
    
    /* au début tous les blocs sont libres */
    for(int i=0; (i < fat.disk_size); i++) {
        fat.tab[i] = FAT_FREE;
    }

    /* le bloc physique 0 est réservé pour le bloc de définition */
    fat.tab[ADR_DIR_BLOCK] = FAT_RESERVED;

    /* les blocs de la FAT sont réservés */
    for(int k = 0; (k < fat.size_in_blocks); k++) {
        fat.tab[k + ADR_FAT_BLOCK] = FAT_RESERVED;
        fat.dirty[k] = 1;
    }
}


/************************************************************
 *  Chargement de la FAT en mémoire centrale.
 ************************************************************/

void init_sgf_fat (void) {
    create_memory_fat();
    
    for(int k = 0; (k < fat.size_in_blocks); k++) {
        read_block(k + ADR_FAT_BLOCK, & fat.blocks[k]);
        fat.dirty[k] = 0;
    }
    
    fat.in_memory = 1;
}


/************************************************************
 *  Sauvegarde des blocs modifiés de la FAT sur disque
 ************************************************************/

static void save_fat (void) {
    for(int k = 0; (k < fat.size_in_blocks); k++) {
        if (fat.dirty[k]) {
            write_block(k + ADR_FAT_BLOCK, & fat.blocks[k]);
            fat.dirty[k] = 0;
        }
    }
}


/************************************************************
 *  Lire une entrée de la FAT
 ************************************************************/

int get_fat (int n) {
    if (n < 0  ||  n >= fat.disk_size)
        panic("Utilisation de <<get_fat>> incorrecte.");
    
    return fat.tab[ n ];
}


/************************************************************
 *  Changer la valeur d'une entrée de la FAT et la sauver
 *  sur disque.
 ************************************************************/

void set_fat (int n, int valeur) {
    if (n < 0  ||  n >= fat.disk_size)
        panic("Utilisation de <<set_fat>> incorrecte.");
    
    assert(
        ((valeur) == FAT_FREE) ||
        ((valeur) == FAT_INODE) ||
        ((valeur) == FAT_EOF) ||
        ((valeur) >= 0 && (valeur) < fat.disk_size)
    );
    
    fat.tab[ n ] = valeur;
    fat.dirty[ n / (BLOCK_SIZE / sizeof(int)) ] = 1;
    save_fat ();
}


/************************************************************
 *  Rechercher un bloc physique libre en parcourant la FAT.
 *  (en cas d'erreur cette fonction renvoie -1).
 ************************************************************/

int alloc_block (void) {
    for(int k = 0; k < fat.disk_size; k++)
        if (fat.tab[k] == FAT_FREE)
            return (k);

    return (-1);
}


/************************************************************
 *  Initialiser le disque avec une FAT vide.
 ************************************************************/

void create_empty_fat () {
    DIRECTORY dir_block;
    
    int disk_size = get_disk_size();
    int fat_size_in_bytes  = (disk_size * sizeof(int));
    int fat_size_in_blocks = PAR_EXCES(fat_size_in_bytes, BLOCK_SIZE);
    fat_size_in_bytes  = (fat_size_in_blocks * BLOCK_SIZE);
    
    int *tab = malloc(fat_size_in_bytes);
    BLOCK *blocks = (BLOCK*) tab;
    if (tab == NULL) {
        panic("FAT: create_empty_fat: impossible d'allouer la FAT en mémoire.");
    }
    
    /* le bloc physique 0 est réservé pour le bloc répertoire */
    /* ------------------------------------------------------ */

    tab[ ADR_DIR_BLOCK ] = FAT_RESERVED;

    for(int k = 0; (k < fat_size_in_blocks); k++) {
        tab[k + ADR_FAT_BLOCK] = FAT_RESERVED;
    }
    
    int first_free = (fat_size_in_blocks + ADR_FAT_BLOCK);
    int last_free = (disk_size - 1);
    for(int k = first_free; (k <= last_free); k++) {
        tab[k] = FAT_FREE;
    }
    
    /* Ecrire la FAT sur le disque */
    /* --------------------------- */

    for(int k = 0; (k < fat_size_in_blocks); k++) {
        write_block(k + ADR_FAT_BLOCK, & blocks[k]);
    }

    /* Préparer et écrire le bloc répertoire sur le disque */
    /* --------------------------------------------------- */
    
    memset(&dir_block, 0, sizeof(dir_block));
    dir_block.signature = SIGNATURE_FS;
    write_dir_block(& dir_block);
    
    /* Liberer la FAT en mémoire */
    /* ------------------------- */
    
    printf("Ecriture d'une FAT vide (block %d to %d)\n", first_free, last_free);

    free(tab);
}

