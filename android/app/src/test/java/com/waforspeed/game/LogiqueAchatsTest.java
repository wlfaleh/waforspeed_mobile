// WFS-511 (RG-09) : règles des achats Google Play avec un faux client de facturation : créditer (par le jeu) PUIS consommer, rattrapage
// au lancement, achat en attente, achat annulé, même jeton reçu deux fois, restauration du « sans pub » après une réinstallation, hors
// ligne, journal sans jeton complet (RG-08). Lancé par la CI avec les autres tests JUnit (testReleaseUnitTest).
package com.waforspeed.game;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import org.junit.Before;
import org.junit.Test;

public class LogiqueAchatsTest {
    // jeton au format de Google (long, opaque) : ne doit jamais sortir en entier dans le journal
    private static final String JETON = "abcdefghijklmnopqrstuvwxyz0123456789.AO-J1Oz_jeton-de-test";
    private static final String JETON2 = "zyxwvutsrqponmlkjihgfedcba9876543210.AO-J1Oz_autre-jeton";

    // faux client de facturation : note chaque appel à Google (dans l'ordre)
    private final List<String> appels = new ArrayList<>();
    private final List<String> journal = new ArrayList<>();
    private LogiqueAchats l;

    @Before public void preparer() {
        appels.clear(); journal.clear();
        l = new LogiqueAchats(Arrays.asList("etoiles_bronze", "etoiles_argent", "etoiles_or", "etoiles_coffre"), new LogiqueAchats.Client() {
            @Override public void consommer(String jeton) { appels.add("consommer " + jeton); }
            @Override public void confirmer(String jeton) { appels.add("confirmer " + jeton); }
        }, new LogiqueAchats.Journal() {
            @Override public void ecrire(String t) { journal.add(t); System.out.println("ACHATS " + t); }
        });
    }

    private static LogiqueAchats.Achat achat(String produit, String jeton, int etat, boolean confirme) {
        return new LogiqueAchats.Achat(produit, jeton, etat, confirme);
    }
    private static List<LogiqueAchats.Achat> une(LogiqueAchats.Achat a) { return Collections.singletonList(a); }

    @Test public void crediterPuisConsommer() {
        l.demande("etoiles_or");
        assertFalse(l.miseAJour(LogiqueAchats.OK, une(achat("etoiles_or", JETON, LogiqueAchats.ACHETE, false))));
        // RG-04 : rien n'est consommé avant que le jeu ait crédité (et écrit la sauvegarde)
        assertTrue("aucune consommation avant le crédit du jeu", appels.isEmpty());
        assertArrayEquals(new String[] {"ACHETE", "etoiles_or", JETON}, l.vider());
        // le jeu a crédité : il demande la consommation
        l.confirmer("etoiles_or", JETON);
        assertEquals(Collections.singletonList("consommer " + JETON), appels);
        l.resultat(JETON, true, LogiqueAchats.OK);
        assertEquals(0, l.vider().length);
    }

    @Test public void nonConsommableConfirme() {
        l.demande("sans_pub");
        l.miseAJour(LogiqueAchats.OK, une(achat("sans_pub", JETON, LogiqueAchats.ACHETE, false)));
        assertTrue(appels.isEmpty());
        l.vider();
        l.confirmer("sans_pub", JETON);
        assertEquals(Collections.singletonList("confirmer " + JETON), appels);   // acknowledgePurchase, jamais consommé
    }

    @Test public void enAttentePuisAchete() {
        l.demande("etoiles_coffre");
        l.miseAJour(LogiqueAchats.OK, une(achat("etoiles_coffre", JETON, LogiqueAchats.EN_ATTENTE, false)));
        assertArrayEquals(new String[] {"ATTENTE", "etoiles_coffre", JETON}, l.vider());   // message au joueur, rien crédité
        assertTrue(appels.isEmpty());
        // au rattrapage, un achat encore en attente n'est ni renvoyé ni confirmé
        l.rattrapage(LogiqueAchats.OK, une(achat("etoiles_coffre", JETON, LogiqueAchats.EN_ATTENTE, false)));
        assertEquals(0, l.vider().length);
        // payé plus tard : il revient « acheté »
        l.miseAJour(LogiqueAchats.OK, une(achat("etoiles_coffre", JETON, LogiqueAchats.ACHETE, false)));
        assertArrayEquals(new String[] {"ACHETE", "etoiles_coffre", JETON}, l.vider());
        assertTrue(appels.isEmpty());
    }

    @Test public void annuleParLeJoueur() {
        l.demande("etoiles_argent");
        assertFalse(l.miseAJour(LogiqueAchats.ANNULE_PAR_JOUEUR, null));
        assertArrayEquals(new String[] {"ANNULE", "etoiles_argent", ""}, l.vider());
        assertTrue(appels.isEmpty());
    }

