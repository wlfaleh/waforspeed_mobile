// WaForSpeed — lignes du panneau des réglages et leurs onglets (WFS-291, WFS-318, épic E8) : table seule, sans rendu,
// testée par tests/test_reglages_onglets.cpp (chaque ligne dans un seul onglet, ordre validé par le PO).
#pragma once

namespace wfs::ecrans {

enum { L_CAMERA = 0, L_QUALITE = 1, L_SON = 2, L_MUSIQUE = 3, L_COMMANDES = 4, L_VIBRATIONS = 5, L_DIRECTION = 6, L_MOUVEMENTS = 7,
       L_BOUTONS = 8, L_GYRO = 9, L_GYROZONE = 10, L_AIDE = 11, L_REGARD = 12, L_FANTOME = 13, N_LIGNES = 14 };

// WFS-291 (RG-01, découpage validé par le PO le 2026-11-08 23:04:56 UTC) : 3 onglets ; chaque ligne dans un seul onglet, ordre fixe
enum { O_AFFICHAGE = 0, O_CONDUITE = 1, O_SON = 2, N_ONGLETS = 3 };
inline const char* const ONGLETS[N_ONGLETS] = {"Affichage", "Conduite", "Son"};
constexpr int MAX_PAR_ONGLET = 7;   // Affichage : 5 lignes, 6 avec « Fantôme du record » (WFS-318, prévu en dernière ligne)
inline constexpr int LIGNES_ONGLET[N_ONGLETS][MAX_PAR_ONGLET] = {
    {L_CAMERA, L_QUALITE, L_REGARD, L_MOUVEMENTS, L_BOUTONS, L_FANTOME, -1},   // WFS-318 : « Fantôme du record » en dernière ligne
    {L_DIRECTION, L_COMMANDES, L_GYRO, L_GYROZONE, L_AIDE, L_VIBRATIONS, -1},
    {L_SON, L_MUSIQUE, -1, -1, -1, -1, -1},
};
inline bool dansOnglet(int l, int o) { for (int i = 0; i < MAX_PAR_ONGLET && LIGNES_ONGLET[o][i] >= 0; i++) if (LIGNES_ONGLET[o][i] == l) return true; return false; }
inline int ongletDe(int l) { for (int o = 0; o < N_ONGLETS; o++) if (dansOnglet(l, o)) return o; return O_AFFICHAGE; }
inline int nLignesOnglet(int o) { int n = 0; while (n < MAX_PAR_ONGLET && LIGNES_ONGLET[o][n] >= 0) n++; return n; }

} // namespace wfs::ecrans
