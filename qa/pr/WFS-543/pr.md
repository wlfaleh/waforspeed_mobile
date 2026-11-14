## WFS-543 — preuves (US technique, sans image à l'écran)

### Contrôle des licences, avant (`main`, 1e9f5d2) : 4 manques
```
licences : 17 voitures livrées (3 retirées : a45, elantra, x5), 172 fichiers d'assets vérifiés, 4 manque(s)
  MANQUE cars/remplacants/virtus.lod : absent de CARS[] (pas de crédit)
  MANQUE cars/remplacants/virtus.wfm : absent de CARS[] (pas de crédit)
  MANQUE cars/remplacants/w212.lod : absent de CARS[] (pas de crédit)
  MANQUE cars/remplacants/w212.wfm : absent de CARS[] (pas de crédit)
code 1
```

### Contrôle des licences, après (branche) : 0 manque
```
licences : 17 voitures livrées (3 retirées : a45, elantra, x5), 172 fichiers d'assets vérifiés, 0 manque(s)
code 0
```

### Contre-épreuve (le contrôle n'est pas affaibli)
Licence `remplacants/w212.txt` retirée et faux fichier `cars/remplacants/inconnu.wfm` ajouté (essai local, annulé ensuite) :
```
licences : 17 voitures livrées (3 retirées : a45, elantra, x5), 173 fichiers d'assets vérifiés, 3 manque(s)
  MANQUE cars/remplacants/inconnu.wfm : actif livré sans crédit connu (ajouter sa ligne dans AUTRES et COUVERTURE)
  MANQUE cars/remplacants/w212.lod : licence remplacants/w212.txt absente de assets/licences/
  MANQUE cars/remplacants/w212.wfm : licence remplacants/w212.txt absente de assets/licences/
```

### Tests CPU (local, `bash tools/qa/tests_cpu.sh build`)
```
tests CPU : 138 test(s) réussi(s) en 394 s
```
(`test_credits` compris : crédits en anglais sans mot français, aucune marque.)

### CI de la branche
- #1066 : annulé (commit sans la traduction anglaise, `test_credits` aurait été rouge).
- #1067 : https://github.com/wlfaleh/waforspeed_mobile/actions/runs/43587972017 (commit e2dd6fdd).
