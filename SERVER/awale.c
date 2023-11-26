/**
 * Code du jeu Awalé
 * Servira de base pour l'implémentation en réseau
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "awale.h"

#define NB_CASES 12

/**
 * Initialisation de la partie :
 *  - plateau : 4 grains par case
 *  - sens de rotation aléatoire : 1=horaire / -1 = anti-horaire (cf affichage pour comprendre pourquoi)
 *  - scores des joueurs à 0
 */
void initPartie(int **plateau, int *score_joueur1, int *score_joueur2, int *sens_rotation)
{
    *plateau = (int *)malloc(NB_CASES * sizeof(int));

    // Initialize plateau
    for (int i = 0; i < NB_CASES; ++i)
    {
        (*plateau)[i] = 4; // Or any other initialization logic you need
    }

    *score_joueur1 = 0;
    *score_joueur2 = 0;
    *sens_rotation = (rand() / RAND_MAX < 0.5) ? (-1) : (1);
}

/**
 * Parcours des cases et affichage ASCII
 * Affichage du jeu adapté en fonction du joueur
 *
 * Joueur 1 :
 * 6 7 8 9 10 11
 * 5 4 3 2 1  0
 *
 * Joueur 2 :
 * 0  1  2 3 4 5
 * 11 10 9 8 7 6
 */
void afficherPlateau(int *plateau, int joueur)
{

    int idx_row1, idx_row2;

    // indices des cases par lesquelles commencent les lignes à afficher
    idx_row1 = NB_CASES / 2 * (2 - joueur);
    idx_row2 = NB_CASES / 2 * joueur - 1;

    // affichage ligne par ligne
    printf("Point de vue du joueur %d\n", joueur);
    printf("Plateau de jeu :\n\n");
    for (int i = idx_row1; i < idx_row1 + 6; ++i)
    {
        printf(" | %d", plateau[i]);
    }
    printf(" |\n");
    for (int i = 0; i < NB_CASES / 2; ++i)
    {
        printf("-----");
    }
    printf("\n");
    for (int i = idx_row2; i > idx_row2 - 6; --i)
    {
        printf(" | %d", plateau[i]);
    }
    printf(" |\n\n");
}

/**
 * Réalise une copie de plateau1 dans plateau2
 */
void copiePlateau(int *plateau1, int *plateau2)
{
    for (int i = 0; i < NB_CASES; ++i)
    {
        plateau2[i] = plateau1[i];
    }
}

/**
 * Renvoie 1 si la case est dans le camp du joueur
 * Renvoie 0 sinon
 */
bool estDansCampJoueur(int joueur, int case_parcourue)
{
    return (NB_CASES / 2 * joueur > case_parcourue) && (NB_CASES / 2 * (joueur - 1) <= case_parcourue);
}

/**
 * Renvoie 1 si le joueur a au moins 1 pion dans son camp
 * Renvoie 0 sinon
 */
bool aDesPionsDansSonCamp(int joueur, int *plateau)
{

    int case_parcourue = NB_CASES / 2 * (joueur - 1);
    while (plateau[case_parcourue] == 0 && case_parcourue < NB_CASES / 2 * joueur)
    {
        case_parcourue++;
    }
    return (case_parcourue == NB_CASES / 2 * joueur) ? false : true;
}

/**
 * Renvoie le nombre de cases entre la case passée en paramètre et la fin du camp du joueur (cela dépend
 * du sens de rotation de la partie)
 */
int nbCasesDansLeCamp(int case_selec, int joueur, int *plateau, int sens_rotation)
{
    if (sens_rotation == 1)
    {
        return (NB_CASES / 2 * (joueur)-case_selec);
    }
    else
    {
        return case_selec + 1 - 6 * (joueur - 1);
    }
}

/**
 * Renvoie 1 si le joueur peut nourrir l'autre, càd joueur un coup qui remette des pions dans le camp de l'autre
 * Renvoie 0 sinon
 */
// bool peutNourrir(int joueur, int *plateau, int sens_rotation) {

//     int case_parcourue = NB_CASES/2*(joueur-1);
//     while (plateau[case_parcourue] < nbCasesDansLeCamp(case_parcourue, joueur, plateau, sens_rotation))
//     {
//         case_parcourue++;
//     }
//     return (case_parcourue == NB_CASES/2*joueur) ? false : true;
// }

/**
 * Renvoie 1 si le joueur peut capturer des graines chez son adversaire
 * Renvoie 0 sinon
 * Pour chaque case du jeu du joueur, il faut que la case atteinte en la jouant soit chez l'adversaire et contienne 1 ou 2 graines
 */
// bool possibleCapturerGraines(int joueur, int *plateau, int sens_rotation) {

//     //si on ne peut pas nourrir l'autre, alors on ne peut pas capturer de graines
//     if (!peutNourrir(joueur, plateau, sens_rotation))
//     {
//         return 0;
//     }

//     //on parcourt toutes les cases du jeu du joueur
//     for (int case_parcourue=(NB_CASES/2 * (joueur-1)); case_parcourue<(NB_CASES/2 * joueur); ++case_parcourue) {
//         //si la case d'arrivée contient 1 ou 2 graines, ou peut capturer
//         int idx_arrivee = (case_parcourue + plateau[case_parcourue] * sens_rotation)%NB_CASES;
//         printf("case de départ : %d / case d'arrivée : %d\n", case_parcourue, idx_arrivee);
//         if ((plateau[idx_arrivee] == 1 && plateau[idx_arrivee] == 2)
//             && !estDansCampJoueur(joueur, case_parcourue))
//         {
//             return true;
//         }
//     }

//     return false;
// }

