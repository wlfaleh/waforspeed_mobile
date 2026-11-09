Agent : agent expert ci cd

**Demande de l'utilisateur** : « Go » (2026-11-09) sur le plan de l'expert : correction de fond après la Release `apercu` pleine (WFS-350), message « Recette QA prête » juste, bancs manquants aux bancs du matin (ANO-200, recette du QA du 2026-11-09).

## Ce qui change
- **RG-01, `matin.yml` + `tools/qa/publier_captures_matin.sh` (neuf)** : les captures du matin ne vont plus dans `apercu` (qui ne garde que l'APK). Une **archive zip par groupe** du banc part dans la Release fixe `captures-matin` (`captures-<groupe>.zip`), avec `jour.txt` (date, run, commit, nombre de fichiers par groupe). ≈ 20 envois au lieu de ~1 000 (limite de 1 000 fichiers par Release et quota horaire du GITHUB_TOKEN tenus). Une archive d'un groupe absent du run est retirée (jamais une capture d'un autre jour présentée comme du jour). Groupe absent ou envoi raté → job rouge.
- **RG-02** : l'émulateur garde son job vert (information), mais le message « Recette prête » reçoit le **vrai** résultat du parcours (`outputs.resultat` = `steps.parcours.outcome`) ; la publication des captures est rouge quand elle rate (avant : verte avec un avertissement). `recette_prete.sh` donne l'adresse de `captures-matin` et celle de l'APK.
- **RG-03, `banc_groupe.sh` + matrice de `matin.yml`** : trois groupes neufs, `complements-a` (qualite), `complements-b` (lod, perf), `complements-c` (cuisson, cgu, paquets, virages). Chaque banc garde son journal (`<banc>-journal.txt`) et ses images préfixées par son nom ; un écart (code 3) rend le job rouge.

## Comment c'est vérifié
- YAML relu (`yaml.safe_load`), `bash -n` sur les scripts.
- `publier_captures_matin.sh` en essai (`ESSAI=1`) avec 3 groupes attendus dont un absent : 2 archives, `jour.txt` « vitrines : ABSENT », code 1.
- `complements-c` en local (4 cœurs, lavapipe) : cuisson vert ≈ 10 min, cgu vert ≈ 3 min, paquets vert ≈ 3 min, virages vert ≈ 21 min (« banc des virages (WFS-295) : 0 écart(s) OK ») → ≈ 37 min, sous la limite de 75 min du job.
- `complements-a` et `complements-b` : durée mesurée au premier matin (au journal) ; s'ils dépassent, ils seront recoupés.

**Captures** : ticket de CI sans changement à l'écran.

## Limites et risques
- La QA lit maintenant les captures du matin dans les archives de `captures-matin` (à décompresser) ; les anciennes captures restent lisibles dans `apercu-ancien-20261109-0925`. Le skill du QA et le PO sont prévenus.
- `build.yml` (photos de fusion dans `apercu-fusion`) ne change pas.
- Un banc ajouté qui est rouge aujourd'hui rendra les bancs du matin rouges : c'est voulu (RG-03), le PO crée alors le ticket de correction.
