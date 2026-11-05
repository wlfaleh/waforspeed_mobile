// WaForSpeed — un doigt = un appui (WFS-113, épic E5) : filtre des touchers pour les boutons de l'interface.
// Sur Android, chaque mouvement d'un doigt posé arrive comme un toucher « appuyé » (AMOTION_EVENT_ACTION_MOVE →
// Touch{down = true}, src/platform/android_main.cpp). Un bouton qui agit « à l'appui » agissait donc à chaque
// micro-mouvement du doigt : au showroom, une flèche faisait sauter plusieurs voitures. Ce filtre retient le doigt
// qui a déclenché un bouton : ses mouvements suivants sont ignorés jusqu'à ce qu'il se relève.
// Calcul seul (aucune dépendance) : testé par tests/test_appui.cpp.
#pragma once

namespace wfs::ui {

class FiltreAppui {
public:
    static constexpr int MAX = 10;   // doigts suivis en même temps
    // à appeler pour chaque toucher, avant les boutons : true = toucher d'un doigt déjà « consommé » par un bouton
    // (mouvement ou relâcher), à ignorer ; le relâcher libère le doigt
    bool consomme(int id, bool down) {
        for (int i = 0; i < n_; i++) if (doigts_[i] == id) {
            if (!down) doigts_[i] = doigts_[--n_];
            return true;
        }
        return false;
    }
    // un bouton vient d'agir pour ce doigt : ses touchers suivants sont ignorés jusqu'au relâcher
    void prendre(int id) { if (n_ < MAX) doigts_[n_++] = id; }
    void vider() { n_ = 0; }
    int nombre() const { return n_; }
private:
    int doigts_[MAX] = {};
    int n_ = 0;
};

} // namespace wfs::ui
