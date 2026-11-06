#!/usr/bin/env python3
# WaForSpeed — WFS-198 : robot des archives (ménage de nuit de journal.md et TACHES.md).
#   - journal.md : les entrées « ### AAAA-MM-JJ … » de plus de 2 jours (on garde aujourd'hui et hier, date UTC) vont à la fin de
#     journal/archive/AAAA-MM-JJ.md (un fichier par jour, créé avec l'en-tête des fichiers existants), dans l'ordre ; l'en-tête
#     de journal.md (texte avant la première entrée) ne bouge pas (RG-02) ;
#   - TACHES.md : les lignes du tableau des tickets dont la case Statut commence par ✅ ou ↪, ou contient « annulé » /
#     « abandonné », vont telles quelles à la fin de archive/taches-terminees.md ; avant, la ligne est recopiée dans
#     regles-de-gestion/WFS-XX.md sous « ## Suivi repris de `TACHES.md` » si cette section n'existe pas encore (RG-03) ;
#   - ne devine jamais : ligne de ticket sans 10 cases ou clé illisible, entrée sans date lisible → laissée en place et listée
#     pour le PO (RG-04) ;
#   - compte les entrées et les lignes de tickets avant et après : la moindre différence → rien n'est écrit, code de sortie 1
#     (RG-05) ; tache utilisateur.md n'est jamais lu ni écrit (RG-06) ;
#   - une entrée « Archives automatiques (robot archives) » au journal quand quelque chose a bougé ou doit être corrigé (RG-07).
# Usage : python3 tools/qa/archives.py --depot . [--aujourdhui AAAA-MM-JJ] [--maintenant "AAAA-MM-JJ hh:mm:ss"]
# Sortie : 0 (fichiers écrits, ou rien à faire), 1 (écart de comptage : rien d'écrit). Résumé sur la sortie standard.
import argparse, datetime, os, re, sys

RE_ENTREE = re.compile(r'^### ')
RE_DATE = re.compile(r'^### (\d{4})-(\d{2})-(\d{2})\b')
RE_CLE = re.compile(r'^WFS-\d+$')
SECTION_SUIVI = "## Suivi repris de `TACHES.md`"
ENTETE_ARCHIVE_T = ("# Tickets terminés (archive de `TACHES.md`)\n\nLignes complètes des tickets ✅ terminés ou annulés, retirées de "
                    "`TACHES.md` (règle de l'utilisateur, 2026-11-06). Critères et suivi : `regles-de-gestion/WFS-XX.md`.\n\n"
                    "| Priorité | Clé | Titre | Épic | Statut | Responsable | Dépend de | Branche | PR | QA |\n"
                    "|---|---|---|---|---|---|---|---|---|---|\n")
ENTETE_JOUR = ("# Journal archivé — {d}\n\nEntrées du {d} retirées de `journal.md` (règle de l'utilisateur, 2026-11-06 : `journal.md` "
               "ne garde que les 2 derniers jours). Lire seulement en cas de besoin précis.\n\n")


def lire(chemin):
    with open(chemin, encoding='utf-8') as f:
        return f.read()


def ecrire(chemin, texte):
    os.makedirs(os.path.dirname(chemin) or '.', exist_ok=True)
    with open(chemin, 'w', encoding='utf-8') as f:
        f.write(texte)


# ---------------------------------------------------------------- journal
def decouper_journal(texte):
    """En-tête (texte avant la 1re entrée) et liste des entrées (chacune : ses lignes, avec la ligne « ### »)."""
    lignes = texte.splitlines(keepends=True)
    entete, entrees = [], []
    for l in lignes:
        if RE_ENTREE.match(l):
            entrees.append([l])
        elif entrees:
            entrees[-1].append(l)
        else:
            entete.append(l)
    return ''.join(entete), [''.join(e) for e in entrees]


def compter_entrees(texte):
    return sum(1 for l in texte.splitlines() if RE_ENTREE.match(l))


def date_entree(entree):
    m = RE_DATE.match(entree)
    if not m:
        return None
    try:
        return datetime.date(int(m[1]), int(m[2]), int(m[3]))
    except ValueError:
        return None


