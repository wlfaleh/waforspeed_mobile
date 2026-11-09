// WFS-306 (RG-05) : boucle d'installation : reprise automatique après une coupure, et « Plus tard » pendant l'attente qui suit une
// coupure rend la phase PRETE (défaut relevé par le lead dev sur la PR #286) ; le .part est gardé et une nouvelle installation aboutit.
package com.waforspeed.game;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
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
import java.util.List;
import java.util.Random;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class InstallationTest {
    private ServerSocket serveur;
    private String adresse, sha;
    private File dossier;
    private final byte[] donnees = new byte[200_000];
    private final AtomicBoolean couper = new AtomicBoolean(true);   // le serveur coupe chaque réponse après 1 000 octets

    @Before public void demarrer() throws Exception {
        new Random(7).nextBytes(donnees);
        final StringBuilder h = new StringBuilder();
        for (byte x : MessageDigest.getInstance("SHA-256").digest(donnees)) h.append(String.format("%02x", x & 0xff));
        sha = h.toString();
        dossier = Files.createTempDirectory("installation").toFile();
        serveur = new ServerSocket(0, 50, InetAddress.getByName("127.0.0.1"));
        adresse = "http://127.0.0.1:" + serveur.getLocalPort() + "/ville.wfcarte";
        final Thread t = new Thread(() -> {
            while (!serveur.isClosed()) {
                try (Socket s = serveur.accept()) { servir(s); } catch (IOException e) { /* fermé */ }
            }
        });
        t.setDaemon(true);
        t.start();
    }
    @After public void arreter() throws IOException { serveur.close(); for (File f : dossier.listFiles()) f.delete(); dossier.delete(); }

    private void servir(Socket s) throws IOException {
        final BufferedReader in = new BufferedReader(new InputStreamReader(s.getInputStream(), StandardCharsets.ISO_8859_1));
        if (in.readLine() == null) return;
        int debut = 0;
        for (String l; (l = in.readLine()) != null && !l.isEmpty(); )
            if (l.toLowerCase().startsWith("range:")) debut = Integer.parseInt(l.substring(6).trim().replace("bytes=", "").replace("-", ""));
        final int n = donnees.length - debut;
        final OutputStream o = s.getOutputStream();
        o.write(((debut > 0 ? "HTTP/1.1 206 Partial Content" : "HTTP/1.1 200 OK") + "\r\nContent-Length: " + n + "\r\nConnection: close\r\n\r\n").getBytes(StandardCharsets.ISO_8859_1));
        o.write(donnees, debut, couper.get() ? Math.min(1000, n) : n);
        o.flush();
    }

    private static final class Releve implements Installation.Suivi {
        final List<Integer> phases = new CopyOnWriteArrayList<>();
        @Override public void phase(int p) { phases.add(p); }
        @Override public void journal(String t) {}
        int derniere() { return phases.get(phases.size() - 1); }
    }

    @Test public void plusTardPendantUneCoupureRendPrete() throws Exception {
        final File fini = new File(dossier, "ville_astc.wfcarte");
        final AtomicBoolean arret = new AtomicBoolean(false);
        final Releve r = new Releve();
        final Thread t = new Thread(() -> Installation.executer(adresse, fini, donnees.length, sha, new AtomicLong(), arret, 300, r));
        t.start();
        final long fin = System.currentTimeMillis() + 5000;
        while (!r.phases.contains(Installation.COUPURE) && System.currentTimeMillis() < fin) Thread.sleep(10);
        assertTrue(r.phases.contains(Installation.COUPURE));
        arret.set(true);   // « Plus tard » pendant l'attente après la coupure
        t.join(5000);
        assertFalse(t.isAlive());
        assertEquals(Installation.PRETE, r.derniere());
        assertTrue(Telechargeur.part(fini).length() > 0);   // .part gardé pour la reprise
        assertFalse(fini.exists());
        // nouvelle installation dans la même session : elle aboutit (reprise par Range)
        couper.set(false);
        arret.set(false);
        final Releve r2 = new Releve();
        Installation.executer(adresse, fini, donnees.length, sha, new AtomicLong(), arret, 300, r2);
        assertEquals(Installation.FINI, r2.derniere());
        assertArrayEquals(donnees, Files.readAllBytes(fini.toPath()));
    }

    @Test public void repriseAutomatiqueApresCoupures() throws Exception {
        final File fini = new File(dossier, "ville_astc.wfcarte");
        final Releve r = new Releve();
        final Thread coupeur = new Thread(() -> { try { Thread.sleep(400); } catch (InterruptedException e) { return; } couper.set(false); });   // réseau revenu
        coupeur.start();
        Installation.executer(adresse, fini, donnees.length, sha, new AtomicLong(), new AtomicBoolean(false), 100, r);
        assertTrue(r.phases.contains(Installation.COUPURE));
        assertEquals(Installation.FINI, r.derniere());
        assertArrayEquals(donnees, Files.readAllBytes(fini.toPath()));
    }

    @Test public void arretAvantLeDebutRendPrete() {
        final Releve r = new Releve();
        Installation.executer(adresse, new File(dossier, "x.wfcarte"), donnees.length, sha, new AtomicLong(), new AtomicBoolean(true), 100, r);
        assertEquals(Installation.PRETE, r.derniere());
    }
}
