// WaForSpeed — règles des achats Google Play (WFS-511, épic E21), sans dépendance à Android ni à la bibliothèque de Google :
// testées sur la JVM (LogiqueAchatsTest, avec un faux client de facturation). Achats.java fait le lien avec BillingClient.
//
// Règles (regles-de-gestion/WFS-511.md, RC-E21-06) :
//   - RG-04 : un achat « acheté » est ENVOYÉ au jeu (file d'événements lue par le pont JNI) ; il n'est JAMAIS consommé ni confirmé ici
//     de lui-même : c'est le jeu qui appelle confirmer() APRÈS avoir écrit crédit + jeton dans la sauvegarde (market::brancher) ;
//     « en attente » : envoyé pour le message, rien crédité, rien confirmé ; « annulé par le joueur » : retour au market, rien ;
//   - RG-05, RG-06 : rattrapage et restauration : chaque achat « acheté » rendu par Google repasse par le jeu (le livre des achats évite
//     le double crédit) ; un non consommable déjà confirmé (« sans pub » possédé) est renvoyé pour être recrédité après une réinstallation,
//     sans nouvelle confirmation ;
//   - une même consommation / confirmation n'est jamais demandée deux fois en même temps (même jeton reçu deux fois) ; un échec la rend
//     de nouveau possible (reprise au rattrapage suivant) ;
//   - RG-08 : jamais de jeton complet dans le journal (8 premiers caractères).
// Appelée depuis le fil de l'interface (rappels de Google) et depuis le fil du jeu (JNI) : méthodes synchronisées.
package com.waforspeed.game;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

final class LogiqueAchats {
    // état d'un achat (valeurs propres : Achats.java traduit Purchase.PurchaseState)
    static final int AUTRE = 0, ACHETE = 1, EN_ATTENTE = 2;
    // codes de réponse de Google utilisés ici (mêmes valeurs que BillingClient.BillingResponseCode)
    static final int OK = 0, ANNULE_PAR_JOUEUR = 1, DEJA_POSSEDE = 7;
    // types d'événements lus par le jeu (src/platform/achats_android.cpp), par triplets { type, produit, valeur }
    static final String EV_ACHETE = "ACHETE", EV_ATTENTE = "ATTENTE", EV_ANNULE = "ANNULE", EV_ERREUR = "ERREUR", EV_PRIX = "PRIX";

    static final class Achat {
        final String produit, jeton;
        final int etat;
        final boolean confirme;   // isAcknowledged (Google)
        Achat(String produit, String jeton, int etat, boolean confirme) {
            this.produit = produit; this.jeton = jeton; this.etat = etat; this.confirme = confirme;
        }
    }
    // client de facturation : Google (Achats.java) ou faux client des tests ; la réponse revient par resultat()
    interface Client {
        void consommer(String jeton);   // consumeAsync (packs d'étoiles)
        void confirmer(String jeton);   // acknowledgePurchase (pack débutant, sans pub)
    }
    interface Journal { void ecrire(String t); }

    private final Set<String> consommables = new HashSet<>();
    private final Client client;
    private final Journal journal;
    private final List<String> evenements = new ArrayList<>();
    private final Set<String> enCours = new HashSet<>();       // jetons dont la consommation / confirmation est partie
    private final Set<String> termines = new HashSet<>();      // consommés / confirmés pendant ce lancement
    private final Map<String, Boolean> dejaConfirmes = new HashMap<>();   // jeton → isAcknowledged au dernier passage
    private String dernierDemande = "";

    LogiqueAchats(Collection<String> consommables, Client client, Journal journal) {
        this.consommables.addAll(consommables);
        this.client = client;
        this.journal = journal;
    }

    // RG-08 : 8 premiers caractères au plus
    static String court(String jeton) {
        if (jeton == null) return "";
        return jeton.length() <= 8 ? jeton : jeton.substring(0, 8) + "…";
    }

    // RG-03 : prix de Google (formattedPrice) ; produit introuvable : aucun prix, le market masque le pack (WFS-513)
    synchronized void produits(Map<String, String> prix, Collection<String> demandes) {
        for (String id : demandes) {
            final String p = prix.get(id);
            if (p == null || p.isEmpty()) { journal.ecrire("produit introuvable chez Google : " + id + " (pack masqué)"); continue; }
            ajouter(EV_PRIX, id, p);
        }
        journal.ecrire("produits : " + prix.size() + " trouvé(s) sur " + demandes.size() + " demandé(s)");
    }

