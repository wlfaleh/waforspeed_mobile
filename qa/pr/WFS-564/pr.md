## Preuves (US technique sans image à l'écran : pas de banc de captures, RG-04)

### Contrôle des licences — avant (`main`, commit e1c86795 et suivants)
```
$ python3 tools/qa/verifier_licences.py
licences : 17 voitures livrées (3 retirées : a45, elantra, x5), 204 fichiers d'assets vérifiés, 1 manque(s)
  MANQUE ui/icone-pistolet.png : actif livré sans crédit connu (ajouter sa ligne dans AUTRES et COUVERTURE)
$ echo $?
1
```
(même sortie que le job « banc-essai (texte) » du run #1147 de `main`, run 43817146076)

### Contrôle des licences — après (branche `feature/waforspeed-564`)
```
$ python3 tools/qa/verifier_licences.py
licences : 17 voitures livrées (3 retirées : a45, elantra, x5), 204 fichiers d'assets vérifiés, 0 manque(s)
$ echo $?
0
```

### Tests CPU (branche, compilation locale Release)
```
$ bash tools/qa/tests_cpu.sh build
tests CPU : 150 test(s) réussi(s) en 394 s
```
