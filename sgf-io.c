
/************************************************************
 *
 *  FONCTIONS DE LECTURE/ECRITURE DANS UN FICHIER
 *
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "sgf-header.h"
#include "sgf-impl.h"

void sgf_read_block(OFILE *file, int block_number);

/************************************************************
 *  Initialisation du SGF
 ************************************************************/

void init_sgf(void)
{
  init_sgf_fat();
  init_sgf_dir();
}

/************************************************************
 Lire un caractère dans un fichier ouvert. Cette fonction
 renvoie -1 si elle trouve la fin du fichier.

 Description :
   - si la fin du fichier est atteinte (comparaison entre ptr
     et la taille), renvoyer -1
   - si le buffer est vide (test à faire sur ptr % BLOCK_SIZE)
     appeler la fonction sgf_read_block pour le remplir
   - renvoyer le caractère dans la bonne position du buffer
     (position à calculer en fonction de ptr).
 ************************************************************/

int sgf_getc(OFILE *file)
{
  //return sgf_getc_impl(file);
  if (file->ptr >= file->inode.size)
    return -1;
  if (file->ptr % BLOCK_SIZE == 0)
  {
    sgf_read_block(file, file->ptr / BLOCK_SIZE);
  }
  return file->buffer[(file->ptr=file->ptr + 8) % BLOCK_SIZE];

}

/************************************************************
 Déplacer le pointeur associé au fichier ouvert en lecture
 (ptr donc) en utilisant le paramètre pos.

 Description :
   - vérifier le mode d'ouverture du fichier
   - tester le débordement (renvoyer -1 en cas d'échec)
   - si la position n'est pas un multiple de la taille des blocs
      - charger le bon bloc dans le buffer (fonction sgf_read_block)
   - affecter le pointeur courant
   - renvoyer 0

 Attention : prenez soin de limiter la lecture de bloc
 notamment quand la nouvelle position concerne le même bloc
 que la position précédente.
 ************************************************************/

int sgf_seek(OFILE *file, int pos)
{
  //return sgf_seek_impl(file, pos);
  if (file->mode != READ_MODE)
    return -1;
  if (pos > file->inode.size)
    return -1;
  if (pos % BLOCK_SIZE != 0)
  {
    sgf_read_block(file, pos / BLOCK_SIZE);
  }
  file->ptr = pos;
  return 0;
}

/************************************************************
 Lire dans le "buffer" le bloc logique "block_number"
 dans le fichier ouvert "file".

 Description :
   - il faut calculer l'adresse du bloc physique qui contient
     le bloc logique de numéro "block_number"
   - pour ce faire, il faut partir de first (adresse physique
     du premier bloc disponible dans l'INODE) et parcourir
     le chainage de la FAT block_number fois.
   - une fois l'adresse trouvée, il faut lire le bloc
     dans le buffer
 ************************************************************/

void sgf_read_block(OFILE *file, int block_number)
{
  //sgf_read_block_impl(file, block_number);
  int adr = file->inode.first;
  for (int i = 0; i < block_number; i++)
  {
    adr = get_fat(adr);
  }
  read_block(adr, &file->buffer);
}

/************************************************************
 Ajouter le bloc contenu dans le tampon au fichier
 ouvert décrit par "file".

 Description :
 - allouer un bloc libre (fonction alloc_block)
 - écrire le buffer dans ce bloc
 - écrire la valeur EOF dans la FAT pour ce bloc
 - si ce bloc est le premier (test sur first/last),
     - mettre à jour first et last de l'INODE
     - écrire l'INODE sur disque
 - sinon
     - il y a donc déjà un dernier bloc (devenu l'avant dernier)
     - mettre à jour la FAT pour que l'avant dernier bloc
       pointe sur le dernier
     - mettre à jour last de l'INODE
     - écrire l'INODE sur disque
 ************************************************************/

void sgf_append_block(OFILE *file)
{
  sgf_append_block_impl(file);
}

/************************************************************
 Ecrire le caractère "c" dans le fichier ouvert
 décrit par "file".

 Description :
 - il y a toujours de la place dans le buffer (postulat)
 - placer le caractère dans le buffer (en fonction de ptr)
 - si le buffer est plein (test sur ptr % BLOCK_SIZE),
   ajouter le bloc au fichier (sgf_append_block)
 ************************************************************/

void sgf_putc(OFILE *file, char c)
{
  OFILE* file = sgf_open("essai.txt", WRITE_MODE);
  sgf_puts(file, "Ceci est un petit texte qui occupe\n");
  sgf_puts(file, "quelques blocs sur ce disque fictif.\n");
  sgf_puts(file, "Le bloc faisant 128 octets, il faut\n");
  sgf_puts(file, "que je remplisse pour utiliser\n");
  sgf_puts(file, "plusieurs blocs.\n");
  sgf_close(file);
}

/************************************************************
 Écrire la chaîne de caractère "s" dans un fichier ouvert
 en écriture décrit par "file".
 ************************************************************/

void sgf_puts(OFILE *file, char *s)
{
  assert(file->mode == WRITE_MODE);

  for (; (*s != '\0'); s++)
  {
    sgf_putc(file, *s);
  }
}

/************************************************************
 Détruire un fichier.

 Description :
 - lire l'INODE et le libérer (FAT_FREE dans la FAT)
 - parcours le chaînage et libérer tous les blocs
 ************************************************************/

void sgf_remove(int adr_inode)
{
  sgf_remove_impl(adr_inode);
}

/************************************************************
 Créer un inode vide, initialiser l'INODE passé en paramètre
 et renvoyer l'adresse. (renvoie -1 en cas d'erreur)

 Description :
 - allouer un bloc libre (fonction alloc_block)
 - initialiser les champs de l'INODE (0 et FAT_EOF)
 - sauver l'INODE et mettre à jour la FAT
 ************************************************************/

static int create_inode(void)
{
  return create_inode_impl();
}

/************************************************************
 Ouvrir un fichier (NULL si échec).
 ************************************************************/

OFILE *sgf_open(const char *name, MODE mode)
{
  /* Allouer une structure OFILE */
  OFILE *file = malloc(sizeof(OFILE));
  if (file == NULL)
    return (NULL);

  int adr_inode = -1;
  switch (mode)
  {
  case READ_MODE:
    /* Chercher le fichier dans le répertoire */
    adr_inode = find_inode(name);
    if (adr_inode < 0)
      return NULL;
    break;
  case WRITE_MODE:
    /* créer un i-node vide sur le disque */
    adr_inode = create_inode();
    if (adr_inode < 0)
      return NULL;

    /* mettre à jour le répertoire */
    int oldinode = add_inode(name, adr_inode);
    if (oldinode > 0)
      sgf_remove(oldinode);
    break;
  default:
    return NULL;
  }

  /* lire l'INODE */
  INODE inode = read_inode(adr_inode);

  file->inode = inode;
  file->adr_inode = adr_inode;
  file->mode = mode;
  file->ptr = 0;

  return (file);
}

/************************************************************
 Fermer un fichier ouvert.

 Description :
 - si le fichier est ouvert en écriture et que le buffer
   n'est pas vide (ptr % BLOCK_SIZE), ajouter le dernier bloc
   au fichier (fonction sgf_append_block)
 ************************************************************/

void sgf_close(OFILE *file)
{
  sgf_close_impl(file);
}
