// WaForSpeed — achats Google Play côté Android (WFS-511, épic E21) : bibliothèque officielle Google Play Billing (RG-01), lue par le jeu
// via JNI (src/platform/achats_android.cpp, modèle de Paquets.java / paquets_android.cpp). Les règles (ordre « créditer puis consommer »,
// rattrapage, en attente, annulé, journal sans jeton complet) sont dans LogiqueAchats.java, testée sur la JVM ; ici, seulement le lien
// avec BillingClient.
//   - RG-02 : connexion au démarrage du jeu (sans bloquer le chargement : tout est asynchrone), reconnexion automatique avec attente
//     croissante (1 s, 2 s, 4 s… 60 s au plus) quand le service se coupe ;
//   - RG-03 : queryProductDetailsAsync (INAPP) sur les identifiants de market.packs ; prix = formattedPrice de Google ;
//   - RG-04 : launchBillingFlow ; onPurchasesUpdated → LogiqueAchats ; consumeAsync / acknowledgePurchase seulement quand le jeu l'a
//     demandé (après l'écriture de la sauvegarde) ;
//   - RG-05, RG-06 : queryPurchasesAsync (INAPP) à la connexion, à chaque retour au premier plan (JeuActivity.onResume) et sur
//     « Restaurer mes achats » ;
//   - RG-07 : sans réseau ou sans Google Play, chaque appel échoue sans exception (le jeu continue, « sans pub » lu dans la sauvegarde).
// Appels Google sur le fil de l'interface (Handler du fil principal) ; les rappels de Google y arrivent aussi.
package com.waforspeed.game;

