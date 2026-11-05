// WFS-132 (RG-08) : règle de la mise à jour, 8 cas
package com.waforspeed.game;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import org.junit.Test;

public class MiseAJourTest {
    private static final String URL = "https://github.com/wlfaleh/waforspeed_mobile/releases/download/apercu/WaForSpeed.apk";

    private static String reponse(String titre, String piece) {
        return "{\"url\":\"x\",\"author\":{\"login\":\"github-actions[bot]\"},\"tag_name\":\"apercu\",\"name\":\"" + titre + "\","
            + "\"assets\":[{\"name\":\"accueil.png\",\"browser_download_url\":\"https://github.com/wlfaleh/waforspeed_mobile/releases/download/apercu/accueil.png\"},"
            + "{\"name\":\"" + piece + "\",\"uploader\":{\"login\":\"bot\"},\"browser_download_url\":\"https://github.com/wlfaleh/waforspeed_mobile/releases/download/apercu/" + piece + "\"}]}";
    }
    private static final String BON = reponse("Aperçu — construction 412 (abc1234)", "WaForSpeed.apk");

    @Test public void titreValideDonneNumeroEtCommit() {
        final MiseAJour.Proposition p = MiseAJour.analyser(BON, 400, 0);
        assertNotNull(p);
        assertEquals(412, p.numero);
        assertEquals("abc1234", p.commit);
        assertEquals(URL, p.adresseApk);
    }
    @Test public void plusRecenteProposee() { assertNotNull(MiseAJour.analyser(BON, 411, 0)); }
    @Test public void egaleOuPlusAncienneRien() {
        assertNull(MiseAJour.analyser(BON, 412, 0));
        assertNull(MiseAJour.analyser(BON, 500, 0));
    }
    @Test public void ignoreeRien() { assertNull(MiseAJour.analyser(BON, 400, 412)); }
    @Test public void versionCodeUnRien() { assertNull(MiseAJour.analyser(BON, 1, 0)); }
    @Test public void jsonCasseRien() {
        assertNull(MiseAJour.analyser(BON.substring(0, BON.length() - 20), 400, 0));
        assertNull(MiseAJour.analyser("pas du json", 400, 0));
        assertNull(MiseAJour.analyser(null, 400, 0));
    }
    @Test public void sansPieceApkRien() { assertNull(MiseAJour.analyser(reponse("Aperçu — construction 412 (abc1234)", "WaForSpeed.aab"), 400, 0)); }
    @Test public void titreSansNumeroRien() { assertNull(MiseAJour.analyser(reponse("Aperçu", "WaForSpeed.apk"), 400, 0)); }
}
