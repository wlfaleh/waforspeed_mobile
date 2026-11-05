// WaForSpeed — nouvelle version de l'APK annoncée au lancement (WFS-132).
// Règle pure (RG-03), sans Android ni bibliothèque JSON (testable par JUnit sur la machine de la CI) : lit la réponse de
// GET /repos/wlfaleh/waforspeed_mobile/releases/tags/apercu (RG-02) et dit s'il faut proposer la construction distante.
package com.waforspeed.game;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public final class MiseAJour {
    private MiseAJour() {}

    // construction proposée : numéro N, commit court, adresse de WaForSpeed.apk
    public static final class Proposition {
        public final int numero;
        public final String commit;
        public final String adresseApk;
        Proposition(int numero, String commit, String adresseApk) { this.numero = numero; this.commit = commit; this.adresseApk = adresseApk; }
    }

    // RG-02 : titre écrit par tools/qa/publier_apercu.sh l. 113 (« Aperçu — construction N (abcdefg) »)
    private static final Pattern TITRE = Pattern.compile("construction (\\d+) \\(([0-9a-f]{7})\\)");
    private static final Pattern NOM = Pattern.compile("\"name\"\\s*:\\s*\"((?:[^\"\\\\]|\\\\.)*)\"");
    private static final Pattern APK = Pattern.compile("\"browser_download_url\"\\s*:\\s*\"(https://[^\"]*/WaForSpeed\\.apk)\"");

    // titre de la Release (premier champ « name » qui porte un numéro de construction), null sinon
    static Matcher lireTitre(String json) {
        final Matcher m = NOM.matcher(json);
        while (m.find()) {
            final Matcher t = TITRE.matcher(m.group(1));
            if (t.find()) return t;
        }
        return null;
    }

    // RG-03 : proposition si N > versionCode, versionCode != 1 (APK hors CI) et N != construction ignorée (RG-05) ;
    // null pour tout le reste (JSON cassé, pièce WaForSpeed.apk absente, titre sans numéro)
    public static Proposition analyser(String json, int versionCode, int ignoree) {
        if (json == null || versionCode == 1) return null;
        final String j = json.trim();
        if (!j.startsWith("{") || !j.endsWith("}")) return null;   // réponse coupée ou illisible
        final Matcher t = lireTitre(j);
        if (t == null) return null;
        final int n;
        try { n = Integer.parseInt(t.group(1)); } catch (NumberFormatException e) { return null; }
        final Matcher a = APK.matcher(j);
        if (!a.find()) return null;
        if (n <= versionCode || n == ignoree) return null;
        return new Proposition(n, t.group(2), a.group(1));
    }
}