/**
 * Appelée après un coup pour savoir si la partie est terminée, et quel joueur l'emporte
 * Valeurs de retour :
 *  - 0 : la partie continue
 *  - 1 : le joueur 1 gagne
 *  - 2 : le joueur 2 gagne
 *  - 3 : aucun des joueurs ne gagne
 */
int testFinPartie(int *plateau, int joueur, int sens_rotation, int score_joueur1, int score_joueur2)
{

    int stopJeu; // pour les tests qui ont besoin de condition sur les 2 joueurs

    // vérification du score des joueurs
    if (score_joueur1 >= 25)
    {
        return 1;
    }

    if (score_joueur2 >= 25)
    {
        return 2;
    }

    // si un des deux joueurs n'a pas de graines, et que l'autre ne peut pas le nourrir
    if (!aDesPionsDansSonCamp(joueur, plateau))
    {
        printf("La partie se termine : le joueur %d n'a plus de graines dans son camp", joueur);
        return 3;
    }

    // si il n'est plus possible de capturer de graines pour aucun des joueurs
    //  if (!possibleCapturerGraines(1, plateau, sens_rotation) && !possibleCapturerGraines(2, plateau, sens_rotation))
    //  {
    //      printf("La partie se termine : aucun des joueurs ne peut plus capturer de graines");
    //      return 3;
    //  }
    return 0;
}

/**
 * Joue un coup : on enlève les pions de la case, on les répartit 1 par 1 dans les cases suivantes du
 * plateau (selon le sens de rotation)
 * Attention : la case est jouée depuis le pdv du joueur (case n°1 = celle tout à gauche, case n°6 = celle tout à droite)
 *  pas selon les indices du tableauq ui représente le plateau
 */
void jouerCoup(int case_jeu, int joueur, int *score_joueur, int sens_rotation, int *plateau)
{

    int nb_pions;
    int case_parcourue, case_depart;
    int *tmpPlateau;

    // check validité case
    if (case_jeu > 6 || case_jeu < 1)
    {
        printf("Case choisie non valide\n");
        return;
    }

    // conversion case en fonction du joueur : cf formule dans afficherPlateau (adaptée ici)
    case_depart = (NB_CASES / 2 * joueur) - case_jeu;
    case_parcourue = case_depart;

    // déroulé du tour
    nb_pions = plateau[case_parcourue];
    plateau[case_parcourue] = 0;

    printf("case sélec : %d | nb pions = %d\n", case_parcourue, nb_pions);

    // Egrenage le long du plateau
    // Si le coup vide le camp de l'adversaire, il est joué mais les graines ne sont pas prises
    while (nb_pions > 0)
    {
        case_parcourue = (case_parcourue - 1 + NB_CASES) % NB_CASES; // la construction/affichage du plateau nous permet de faire ça facile

        // si plus de 11 graines : on saute la case de départ
        if (case_parcourue == case_depart)
        {
            continue;
        }

        printf("case sélec : %d | nb pions = %d\n", case_parcourue, plateau[case_parcourue]);

        plateau[case_parcourue] = plateau[case_parcourue] + 1;
        nb_pions--;
    }

    // sauvegarde temporaire du plateau de jeu : en cas de non-validité on peut y revenir
    // faite ici parce que on a égrené les pions, et que cette opération se dfait dans tous les cas
    /* tmpPlateau = (int*)malloc(NB_CASES*sizeof(int));
    copiePlateau(plateau, tmpPlateau); */

    // TESTS SAUVEGARDE
    /* afficherPlateau(plateau, 1);
    afficherPlateau(tmpPlateau, 1); */

    // on vérifie si le joueur remporte des pions
    while (!estDansCampJoueur(joueur, case_parcourue) && (plateau[case_parcourue] == 2 || plateau[case_parcourue] == 3))
    {
        printf("Le joueur prend les pions : %d\n", plateau[case_parcourue]);
        *score_joueur += plateau[case_parcourue];
        plateau[case_parcourue] = 0;
        case_parcourue -= -1;

        /* //on vérifie que le coup n'a pas vidé le camp de l'adversaire
        //càd on est sur la dernière case du camp adverse et cette case est à 0

        if ( ((sens_rotation == 1 && case_parcourue == NB_CASES/2 * (2-joueur))
            || (sens_rotation == 0 && case_parcourue == NB_CASES/2 * (3-joueur) - 1))
            && plateau[case_parcourue] == 0)
        {
            copiePlateau(tmpPlateau, plateau);
        } */
    }
    // free(tmpPlateau);
}

/*int main()
{
    int *plateau;
    int score_joueur1, score_joueur2, sens_rotation;

    // initialisation des paramètres de la partie
    plateau = (int *)malloc(NB_CASES * sizeof(int));
    initPartie(plateau, &score_joueur1, &score_joueur2, &sens_rotation);
    afficherPlateau(plateau, 1);
    jouerCoup(2, 1, &score_joueur1, -1, plateau);
    afficherPlateau(plateau, 1);
    afficherPlateau(plateau, 2);
    jouerCoup(2, 2, &score_joueur1, -1, plateau);
    afficherPlateau(plateau, 2);
    afficherPlateau(plateau, 1);
    jouerCoup(6, 1, &score_joueur1, -1, plateau);
    afficherPlateau(plateau, 1);
    afficherPlateau(plateau, 2);
    jouerCoup(5, 2, &score_joueur1, -1, plateau);
    afficherPlateau(plateau, 2);
    afficherPlateau(plateau, 1);
    jouerCoup(5, 1, &score_joueur1, -1, plateau);
    afficherPlateau(plateau, 1);
    free(plateau);
    exit(0);
} */