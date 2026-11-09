// WFS-307 (RG-02) : plusieurs cartes, une à la fois (file d'attente), annulation d'une carte qui attend, état par carte, carte supprimée
// de nouveau proposable ; liste absente → rien à proposer. Petit serveur HTTP local (java.net.ServerSocket) sur 127.0.0.1.
package com.waforspeed.game;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class FilePaquetsTest {
    private ServerSocket serveur;
    private String base;
    private File dossier;
    private final byte[] ville = new byte[150_000], plage = new byte[120_000];
    private final AtomicInteger enCours = new AtomicInteger(0), maxEnMemeTemps = new AtomicInteger(0);

    private static String sha(byte[] b) throws Exception {
        final StringBuilder h = new StringBuilder();
        for (byte x : MessageDigest.getInstance("SHA-256").digest(b)) h.append(String.format("%02x", x & 0xff));
        return h.toString();
    }
    private String liste() throws Exception {
        return "{\"version\":1,\"paquets\":["
            + "{\"carte\":\"ville\",\"nom\":\"Ville\",\"format\":\"astc\",\"empreinte\":\"aa\",\"version_min_apk\":1,\"taille\":" + ville.length + ",\"sha256\":\"" + sha(ville) + "\",\"adresse\":\"https://x/ville\"},"
            + "{\"carte\":\"plage\",\"nom\":\"Corniche\",\"format\":\"astc\",\"empreinte\":\"bb\",\"version_min_apk\":1,\"taille\":" + plage.length + ",\"sha256\":\"" + sha(plage) + "\",\"adresse\":\"https://x/plage\"}]}";
    }

    @Before public void demarrer() throws Exception {
        new Random(1).nextBytes(ville); new Random(2).nextBytes(plage);
        dossier = Files.createTempDirectory("file").toFile();
        serveur = new ServerSocket(0, 50, InetAddress.getByName("127.0.0.1"));
        base = "http://127.0.0.1:" + serveur.getLocalPort();
        final Thread t = new Thread(() -> {
            while (!serveur.isClosed()) {
                try { final Socket s = serveur.accept(); new Thread(() -> { try (Socket x = s) { servir(x); } catch (IOException e) { /* fini */ } }).start(); }
                catch (IOException e) { /* fermé */ }
            }
        });
        t.setDaemon(true);
        t.start();
    }
    @After public void arreter() throws IOException { serveur.close(); for (File f : dossier.listFiles()) f.delete(); dossier.delete(); }

    private void servir(Socket s) throws IOException {
        final BufferedReader in = new BufferedReader(new InputStreamReader(s.getInputStream(), StandardCharsets.ISO_8859_1));
        final String l = in.readLine();
        if (l == null) return;
        for (String h; (h = in.readLine()) != null && !h.isEmpty(); ) { }
        final byte[] d = l.contains("/ville") ? ville : plage;
        maxEnMemeTemps.accumulateAndGet(enCours.incrementAndGet(), Math::max);
        try {
            final OutputStream o = s.getOutputStream();
            o.write(("HTTP/1.1 200 OK\r\nContent-Length: " + d.length + "\r\nConnection: close\r\n\r\n").getBytes(StandardCharsets.ISO_8859_1));
            for (int i = 0; i < d.length; i += 10_000) { try { Thread.sleep(5); } catch (InterruptedException e) { return; } o.write(d, i, Math.min(10_000, d.length - i)); o.flush(); }   // pause avant chaque bloc : fin comptée dès le dernier octet
        } finally { enCours.decrementAndGet(); }
    }

    private static void attendre(FilePaquets f, String c, int phase) throws InterruptedException {
        final long fin = System.currentTimeMillis() + 10000;
        while (f.etat(c)[0] != phase && System.currentTimeMillis() < fin) Thread.sleep(10);
    }

    @Test public void deuxCartesUneAlaFois() throws Exception {
        // adresses http://127.0.0.1 : refusées par ListePaquets (https seulement) → liste écrite à la main avec https puis remplacée
        final FilePaquets f = new FilePaquets(50, null);
        final Map<String, String> emp = new LinkedHashMap<>();
        emp.put("ville", "aa"); emp.put("plage", "bb");
        f.liste(liste(), new String[] {"astc"}, emp, 900);
        assertEquals(Installation.PRETE, f.etat("ville")[0]);
        assertArrayEquals(new String[] {"Corniche", "astc"}, f.proposition("plage"));
    }

    @Test public void fileDAttenteEtAnnulation() throws Exception {
        final FilePaquets f = fichierLocal();
        assertTrue(f.installer(dossier.getPath(), "ville"));
        assertTrue(f.installer(dossier.getPath(), "plage"));
        assertFalse(f.installer(dossier.getPath(), "plage"));   // déjà dans la file
        assertEquals(FilePaquets.ATTENTE, f.etat("plage")[0]);
        attendre(f, "ville", Installation.FINI);
        attendre(f, "plage", Installation.FINI);
        assertEquals(Installation.FINI, f.etat("ville")[0]);
        assertEquals(Installation.FINI, f.etat("plage")[0]);
        assertEquals(1, maxEnMemeTemps.get());   // une seule carte à la fois
        assertArrayEquals(ville, Files.readAllBytes(new File(dossier, "ville_astc.wfcarte").toPath()));
        assertArrayEquals(plage, Files.readAllBytes(new File(dossier, "plage_astc.wfcarte").toPath()));
        // supprimée par le jeu : de nouveau proposable
        new File(dossier, "plage_astc.wfcarte").delete();
        f.supprimee("plage");
        assertEquals(Installation.PRETE, f.etat("plage")[0]);
        // annulation d'une carte qui attend son tour
        assertTrue(f.installer(dossier.getPath(), "ville"));
        assertTrue(f.installer(dossier.getPath(), "plage"));
        f.arreter("plage");
        assertEquals(Installation.PRETE, f.etat("plage")[0]);
        attendre(f, "ville", Installation.FINI);
        Thread.sleep(200);
        assertFalse(new File(dossier, "plage_astc.wfcarte").exists());
    }

    @Test public void sansListeRienAProposer() {
        final FilePaquets f = new FilePaquets(50, null);
        f.recherche(Arrays.asList("ville", "plage"));
        assertEquals(Installation.LISTE, f.etat("ville")[0]);
        f.listeAbsente(Arrays.asList("ville", "plage"));
        assertEquals(Installation.ABSENTE, f.etat("plage")[0]);
        assertNull(f.proposition("plage"));
        assertFalse(f.installer("x", "plage"));
    }

    // liste dont les adresses pointent sur le serveur local (ListePaquets n'accepte que https : entrées remplacées après le choix)
    private FilePaquets fichierLocal() throws Exception {
        final FilePaquets f = new FilePaquets(50, null);
        final Map<String, String> emp = new LinkedHashMap<>();
        emp.put("ville", "aa"); emp.put("plage", "bb");
        f.liste(liste(), new String[] {"astc"}, emp, 900);
        f.adresseDeTest("ville", base + "/ville");
        f.adresseDeTest("plage", base + "/plage");
        return f;
    }
}
