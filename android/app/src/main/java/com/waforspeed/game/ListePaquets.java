// WaForSpeed — liste des paquets de cartes « cartes.json » (WFS-306, format proposé dans regles-de-gestion/WFS-306.md, repris par WFS-302).
// Règle pure, sans Android ni bibliothèque JSON (testée par JUnit sur la machine de la CI) : lit la liste et choisit le paquet d'une carte
// pour le téléphone (format de texture accepté, empreinte attendue par le jeu, version minimale de l'APK).
//
// {
//   "version": 1,
//   "paquets": [
//     { "carte": "ville", "nom": "Ville", "format": "astc", "version_carte": 2, "empreinte": "1f2e3d4c5b6a7980",
//       "version_min_apk": 950, "taille": 22771234, "sha256": "<64 chiffres hexadécimaux>",
//       "adresse": "https://github.com/wlfaleh/waforspeed_mobile/releases/download/paquets/ville_astc.wfcarte" }
//   ]
// }
package com.waforspeed.game;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class ListePaquets {
    private ListePaquets() {}

    public static final int VERSION_LISTE = 1;   // version du format de cartes.json lue par ce jeu (plus récente : liste ignorée)

    // paquet retenu pour le téléphone
    public static final class Entree {
        public final String carte, nom, format, empreinte, sha256, adresse;
        public final long taille;
        public final int versionCarte;
        Entree(String carte, String nom, String format, String empreinte, String sha256, String adresse, long taille, int versionCarte) {
            this.carte = carte; this.nom = nom; this.format = format; this.empreinte = empreinte; this.sha256 = sha256; this.adresse = adresse;
            this.taille = taille; this.versionCarte = versionCarte;
        }
    }

    // RG-01 : paquet de « carte » dont l'empreinte est celle qu'attend le jeu (WFS-301 RG-02), au premier format accepté par le
    // téléphone (ordre de « formats » : ASTC, puis ETC2, puis RGBA8), avec version_min_apk ≤ versionApk ; null sinon (liste illisible,
    // trop récente, carte absente, adresse non https, taille ou SHA-256 invalides)
    public static Entree choisir(String json, String carte, String[] formats, String empreinte, int versionApk) {
        final Object racine;
        try { racine = new Lecteur(json).lireTout(); } catch (RuntimeException e) { return null; }
        if (!(racine instanceof Map)) return null;
        final Map<?, ?> r = (Map<?, ?>) racine;
        final long v = nombre(r.get("version"), -1);
        if (v < 1 || v > VERSION_LISTE) return null;
        if (!(r.get("paquets") instanceof List)) return null;
        for (String f : formats) {
            for (Object o : (List<?>) r.get("paquets")) {
                if (!(o instanceof Map)) continue;
                final Map<?, ?> p = (Map<?, ?>) o;
                if (!carte.equals(p.get("carte")) || !f.equals(p.get("format")) || !empreinte.equalsIgnoreCase(texte(p.get("empreinte")))) continue;
                if (nombre(p.get("version_min_apk"), Long.MAX_VALUE) > versionApk) continue;
                final long taille = nombre(p.get("taille"), -1);
                final String sha = texte(p.get("sha256")), adresse = texte(p.get("adresse"));
                if (taille <= 0 || !sha.matches("[0-9a-fA-F]{64}") || !adresse.startsWith("https://")) continue;
                final String nom = texte(p.get("nom")).isEmpty() ? carte : texte(p.get("nom"));
                return new Entree(carte, nom, f, texte(p.get("empreinte")), sha.toLowerCase(), adresse, taille, (int) nombre(p.get("version_carte"), 1));
            }
        }
        return null;
    }

    private static String texte(Object o) { return o instanceof String ? (String) o : ""; }
    private static long nombre(Object o, long defaut) {
        if (!(o instanceof Double)) return defaut;
        final double d = (Double) o;
        return d == Math.rint(d) && Math.abs(d) < 9.0e15 ? (long) d : defaut;
    }

    // lecteur JSON minimal (objets, tableaux, textes, nombres, true / false / null) ; erreur → IllegalArgumentException
    static final class Lecteur {
        private final String s; private int p = 0, profondeur = 0;
        Lecteur(String s) { this.s = s == null ? "" : s; }
        Object lireTout() { final Object o = valeur(); blancs(); if (p != s.length()) throw erreur(); return o; }
        private IllegalArgumentException erreur() { return new IllegalArgumentException("JSON illisible à " + p); }
        private void blancs() { while (p < s.length() && " \t\r\n".indexOf(s.charAt(p)) >= 0) p++; }
        private Object valeur() {
            blancs();
            if (p >= s.length()) throw erreur();
            final char c = s.charAt(p);
            if (c == '{') return objet();
            if (c == '[') return tableau();
            if (c == '"') return chaine();
            if (s.startsWith("true", p)) { p += 4; return Boolean.TRUE; }
            if (s.startsWith("false", p)) { p += 5; return Boolean.FALSE; }
            if (s.startsWith("null", p)) { p += 4; return null; }
            return nombre();
        }
        private Map<String, Object> objet() {
            if (++profondeur > 32) throw erreur();
            final Map<String, Object> m = new LinkedHashMap<>();
            p++; blancs();
            if (p < s.length() && s.charAt(p) == '}') { p++; profondeur--; return m; }
            while (true) {
                blancs();
                if (p >= s.length() || s.charAt(p) != '"') throw erreur();
                final String k = chaine();
                blancs();
                if (p >= s.length() || s.charAt(p++) != ':') throw erreur();
                m.put(k, valeur());
                blancs();
                if (p >= s.length()) throw erreur();
                final char c = s.charAt(p++);
                if (c == '}') { profondeur--; return m; }
                if (c != ',') throw erreur();
            }
        }
        private List<Object> tableau() {
            if (++profondeur > 32) throw erreur();
            final List<Object> l = new ArrayList<>();
            p++; blancs();
            if (p < s.length() && s.charAt(p) == ']') { p++; profondeur--; return l; }
            while (true) {
                l.add(valeur());
                blancs();
                if (p >= s.length()) throw erreur();
                final char c = s.charAt(p++);
                if (c == ']') { profondeur--; return l; }
                if (c != ',') throw erreur();
            }
        }
        private String chaine() {
            final StringBuilder b = new StringBuilder();
            p++;
            while (p < s.length()) {
                final char c = s.charAt(p++);
                if (c == '"') return b.toString();
                if (c != '\\') { b.append(c); continue; }
                if (p >= s.length()) break;
                final char e = s.charAt(p++);
                switch (e) {
                    case 'n': b.append('\n'); break;
                    case 't': b.append('\t'); break;
                    case 'r': b.append('\r'); break;
                    case 'b': b.append('\b'); break;
                    case 'f': b.append('\f'); break;
                    case 'u':
                        if (p + 4 > s.length()) throw erreur();
                        try { b.append((char) Integer.parseInt(s.substring(p, p + 4), 16)); } catch (NumberFormatException x) { throw erreur(); }
                        p += 4; break;
                    default: b.append(e);
                }
            }
            throw erreur();
        }
        private Double nombre() {
            final int d = p;
            while (p < s.length() && "+-0123456789.eE".indexOf(s.charAt(p)) >= 0) p++;
            if (d == p) throw erreur();
            try { return Double.valueOf(s.substring(d, p)); } catch (NumberFormatException x) { throw erreur(); }
        }
    }
}
