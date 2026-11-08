// WaForSpeed — téléchargement d'un paquet de carte (WFS-306 RG-03, RG-05, RG-07). Java pur (HttpURLConnection, MessageDigest), sans
// Android ni bibliothèque : testé par JUnit avec un petit serveur local (TelechargeurTest).
//   - écriture dans <fini>.part, reprise par « Range: bytes=N- » (réponse 206 : suite ; 200 : le serveur repart de zéro) ;
//   - SHA-256 du fichier entier comparé à cartes.json : juste → renommé en <fini> (le C++ ne lit que des fichiers finis) ;
//     faux → .part effacé, un seul nouvel essai complet, puis SHA_FAUX ;
//   - place libre comparée au reste à télécharger + 10 % de la taille (RG-07) ;
//   - redirections suivies (GitHub Releases renvoie vers un autre domaine) ; une redirection non https est refusée (sauf 127.0.0.1 : tests) ;
//   - coupure (erreur réseau, arrêt demandé) : le .part est gardé, la reprise repart de là.
// Rien n'est envoyé hormis la requête du fichier (RG-08).
package com.waforspeed.game;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

public final class Telechargeur {
    public enum Resultat { FINI, COUPURE, SHA_FAUX, PLACE, ERREUR_HTTP, ARRET }

    public static int delaiMs = 15000;               // connexion et lecture
    public static int redirectionsMax = 5;

    private Telechargeur() {}

    // octets du fichier .part déjà là (0 sans fichier)
    public static File part(File fini) { return new File(fini.getPath() + ".part"); }

    // RG-07 : place nécessaire avant de commencer (reste à télécharger + 10 % de la taille)
    public static long placeNecessaire(long taille, long dejaLa) { return Math.max(0, taille - dejaLa) + taille / 10; }

    // texte d'une adresse (cartes.json), null en cas d'erreur, de réponse non 200 ou de plus de max octets
    public static String lireTexte(String adresse, int max) {
        try {
            final HttpURLConnection c = ouvrir(adresse, -1);
            try {
                if (c.getResponseCode() != 200) return null;
                final byte[] b = new byte[16384];
                final java.io.ByteArrayOutputStream o = new java.io.ByteArrayOutputStream();
                try (InputStream in = c.getInputStream()) {
                    int k;
                    while ((k = in.read(b)) > 0) { o.write(b, 0, k); if (o.size() > max) return null; }
                }
                return new String(o.toByteArray(), java.nio.charset.StandardCharsets.UTF_8);
            } finally { c.disconnect(); }
        } catch (IOException e) { return null; }
    }

    // RG-03 / RG-05 : télécharge jusqu'au fichier fini vérifié ; recus est mis à jour en continu (octets présents dans le .part)
    public static Resultat telecharger(String adresse, File fini, long taille, String sha256, AtomicLong recus, AtomicBoolean arret) {
        for (int essai = 0; essai < 2; essai++) {
            final Resultat r = unEssai(adresse, fini, taille, sha256, recus, arret);
            if (r != Resultat.SHA_FAUX) return r;
        }
        return Resultat.SHA_FAUX;
    }

    private static Resultat unEssai(String adresse, File fini, long taille, String sha256, AtomicLong recus, AtomicBoolean arret) {
        final File p = part(fini);
        final File dossier = fini.getAbsoluteFile().getParentFile();
        if (dossier != null && !dossier.isDirectory() && !dossier.mkdirs()) return Resultat.ERREUR_HTTP;
        long deja = p.isFile() ? p.length() : 0;
        if (deja > taille) { p.delete(); deja = 0; }
        recus.set(deja);
        if (dossier != null && dossier.getUsableSpace() < placeNecessaire(taille, deja)) return Resultat.PLACE;
        if (deja < taille) {
            HttpURLConnection c = null;
            try {
                c = ouvrir(adresse, deja);
                final int code = c.getResponseCode();
                boolean ajout;
                if (code == 206 && deja > 0) ajout = true;
                else if (code == 200) { ajout = false; deja = 0; recus.set(0); }
                else if (code == 416) { ajout = true; }   // déjà tout reçu : vérification ci-dessous
                else return Resultat.ERREUR_HTTP;
                if (code != 416) {
                    try (InputStream in = c.getInputStream(); FileOutputStream out = new FileOutputStream(p, ajout)) {
                        final byte[] b = new byte[65536];
                        int k;
                        while ((k = in.read(b)) > 0) {
                            if (arret.get()) return Resultat.ARRET;
                            if (deja + k > taille) return Resultat.ERREUR_HTTP;   // plus long qu'annoncé
                            out.write(b, 0, k);
                            deja += k;
                            recus.set(deja);
                        }
                    }
                }
            } catch (IOException e) {
                return arret.get() ? Resultat.ARRET : Resultat.COUPURE;
            } finally { if (c != null) c.disconnect(); }
            if (deja < taille) return arret.get() ? Resultat.ARRET : Resultat.COUPURE;   // flux terminé trop tôt
        }
        final String calcule = sha256(p);
        if (calcule == null || !calcule.equalsIgnoreCase(sha256)) { p.delete(); recus.set(0); return Resultat.SHA_FAUX; }
        if (fini.exists() && !fini.delete()) return Resultat.ERREUR_HTTP;   // ancien paquet remplacé seulement par un nouveau vérifié
        return p.renameTo(fini) ? Resultat.FINI : Resultat.ERREUR_HTTP;
    }

    // connexion https (ou http local du banc et des tests), redirections suivies à la main (HttpURLConnection ne suit pas http → https)
    static HttpURLConnection ouvrir(String adresse, long depuis) throws IOException {
        String a = adresse;
        for (int i = 0; i <= redirectionsMax; i++) {
            final HttpURLConnection c = (HttpURLConnection) new URL(a).openConnection();
            c.setInstanceFollowRedirects(false);
            c.setConnectTimeout(delaiMs);
            c.setReadTimeout(delaiMs);
            c.setRequestProperty("User-Agent", "WaForSpeed-Android");
            if (depuis > 0) c.setRequestProperty("Range", "bytes=" + depuis + "-");
            final int code = c.getResponseCode();
            if (code / 100 != 3) return c;
            final String suite = c.getHeaderField("Location");
            c.disconnect();
            if (suite == null) throw new IOException("redirection sans adresse");
            a = new URL(new URL(a), suite).toString();
            if (!a.startsWith("https://") && !a.startsWith("http://127.0.0.1")) throw new IOException("redirection non https refusée");   // RG-08
        }
        throw new IOException("trop de redirections");
    }

    // SHA-256 en hexadécimal minuscule, null si le fichier est illisible
    public static String sha256(File f) {
        try (InputStream in = new FileInputStream(f)) {
            final MessageDigest md = MessageDigest.getInstance("SHA-256");
            final byte[] b = new byte[65536];
            int k;
            while ((k = in.read(b)) > 0) md.update(b, 0, k);
            final StringBuilder s = new StringBuilder();
            for (byte x : md.digest()) s.append(String.format("%02x", x & 0xff));
            return s.toString();
        } catch (IOException | NoSuchAlgorithmException e) { return null; }
    }
}
