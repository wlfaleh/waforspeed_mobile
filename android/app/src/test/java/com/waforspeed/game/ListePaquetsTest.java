// WFS-306 (RG-01) : choix du paquet dans cartes.json (format accepté, empreinte, version de l'APK, liste illisible ou trop récente)
package com.waforspeed.game;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import org.junit.Test;

public class ListePaquetsTest {
    private static final String SHA = "aa00000000000000000000000000000000000000000000000000000000000000";
    private static String paquet(String format, String empreinte, int minApk, String adresse) {
        return "{\"carte\":\"ville\",\"nom\":\"Ville\",\"format\":\"" + format + "\",\"version_carte\":2,\"empreinte\":\"" + empreinte + "\","
            + "\"version_min_apk\":" + minApk + ",\"taille\":22771234,\"sha256\":\"" + SHA + "\",\"adresse\":\"" + adresse + "\"}";
    }
    private static final String A = "https://github.com/wlfaleh/waforspeed_mobile/releases/download/paquets/ville_astc.wfcarte";
    private static final String R = "https://github.com/wlfaleh/waforspeed_mobile/releases/download/paquets/ville_rgba8.wfcarte";
    private static final String LISTE = "{\n \"version\": 1,\n \"paquets\": [" + paquet("astc", "1F2E", 900, A) + ",\n" + paquet("rgba8", "1f2e", 900, R) + "]}";
    private static final String[] TOUS = {"astc", "etc2", "rgba8"};

    @Test public void premierFormatAccepte() {
        final ListePaquets.Entree e = ListePaquets.choisir(LISTE, "ville", TOUS, "1f2e", 950);
        assertNotNull(e);
        assertEquals("astc", e.format);
        assertEquals(A, e.adresse);
        assertEquals(22771234L, e.taille);
        assertEquals("Ville", e.nom);
        assertEquals(2, e.versionCarte);
    }
    @Test public void sansAstcOnPrendRgba8() {
        final ListePaquets.Entree e = ListePaquets.choisir(LISTE, "ville", new String[] {"etc2", "rgba8"}, "1f2e", 950);
        assertNotNull(e);
        assertEquals("rgba8", e.format);
    }
    @Test public void autreEmpreinteRien() { assertNull(ListePaquets.choisir(LISTE, "ville", TOUS, "9999", 950)); }
    @Test public void apkTropAncienRien() { assertNull(ListePaquets.choisir(LISTE, "ville", TOUS, "1f2e", 899)); }
    @Test public void autreCarteRien() { assertNull(ListePaquets.choisir(LISTE, "plage", TOUS, "1f2e", 950)); }
    @Test public void listeIllisibleOuTropRecente() {
        assertNull(ListePaquets.choisir(null, "ville", TOUS, "1f2e", 950));
        assertNull(ListePaquets.choisir("", "ville", TOUS, "1f2e", 950));
        assertNull(ListePaquets.choisir("<html>404</html>", "ville", TOUS, "1f2e", 950));
        assertNull(ListePaquets.choisir(LISTE.substring(0, LISTE.length() - 3), "ville", TOUS, "1f2e", 950));
        assertNull(ListePaquets.choisir(LISTE.replace("\"version\": 1", "\"version\": 2"), "ville", TOUS, "1f2e", 950));
    }
    @Test public void adresseNonHttpsOuShaFauxRefuses() {
        assertNull(ListePaquets.choisir("{\"version\":1,\"paquets\":[" + paquet("astc", "1f2e", 1, "http://exemple.fr/v") + "]}", "ville", TOUS, "1f2e", 950));
        assertNull(ListePaquets.choisir("{\"version\":1,\"paquets\":[" + paquet("astc", "1f2e", 1, A).replace(SHA, "zz") + "]}", "ville", TOUS, "1f2e", 950));
    }
    @Test public void textesEchappes() {
        final String l = "{\"version\":1,\"paquets\":[" + paquet("astc", "1f2e", 1, A).replace("\"nom\":\"Ville\"", "\"nom\":\"Ville \\u00e9t\\u00e9 \\\"neuve\\\"\"") + "]}";
        final ListePaquets.Entree e = ListePaquets.choisir(l, "ville", TOUS, "1f2e", 950);
        assertNotNull(e);
        assertEquals("Ville été \"neuve\"", e.nom);
    }
}
