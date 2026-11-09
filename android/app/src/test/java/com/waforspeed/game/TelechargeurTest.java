// WFS-306 (RG-03, RG-05, RG-07, RG-09) : téléchargement avec un petit serveur HTTP local (java.net.ServerSocket : les tests unitaires
// Android n'ont pas com.sun.net.httpserver) sur 127.0.0.1 :
// fichier complet, reprise par Range après une coupure, serveur sans Range (repart de zéro), SHA-256 faux (deux essais puis refus,
// .part effacé, ancien fichier gardé), redirection, erreur 404, place insuffisante, arrêt demandé.
package com.waforspeed.game;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.util.Random;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class TelechargeurTest {
    private ServerSocket serveur;
    private String base;
    private File dossier;
    private final byte[] donnees = new byte[300_000];
    private String sha;
    private final AtomicInteger coupureA = new AtomicInteger(-1);   // le serveur coupe après N octets (une fois)
    private final AtomicBoolean rangeAccepte = new AtomicBoolean(true);
    private final AtomicInteger requetes = new AtomicInteger(0);

    private static String hex(byte[] b) throws Exception {
        final StringBuilder s = new StringBuilder();
        for (byte x : MessageDigest.getInstance("SHA-256").digest(b)) s.append(String.format("%02x", x & 0xff));
        return s.toString();
    }

    @Before public void demarrer() throws Exception {
        new Random(42).nextBytes(donnees);
        sha = hex(donnees);
        dossier = Files.createTempDirectory("paquets").toFile();
        serveur = new ServerSocket(0, 50, InetAddress.getByName("127.0.0.1"));
        base = "http://127.0.0.1:" + serveur.getLocalPort();
        final Thread t = new Thread(() -> {
            while (!serveur.isClosed()) {
                try (Socket s = serveur.accept()) { servir(s); } catch (IOException e) { /* serveur fermé ou client parti */ }
            }
        }, "serveur-test");
        t.setDaemon(true);
        t.start();
    }
    @After public void arreter() throws IOException { serveur.close(); for (File f : dossier.listFiles()) f.delete(); dossier.delete(); }

    // une requête HTTP/1.1 par connexion : /ville.wfcarte (Range, coupure), /redirige (302), le reste 404
    private void servir(Socket s) throws IOException {
        final BufferedReader in = new BufferedReader(new InputStreamReader(s.getInputStream(), StandardCharsets.ISO_8859_1));
        final String ligne = in.readLine();
        if (ligne == null) return;
        String range = null;
        for (String h; (h = in.readLine()) != null && !h.isEmpty(); ) if (h.toLowerCase().startsWith("range:")) range = h.substring(6).trim();
        final String chemin = ligne.split(" ")[1];
        final OutputStream o = s.getOutputStream();
        if (chemin.equals("/redirige")) { o.write("HTTP/1.1 302 Found\r\nLocation: /ville.wfcarte\r\nContent-Length: 0\r\nConnection: close\r\n\r\n".getBytes(StandardCharsets.ISO_8859_1)); return; }
        if (!chemin.equals("/ville.wfcarte")) { o.write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n".getBytes(StandardCharsets.ISO_8859_1)); return; }
        requetes.incrementAndGet();
        int debut = 0;
        if (range != null && rangeAccepte.get()) debut = Integer.parseInt(range.replace("bytes=", "").replace("-", ""));
        final int n = donnees.length - debut;
        o.write(((debut > 0 ? "HTTP/1.1 206 Partial Content" : "HTTP/1.1 200 OK") + "\r\nContent-Length: " + n + "\r\nConnection: close\r\n\r\n").getBytes(StandardCharsets.ISO_8859_1));
        final int coupe = coupureA.getAndSet(-1);
        o.write(donnees, debut, coupe >= 0 ? coupe : n);   // coupure : la connexion se ferme avant la fin annoncée
        o.flush();
    }

    private File fini() { return new File(dossier, "ville_astc.wfcarte"); }
    private Telechargeur.Resultat telecharger(String chemin, String s) {
        return Telechargeur.telecharger(base + chemin, fini(), donnees.length, s, new AtomicLong(), new AtomicBoolean(false));
    }

    @Test public void fichierCompletVerifieEtRenomme() throws Exception {
        assertEquals(Telechargeur.Resultat.FINI, telecharger("/ville.wfcarte", sha));
        assertArrayEquals(donnees, Files.readAllBytes(fini().toPath()));
        assertFalse(Telechargeur.part(fini()).exists());
    }
    @Test public void repriseParRangeApresCoupure() throws Exception {
        coupureA.set(120_000);
        final Telechargeur.Resultat r1 = telecharger("/ville.wfcarte", sha);
        assertEquals(Telechargeur.Resultat.COUPURE, r1);
        final long dejaLa = Telechargeur.part(fini()).length();
        assertTrue(dejaLa > 0 && dejaLa < donnees.length);
        assertFalse(fini().exists());   // le C++ ne voit jamais un fichier non fini
        assertEquals(Telechargeur.Resultat.FINI, telecharger("/ville.wfcarte", sha));
        assertArrayEquals(donnees, Files.readAllBytes(fini().toPath()));
    }
    @Test public void serveurSansRangeRepartDeZero() throws Exception {
        rangeAccepte.set(false);
        try (FileOutputStream o = new FileOutputStream(Telechargeur.part(fini()))) { o.write(donnees, 0, 1000); }
        assertEquals(Telechargeur.Resultat.FINI, telecharger("/ville.wfcarte", sha));
        assertArrayEquals(donnees, Files.readAllBytes(fini().toPath()));
    }
    @Test public void shaFauxDeuxEssaisPuisRefusAncienGarde() throws Exception {
        Files.write(fini().toPath(), new byte[] {1, 2, 3});   // ancien paquet
        final String faux = sha.replace(sha.charAt(0), sha.charAt(0) == 'a' ? 'b' : 'a');
        assertEquals(Telechargeur.Resultat.SHA_FAUX, telecharger("/ville.wfcarte", faux));
        assertEquals(2, requetes.get());
        assertFalse(Telechargeur.part(fini()).exists());
        assertArrayEquals(new byte[] {1, 2, 3}, Files.readAllBytes(fini().toPath()));   // RG-05 : ancien gardé
    }
    @Test public void redirectionSuivie() throws Exception {
        assertEquals(Telechargeur.Resultat.FINI, telecharger("/redirige", sha));
    }
    @Test public void erreurHttp() {
        assertEquals(Telechargeur.Resultat.ERREUR_HTTP, telecharger("/absent", sha));
        assertEquals(null, Telechargeur.lireTexte(base + "/absent", 1000));
    }
    @Test public void placeInsuffisante() {
        final long enorme = Long.MAX_VALUE / 4;
        assertEquals(Telechargeur.Resultat.PLACE, Telechargeur.telecharger(base + "/ville.wfcarte", fini(), enorme, sha, new AtomicLong(), new AtomicBoolean(false)));
        assertEquals(0, requetes.get());   // rien n'est demandé au serveur
        assertEquals(1100L, Telechargeur.placeNecessaire(1000, 0));
        assertEquals(600L, Telechargeur.placeNecessaire(1000, 500));
    }
    @Test public void arretDemande() {
        assertEquals(Telechargeur.Resultat.ARRET, Telechargeur.telecharger(base + "/ville.wfcarte", fini(), donnees.length, sha, new AtomicLong(), new AtomicBoolean(true)));
        assertFalse(fini().exists());
    }
}
