#ifndef AWALE_H
#define AWALE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NB_CASES 12

void initPartie(int **plateau, int *score_joueur1, int *score_joueur2, int *sens_rotation);
void afficherPlateau(int *plateau, int joueur);
void copiePlateau(int *plateau1, int *plateau2);
bool estDansCampJoueur(int joueur, int case_parcourue);
bool aDesPionsDansSonCamp(int joueur, int *plateau);
int nbCasesDansLeCamp(int case_selec, int joueur, int *plateau, int sens_rotation);
int testFinPartie(int *plateau, int joueur, int sens_rotation, int score_joueur1, int score_joueur2);
void jouerCoup(int case_jeu, int joueur, int *score_joueur, int sens_rotation, int *plateau);

#endif // AWALE_H