# ---------------------------------------------------------------- TACHES.md
def cases(ligne):
    """Cases d'une ligne de tableau Markdown (sans les bords), ou None si ce n'est pas une ligne de tableau."""
    l = ligne.rstrip('\n')
    if not l.startswith('|') or not l.endswith('|'):
        return None
    return [c.strip() for c in l[1:-1].split('|')]


def est_ligne_ticket(ligne):
    """(ticket bien formé ?, clé) pour une ligne qui ressemble à un ticket ; (None, None) sinon."""
    c = cases(ligne)
    if c is None or len(c) < 2:
        return None, None
    cle = c[1]
    if not cle.startswith('WFS'):
        return None, None
    if len(c) != 10 or not RE_CLE.match(cle):
        return False, cle
    return True, cle


def termine(statut):
    s = statut.strip()
    return s.startswith('✅') or s.startswith('↪') or 'annulé' in s.lower() or 'abandonné' in s.lower()


def lignes_tableau_tickets(lignes):
    """Indices des lignes qui sont dans un tableau des tickets (en-tête « | Priorité | Clé | … » de 10 colonnes) ; les autres
    tableaux (épics, paliers du réalisme…) qui citent des tickets ne sont jamais touchés."""
    dedans, idx = False, set()
    for i, l in enumerate(lignes):
        c = cases(l)
        if c is None:
            dedans = False
            continue
        if len(c) == 10 and c[1] == 'Clé':
            dedans = True
            continue
        if dedans:
            idx.add(i)
    return idx


def compter_tickets(texte):
    lignes = texte.splitlines()
    return sum(1 for i in lignes_tableau_tickets(lignes) if est_ligne_ticket(lignes[i])[0] is not None)


