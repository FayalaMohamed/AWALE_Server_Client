#ifndef AWALE_H
#define AWALE_H

#include <stdbool.h>
#include <stdint.h>

#define NB_CASES 12

void initPartie(uint8_t **plateau, uint8_t *score_joueur1, uint8_t *score_joueur2, uint8_t *sens_rotation);

void afficherPlateau(uint8_t *plateau, uint8_t joueur);

uint8_t testFinPartie(uint8_t *plateau, uint8_t joueur, uint8_t sens_rotation, uint8_t score_joueur1, uint8_t score_joueur2);

bool obligerNourrir(uint8_t joueur, uint8_t *plateau, uint8_t sens_rotation, uint8_t *coup, uint8_t *indexCoup);

void jouerCoup(uint8_t case_jeu, uint8_t joueur, uint8_t *score_joueur, uint8_t sens_rotation, uint8_t **plateau, char *buffer);

#endif