import android.app.Activity;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import com.android.billingclient.api.AcknowledgePurchaseParams;
import com.android.billingclient.api.AcknowledgePurchaseResponseListener;
import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingFlowParams;
import com.android.billingclient.api.BillingResult;
import com.android.billingclient.api.ConsumeParams;
import com.android.billingclient.api.ConsumeResponseListener;
import com.android.billingclient.api.PendingPurchasesParams;
import com.android.billingclient.api.ProductDetails;
import com.android.billingclient.api.ProductDetailsResponseListener;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesResponseListener;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.android.billingclient.api.QueryProductDetailsParams;
import com.android.billingclient.api.QueryProductDetailsResult;
import com.android.billingclient.api.QueryPurchasesParams;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public final class Achats {
    private Achats() {}

    private static final String TAG = "WaForSpeed";
    private static final Handler PRINCIPAL = new Handler(Looper.getMainLooper());
    private static final LogiqueAchats.Journal JOURNAL = new LogiqueAchats.Journal() {
        @Override public void ecrire(String t) { Log.i(TAG, "ACHATS " + t); }
    };

    private static volatile LogiqueAchats logique;
    private static volatile BillingClient client;
    private static volatile boolean connecte, connexionEnCours;
    private static volatile WeakReference<Activity> activite = new WeakReference<>(null);
    private static volatile List<String> produitsDemandes = new ArrayList<>();
    private static final Map<String, ProductDetails> details = new ConcurrentHashMap<>();
    private static long attenteMs = 1000;
    // achats demandés avant le démarrage de la boutique : réponse « erreur » rendue au jeu (l'écran ne reste pas en attente)
    private static final List<String> erreursAvantDemarrage = new ArrayList<>();

    // ---- appelés par le jeu (JNI, fil du jeu)

    // RG-02 : une seule fois, après la lecture de la sauvegarde ; produits et consommables séparés par des virgules
    public static void demarrer(final Activity act, final String produits, final String consommables) {
        if (act != null) activite = new WeakReference<>(act);
        final Context ctx = act == null ? null : act.getApplicationContext();
        PRINCIPAL.post(new Runnable() {
            @Override public void run() {
                if (client != null || ctx == null) return;
                produitsDemandes = liste(produits);
                logique = new LogiqueAchats(liste(consommables), new LogiqueAchats.Client() {
                    @Override public void consommer(String jeton) { consommerGoogle(jeton); }
                    @Override public void confirmer(String jeton) { confirmerGoogle(jeton); }
                }, JOURNAL);
                try {
                    client = BillingClient.newBuilder(ctx)
                        .setListener(new PurchasesUpdatedListener() {
                            @Override public void onPurchasesUpdated(BillingResult r, List<Purchase> achats) {
                                if (logique.miseAJour(r.getResponseCode(), convertir(achats))) rattraper();
                            }
                        })
                        .enablePendingPurchases(PendingPurchasesParams.newBuilder().enableOneTimeProducts().build())
                        .build();
                } catch (RuntimeException e) {   // RG-07 : bibliothèque ou service absent : le jeu continue sans achats
                    JOURNAL.ecrire("service de facturation indisponible : " + e.getClass().getSimpleName());
                    client = null;
                    return;
                }
                JOURNAL.ecrire("démarrage : " + produitsDemandes.size() + " produit(s) : " + produitsDemandes);
                connecter();
            }
        });
    }

    // RG-03 de nouveau (ouverture du market) + rattrapage
    public static void charger() {
        PRINCIPAL.post(new Runnable() { @Override public void run() { if (pret()) { chargerProduits(); rattraper(); } else connecter(); } });
    }

    // RG-04
    public static void acheter(final String produit) {
        PRINCIPAL.post(new Runnable() {
            @Override public void run() {
                if (logique == null) {
                    JOURNAL.ecrire("achat demandé avant le démarrage du service : " + produit + " (rien de crédité)");
                    synchronized (erreursAvantDemarrage) { erreursAvantDemarrage.addAll(Arrays.asList(LogiqueAchats.EV_ERREUR, produit, "")); }
                    return;
                }
                logique.demande(produit);
                final ProductDetails d = details.get(produit);
                final Activity a = activite.get();
                if (!pret() || d == null || a == null) {   // hors ligne, produit introuvable : rien de crédité (RG-07)
                    logique.miseAJour(-1, null);
                    if (!pret()) connecter();
                    return;
                }
                final List<BillingFlowParams.ProductDetailsParams> p = new ArrayList<>();
                p.add(BillingFlowParams.ProductDetailsParams.newBuilder().setProductDetails(d).build());
                final BillingResult r = client.launchBillingFlow(a, BillingFlowParams.newBuilder().setProductDetailsParamsList(p).build());
                JOURNAL.ecrire("achat demandé : " + produit + " (code " + r.getResponseCode() + ")");
                if (r.getResponseCode() != BillingClient.BillingResponseCode.OK && logique.miseAJour(r.getResponseCode(), null)) rattraper();
            }
        });
    }

    // RG-06 : « Restaurer mes achats » (même requête que le rattrapage)
    public static void restaurer() {
        PRINCIPAL.post(new Runnable() { @Override public void run() { JOURNAL.ecrire("restauration demandée"); if (pret()) rattraper(); else connecter(); } });
    }

    // RG-04 : le jeu a écrit crédit + jeton dans la sauvegarde
    public static void confirmer(final String produit, final String jeton) {
        PRINCIPAL.post(new Runnable() { @Override public void run() { if (logique != null) logique.confirmer(produit, jeton); } });
    }

    // événements pour le jeu (triplets type, produit, valeur) ; tableau vide s'il n'y a rien
    public static String[] evenements() {
        final List<String> t = new ArrayList<>();
        synchronized (erreursAvantDemarrage) { t.addAll(erreursAvantDemarrage); erreursAvantDemarrage.clear(); }
        final LogiqueAchats l = logique;
        if (l != null) t.addAll(Arrays.asList(l.vider()));
        return t.toArray(new String[0]);
    }

    // ---- appelé par JeuActivity.onResume (fil de l'interface) : RG-05, rattrapage à chaque retour au premier plan
    static void premierPlan(Activity act) {
        activite = new WeakReference<>(act);
        if (pret()) rattraper();
    }

    // ---- lien avec Google (fil de l'interface)

    private static boolean pret() { return client != null && connecte; }

    private static void connecter() {
        if (client == null || connecte || connexionEnCours) return;
        connexionEnCours = true;
        try {
            client.startConnection(new BillingClientStateListener() {
                @Override public void onBillingSetupFinished(final BillingResult r) {
                    PRINCIPAL.post(new Runnable() {
                        @Override public void run() {
                            connexionEnCours = false;
                            if (r.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                                connecte = true; attenteMs = 1000;
                                JOURNAL.ecrire("connecté au service Google Play");
                                chargerProduits();
                                rattraper();
                            } else {
                                JOURNAL.ecrire("connexion refusée (code " + r.getResponseCode() + ") : nouvel essai dans " + attenteMs / 1000 + " s");
                                reconnecterPlusTard();
                            }
                        }
                    });
                }
                @Override public void onBillingServiceDisconnected() {
                    PRINCIPAL.post(new Runnable() {
                        @Override public void run() {
                            connecte = false; connexionEnCours = false;
                            JOURNAL.ecrire("service Google Play coupé : reconnexion dans " + attenteMs / 1000 + " s");
                            reconnecterPlusTard();
                        }
                    });
                }
            });
        } catch (RuntimeException e) {
            connexionEnCours = false;
            JOURNAL.ecrire("connexion impossible : " + e.getClass().getSimpleName());
            reconnecterPlusTard();
        }
    }

    private static void reconnecterPlusTard() {
        PRINCIPAL.postDelayed(new Runnable() { @Override public void run() { connecter(); } }, attenteMs);
        attenteMs = Math.min(attenteMs * 2, 60000);
    }

    private static void chargerProduits() {
        final List<QueryProductDetailsParams.Product> liste = new ArrayList<>();
        for (String id : produitsDemandes)
            liste.add(QueryProductDetailsParams.Product.newBuilder().setProductId(id).setProductType(BillingClient.ProductType.INAPP).build());
        if (liste.isEmpty()) return;
        JOURNAL.ecrire("produits demandés à Google : " + produitsDemandes);
        client.queryProductDetailsAsync(QueryProductDetailsParams.newBuilder().setProductList(liste).build(), new ProductDetailsResponseListener() {
            @Override public void onProductDetailsResponse(BillingResult r, QueryProductDetailsResult res) {
                final Map<String, String> prix = new HashMap<>();
                if (r.getResponseCode() == BillingClient.BillingResponseCode.OK && res != null) {
                    for (ProductDetails d : res.getProductDetailsList()) {
                        final ProductDetails.OneTimePurchaseOfferDetails o = d.getOneTimePurchaseOfferDetails();
                        if (o == null) continue;
                        details.put(d.getProductId(), d);
                        prix.put(d.getProductId(), o.getFormattedPrice());
                    }
                } else JOURNAL.ecrire("produits non reçus (code " + r.getResponseCode() + ")");
                logique.produits(prix, produitsDemandes);
            }
        });
    }

    private static void rattraper() {
        if (!pret()) return;
        client.queryPurchasesAsync(QueryPurchasesParams.newBuilder().setProductType(BillingClient.ProductType.INAPP).build(), new PurchasesResponseListener() {
            @Override public void onQueryPurchasesResponse(BillingResult r, List<Purchase> achats) { logique.rattrapage(r.getResponseCode(), convertir(achats)); }
        });
    }

    private static void consommerGoogle(final String jeton) {
        if (!pret()) { logique.resultat(jeton, false, -1); return; }
        client.consumeAsync(ConsumeParams.newBuilder().setPurchaseToken(jeton).build(), new ConsumeResponseListener() {
            @Override public void onConsumeResponse(BillingResult r, String t) {
                logique.resultat(jeton, r.getResponseCode() == BillingClient.BillingResponseCode.OK, r.getResponseCode());
            }
        });
    }

    private static void confirmerGoogle(final String jeton) {
        if (!pret()) { logique.resultat(jeton, false, -1); return; }
        client.acknowledgePurchase(AcknowledgePurchaseParams.newBuilder().setPurchaseToken(jeton).build(), new AcknowledgePurchaseResponseListener() {
            @Override public void onAcknowledgePurchaseResponse(BillingResult r) {
                logique.resultat(jeton, r.getResponseCode() == BillingClient.BillingResponseCode.OK, r.getResponseCode());
            }
        });
    }

    // un achat Google peut porter plusieurs produits (quantité) : un événement par produit
    private static List<LogiqueAchats.Achat> convertir(List<Purchase> achats) {
        final List<LogiqueAchats.Achat> l = new ArrayList<>();
        if (achats == null) return l;
        for (Purchase p : achats) {
            final int etat = p.getPurchaseState() == Purchase.PurchaseState.PURCHASED ? LogiqueAchats.ACHETE
                           : p.getPurchaseState() == Purchase.PurchaseState.PENDING ? LogiqueAchats.EN_ATTENTE : LogiqueAchats.AUTRE;
            for (String produit : p.getProducts()) l.add(new LogiqueAchats.Achat(produit, p.getPurchaseToken(), etat, p.isAcknowledged()));
        }
        return l;
    }

    private static List<String> liste(String s) {
        final List<String> l = new ArrayList<>();
        if (s != null) for (String x : Arrays.asList(s.split(","))) if (!x.trim().isEmpty()) l.add(x.trim());
        return l;
    }
}