    @Test public void rattrapageAuLancement() {
        // jeu tué après le paiement : Google rend l'achat non consommé au lancement suivant
        l.rattrapage(LogiqueAchats.OK, Arrays.asList(achat("etoiles_bronze", JETON, LogiqueAchats.ACHETE, false),
                                                     achat("pack_debutant", JETON2, LogiqueAchats.ACHETE, false)));
        assertArrayEquals(new String[] {"ACHETE", "etoiles_bronze", JETON, "ACHETE", "pack_debutant", JETON2}, l.vider());
        assertTrue("rien n'est consommé avant le jeu", appels.isEmpty());
        l.confirmer("etoiles_bronze", JETON);
        l.confirmer("pack_debutant", JETON2);
        assertEquals(Arrays.asList("consommer " + JETON, "confirmer " + JETON2), appels);
    }

    @Test public void memeJetonDeuxFois() {
        // Google renvoie le même achat (mise à jour + rattrapage) : le livre du jeu ne crédite qu'une fois ; ici, une seule consommation
        l.miseAJour(LogiqueAchats.OK, une(achat("etoiles_or", JETON, LogiqueAchats.ACHETE, false)));
        l.rattrapage(LogiqueAchats.OK, une(achat("etoiles_or", JETON, LogiqueAchats.ACHETE, false)));
        l.confirmer("etoiles_or", JETON);
        l.confirmer("etoiles_or", JETON);   // 2e « déjà crédité » du jeu : il confirme quand même
        assertEquals(Collections.singletonList("consommer " + JETON), appels);
        l.resultat(JETON, true, LogiqueAchats.OK);
        // encore listé par Google juste après : plus renvoyé au jeu
        l.vider();
        l.rattrapage(LogiqueAchats.OK, une(achat("etoiles_or", JETON, LogiqueAchats.ACHETE, false)));
        assertEquals(0, l.vider().length);
    }

    @Test public void consommationRefuseeRepriseAuRattrapage() {
        l.miseAJour(LogiqueAchats.OK, une(achat("etoiles_or", JETON, LogiqueAchats.ACHETE, false)));
        l.confirmer("etoiles_or", JETON);
        l.resultat(JETON, false, 6);   // erreur de Google (réseau coupé)
        l.vider();
        l.rattrapage(LogiqueAchats.OK, une(achat("etoiles_or", JETON, LogiqueAchats.ACHETE, false)));
        assertArrayEquals(new String[] {"ACHETE", "etoiles_or", JETON}, l.vider());
        l.confirmer("etoiles_or", JETON);
        assertEquals(Arrays.asList("consommer " + JETON, "consommer " + JETON), appels);   // 2e essai permis
    }

    @Test public void restaurationSansPubApresReinstallation() {
        // « sans pub » acheté et confirmé avant la réinstallation : rendu par Google, renvoyé au jeu (livre vide : recrédité), pas reconfirmé
        l.rattrapage(LogiqueAchats.OK, une(achat("sans_pub", JETON, LogiqueAchats.ACHETE, true)));
        assertArrayEquals(new String[] {"ACHETE", "sans_pub", JETON}, l.vider());
        l.confirmer("sans_pub", JETON);
        assertTrue("déjà confirmé chez Google : rien à refaire", appels.isEmpty());
    }

    @Test public void dejaPossedeRelanceLeRattrapage() {
        l.demande("sans_pub");
        assertTrue(l.miseAJour(LogiqueAchats.DEJA_POSSEDE, null));
        assertArrayEquals(new String[] {"ERREUR", "sans_pub", ""}, l.vider());
    }

    @Test public void horsLigne() {
        l.rattrapage(3, null);   // BILLING_UNAVAILABLE : pas de Google Play
        l.produits(new LinkedHashMap<String, String>(), Arrays.asList("etoiles_bronze", "sans_pub"));
        assertEquals("aucun prix : packs masqués", 0, l.vider().length);
        assertTrue(appels.isEmpty());
    }

    @Test public void prixDeGoogle() {
        final Map<String, String> prix = new LinkedHashMap<>();
        prix.put("etoiles_bronze", "0,99 €");
        prix.put("sans_pub", "2,99 €");
        l.produits(prix, Arrays.asList("etoiles_bronze", "etoiles_or", "sans_pub"));
        assertArrayEquals(new String[] {"PRIX", "etoiles_bronze", "0,99 €", "PRIX", "sans_pub", "2,99 €"}, l.vider());
    }

    @Test public void journalSansJetonComplet() {
        l.miseAJour(LogiqueAchats.OK, une(achat("etoiles_or", JETON, LogiqueAchats.ACHETE, false)));
        l.confirmer("etoiles_or", JETON);
        l.resultat(JETON, true, LogiqueAchats.OK);
        l.rattrapage(LogiqueAchats.OK, une(achat("sans_pub", JETON2, LogiqueAchats.EN_ATTENTE, false)));
        assertFalse(journal.isEmpty());
        for (String t : journal) {
            assertFalse("jeton complet dans le journal : " + t, t.contains(JETON) || t.contains(JETON2));
            assertFalse("plus de 8 caractères du jeton : " + t, t.contains(JETON.substring(0, 9)) || t.contains(JETON2.substring(0, 9)));
        }
        assertEquals("abcdefgh…", LogiqueAchats.court(JETON));
    }
}