# ---------------------------------------------------------------- ménage
def menage(depot, aujourdhui, maintenant):
    p_journal = os.path.join(depot, 'journal.md')
    p_taches = os.path.join(depot, 'TACHES.md')
    p_archive_t = os.path.join(depot, 'archive', 'taches-terminees.md')
    d_archive_j = os.path.join(depot, 'journal', 'archive')
    journal, taches = lire(p_journal), lire(p_taches)
    archive_t = lire(p_archive_t) if os.path.exists(p_archive_t) else ''
    archives_j = {}
    if os.path.isdir(d_archive_j):
        for nom in sorted(os.listdir(d_archive_j)):
            if nom.endswith('.md'):
                archives_j[nom] = lire(os.path.join(d_archive_j, nom))
    avant_j = compter_entrees(journal) + sum(compter_entrees(t) for t in archives_j.values())
    avant_t = compter_tickets(taches) + compter_tickets(archive_t)

    a_corriger = []
    # journal : entrées de plus de 2 jours (on garde aujourd'hui et hier)
    entete, entrees = decouper_journal(journal)
    limite = aujourdhui - datetime.timedelta(days=1)
    gardees, deplacees = [], {}
    for e in entrees:
        d = date_entree(e)
        if d is None:
            a_corriger.append('journal : entrée sans date lisible : « %s »' % e.splitlines()[0][:80])
            gardees.append(e)
        elif d < limite:
            deplacees.setdefault(d.isoformat(), []).append(e)
        else:
            gardees.append(e)
    nouveaux_j = dict(archives_j)
    for jour, es in deplacees.items():
        nom = jour + '.md'
        base = nouveaux_j.get(nom, ENTETE_JOUR.format(d=jour))
        if base and not base.endswith('\n'):
            base += '\n'
        corps = ''.join(x if x.endswith('\n') else x + '\n' for x in es)
        nouveaux_j[nom] = base + corps
    n_entrees = sum(len(es) for es in deplacees.values())

    # TACHES.md : lignes de tickets terminés, annulés ou abandonnés
    lignes = taches.splitlines(keepends=True)
    dans_tickets = lignes_tableau_tickets(lignes)
    restantes, sorties, regles = [], [], {}
    for i, l in enumerate(lignes):
        ok, cle = est_ligne_ticket(l) if i in dans_tickets else (None, None)
        if ok is False:
            a_corriger.append('TACHES.md : ligne de ticket illisible (%s) : « %s »' % (cle, l.strip()[:80]))
            restantes.append(l)
            continue
        if ok and termine(cases(l)[4]):
            sorties.append(l if l.endswith('\n') else l + '\n')
            p_regle = os.path.join(depot, 'regles-de-gestion', cle + '.md')
            texte_r = regles.get(p_regle, lire(p_regle) if os.path.exists(p_regle) else '# %s\n' % cle)
            if SECTION_SUIVI not in texte_r:
                if not texte_r.endswith('\n'):
                    texte_r += '\n'
                texte_r += '\n' + SECTION_SUIVI + '\n' + sorties[-1]
            regles[p_regle] = texte_r
            continue
        restantes.append(l)
    nouveau_taches = ''.join(restantes)
    nouvelle_archive_t = archive_t
    if sorties:
        if not nouvelle_archive_t.strip():
            nouvelle_archive_t = ENTETE_ARCHIVE_T
        if nouvelle_archive_t and not nouvelle_archive_t.endswith('\n'):
            nouvelle_archive_t += '\n'
        nouvelle_archive_t += ''.join(sorties)

    # entrée du robot au journal (RG-07) : seulement si quelque chose a bougé ou doit être corrigé
    nouveau_journal = entete + ''.join(gardees)
    entree_robot = 0
    if n_entrees or sorties or a_corriger:
        lignes_r = ['### %s — Archives automatiques (robot archives)\n' % maintenant,
                    '- Journal : %d entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (%s).\n'
                    % (n_entrees, ', '.join(sorted(deplacees)) or 'aucun jour'),
                    '- TACHES.md : %d ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md`%s.\n'
                    % (len(sorties), (' (' + ', '.join(cases(l)[1] for l in sorties) + ')') if sorties else '')]
        if a_corriger:
            lignes_r.append('- À corriger par le PO (laissé en place, rien deviné) :\n')
            lignes_r += ['  - %s\n' % x for x in a_corriger]
        if not nouveau_journal.endswith('\n'):
            nouveau_journal += '\n'
        nouveau_journal += '\n' + ''.join(lignes_r)
        entree_robot = 1

    # RG-05 : comptage avant / après (l'entrée du robot mise à part)
    apres_j = compter_entrees(nouveau_journal) - entree_robot + sum(compter_entrees(t) for t in nouveaux_j.values())
    apres_t = compter_tickets(nouveau_taches) + compter_tickets(nouvelle_archive_t)
    if apres_j != avant_j or apres_t != avant_t:
        print('archives : ÉCART DE COMPTAGE, rien n\'est écrit : entrées du journal %d → %d, lignes de tickets %d → %d'
              % (avant_j, apres_j, avant_t, apres_t))
        return 1

    if not (n_entrees or sorties or a_corriger):
        print('archives : rien à ranger (entrées %d, lignes de tickets %d)' % (avant_j, avant_t))
        return 0
    ecrire(p_journal, nouveau_journal)
    for nom, texte in nouveaux_j.items():
        if archives_j.get(nom) != texte:
            ecrire(os.path.join(d_archive_j, nom), texte)
    if sorties:
        ecrire(p_taches, nouveau_taches)
        ecrire(p_archive_t, nouvelle_archive_t)
        for p, texte in regles.items():
            ecrire(p, texte)
    print('archives : %d entrée(s) et %d ligne(s) de tickets rangées, %d point(s) à corriger ; comptages égaux (entrées %d, lignes %d)'
          % (n_entrees, len(sorties), len(a_corriger), avant_j, avant_t))
    return 0


def main():
    ap = argparse.ArgumentParser(description='Robot des archives (WFS-198)')
    ap.add_argument('--depot', default='.')
    ap.add_argument('--aujourdhui', help='date UTC du run (AAAA-MM-JJ), par défaut aujourd\'hui')
    ap.add_argument('--maintenant', help='date et heure UTC de l\'entrée du journal, par défaut maintenant')
    a = ap.parse_args()
    now = datetime.datetime.now(datetime.timezone.utc)
    aujourdhui = datetime.date.fromisoformat(a.aujourdhui) if a.aujourdhui else now.date()
    maintenant = a.maintenant or now.strftime('%Y-%m-%d %H:%M:%S')
    return menage(a.depot, aujourdhui, maintenant)


if __name__ == '__main__':
    sys.exit(main())