    synchronized void demande(String produit) { dernierDemande = produit == null ? "" : produit; }

    // RG-04 : onPurchasesUpdated ; rend vrai quand Google dit « déjà possédé » : l'appelant relance le rattrapage (achat pas encore confirmé)
    synchronized boolean miseAJour(int code, List<Achat> achats) {
        if (code == OK) {
            if (achats != null) for (Achat a : achats) signaler(a, true);
            return false;
        }
        if (code == ANNULE_PAR_JOUEUR) {
            journal.ecrire("achat annulé par le joueur : " + dernierDemande + " (rien de crédité)");
            ajouter(EV_ANNULE, dernierDemande, "");
            return false;
        }
        journal.ecrire("achat en erreur (code " + code + ") : " + dernierDemande + " (rien de crédité)");
        ajouter(EV_ERREUR, dernierDemande, "");
        return code == DEJA_POSSEDE;
    }

    // RG-05, RG-06 : queryPurchasesAsync(INAPP) ; hors ligne / sans Google (code ≠ OK) : rien, le jeu continue (RG-07)
    synchronized void rattrapage(int code, List<Achat> achats) {
        if (code != OK) { journal.ecrire("rattrapage impossible (code " + code + ") : nouvel essai au prochain retour au jeu"); return; }
        int n = 0;
        if (achats != null) for (Achat a : achats) if (a.etat == ACHETE && signaler(a, false)) n++;
        journal.ecrire("rattrapage : " + n + " achat(s) renvoyé(s) au jeu");
    }

    // appelé par le jeu APRÈS l'écriture de la sauvegarde (market::brancher → Boutique::confirmer)
    synchronized void confirmer(String produit, String jeton) {
        if (jeton == null || jeton.isEmpty()) return;
        if (enCours.contains(jeton) || termines.contains(jeton)) {
            journal.ecrire(produit + " " + court(jeton) + " : confirmation déjà faite ou en cours (rien de plus)");
            return;
        }
        if (consommables.contains(produit)) {
            enCours.add(jeton);
            journal.ecrire(produit + " " + court(jeton) + " : crédité par le jeu, consommation demandée");
            client.consommer(jeton);
        } else if (Boolean.TRUE.equals(dejaConfirmes.get(jeton))) {
            termines.add(jeton);
            journal.ecrire(produit + " " + court(jeton) + " : déjà confirmé chez Google (restauration)");
        } else {
            enCours.add(jeton);
            journal.ecrire(produit + " " + court(jeton) + " : crédité par le jeu, confirmation demandée");
            client.confirmer(jeton);
        }
    }

    // réponse de Google à consommer / confirmer
    synchronized void resultat(String jeton, boolean ok, int code) {
        enCours.remove(jeton);
        if (ok) { termines.add(jeton); dejaConfirmes.put(jeton, true); journal.ecrire(court(jeton) + " : consommé / confirmé chez Google"); }
        else journal.ecrire(court(jeton) + " : consommation / confirmation refusée (code " + code + "), reprise au prochain rattrapage");
    }

    // événements pour le jeu, vidés à chaque lecture (triplets type, produit, valeur)
    synchronized String[] vider() {
        final String[] t = evenements.toArray(new String[0]);
        evenements.clear();
        return t;
    }

    // rend vrai si l'achat est envoyé au jeu
    private boolean signaler(Achat a, boolean achatEnDirect) {
        if (a.produit == null || a.jeton == null) return false;
        if (a.etat == EN_ATTENTE) {
            journal.ecrire(a.produit + " " + court(a.jeton) + " : en attente de paiement (rien de crédité)");
            if (achatEnDirect) ajouter(EV_ATTENTE, a.produit, a.jeton);   // message au joueur seulement pour son achat, pas au rattrapage
            return false;
        }
        if (a.etat != ACHETE) return false;
        // déjà consommé pendant ce lancement : Google peut encore le lister un instant, inutile de le renvoyer
        if (termines.contains(a.jeton) && consommables.contains(a.produit)) return false;
        dejaConfirmes.put(a.jeton, a.confirme);
        journal.ecrire(a.produit + " " + court(a.jeton) + " : acheté, envoyé au jeu" + (a.confirme ? " (déjà confirmé)" : ""));
        ajouter(EV_ACHETE, a.produit, a.jeton);
        return true;
    }

    private void ajouter(String type, String produit, String valeur) {
        evenements.add(type); evenements.add(produit); evenements.add(valeur);
    }
}
