#!/usr/bin/env bash
# WaForSpeed — publication de la Release d'aperçu (WFS-87, WFS-106), appelée par les jobs « publication-apk »
# (PUBLIER=apk) et « publication-captures » (PUBLIER=captures) de build.yml ; PUBLIER vide : les deux à la suite (essai local).
# Main : Release « apercu » ; branche feature/XX : « apercu-XX » (claude/XX : idem). Adresses inchangées pour la QA :
#   https://github.com/<dépôt>/releases/download/<tag>/WaForSpeed.apk et …/<capture>.png
# Règles :
#   - jamais de moment sans APK : le nouvel APK (et l'AAB) est envoyé sous un nom provisoire, puis l'ancien est supprimé
#     et le nouveau renommé (deux appels courts) ; la Release n'est jamais supprimée (WFS-87) ;
#   - jamais un fichier d'un commit plus récent remplacé par celui d'un commit plus ancien : l'APK est jugé sur le commit
#     de la Release (target_commitish), les captures sur celui du fichier d'empreintes (WFS-106 RG-02) ;
#   - chaque appel : 300 s au plus, puis 4 nouvelles tentatives (15, 30, 60, 120 s) ; limite secondaire de l'API : 60 s,
#     puis le double ; quota épuisé : l'APK attend la remise à zéro, les captures s'arrêtent (RG-07, RG-11) ;
#   - APK non publié : job rouge ; captures non publiées : avertissement, job vert (RG-03) ;
#   - le mode captures ne touche ni à WaForSpeed.apk / .aab (noms provisoires compris), ni au tag, ni au titre ; le mode
#     APK ne touche ni aux captures, ni aux notes (RG-06) ;
#   - captures : seules les nouvelles ou modifiées partent, comparées au fichier d'empreintes (sha256) de la Release, lu
#     par l'adresse publique (hors quota) ; les 200 derniers appels de l'heure restent aux APK (RG-11).
# Entrées : GH_TOKEN, GH_REPO, GITHUB_REF_NAME, GITHUB_SHA, GITHUB_RUN_NUMBER ; dossiers dl/apk et dl/captures
# (artefacts) ; BANC : résultat du banc (les captures périmées ne sont retirées que s'il a réussi).
# shellcheck disable=SC2317   # fonctions appelées par « essayer »
set -uo pipefail

PUBLIER="${PUBLIER:-}"
APK_DIR="${APK_DIR:-dl/apk}"
CAPTURES_DIR="${CAPTURES_DIR:-dl/captures}"
LOT="${LOT:-20}"                             # captures par appel d'envoi
RESERVE="${RESERVE:-200}"                    # RG-11 : appels de l'heure gardés pour les APK
APPELS_APK=15                                # RG-11 : appels d'une publication de l'APK, au plus
DELAI_GH="${DELAI_GH:-300}"                  # RG-07 : durée maximale d'un appel
ATTENTES=(15 30 60 120)                      # nouvelles tentatives
[ -n "${ATTENTES_ESSAI:-}" ] && read -r -a ATTENTES <<< "$ATTENTES_ESSAI"   # essais locaux : attentes courtes
SECONDAIRE="${SECONDAIRE:-60}"               # limite secondaire : première attente, doublée à chaque échec
ATTENTE_QUOTA_MIN="${ATTENTE_QUOTA_MIN:-30}" # quota épuisé : attente minimale avant un nouvel essai
ATTENTE_RELEASE="${ATTENTE_RELEASE:-600}"    # captures : attente de la Release créée par le job de l'APK
DUREE_MAX="${DUREE_MAX:-$([ "$PUBLIER" = captures ] && echo 1080 || echo 3900)}"   # sous timeout-minutes (20 et 70 min)
ECHEANCE=$(( $(date +%s) + DUREE_MAX ))
SERVEUR="${GITHUB_SERVER_URL:-https://github.com}"
MODE=""
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
APPELS_F="$TMP/appels"; : > "$APPELS_F"

# ---------------------------------------------------------------- outils
# gh : durée maximale par appel (RG-07), jamais au-delà de l'échéance du job ; appels à l'API comptés (GH_COUT : appels
# faits par la commande, 1 par défaut ; « gh api rate_limit » ne compte pas dans le quota)
ECRIT=0   # 1 dès la première écriture du jeton (envoi, suppression, modification) : voir lire_quota
gh() {
    local cout="${GH_COUT:-1}" reste
    [ "${1:-} ${2:-}" = "api rate_limit" ] && cout=0
    [[ "${1:-} ${2:-}" == "release upload" || " $* " =~ " -X "(DELETE|PATCH|POST)" " ]] && ECRIT=1
    echo "$cout" >> "$APPELS_F"
    reste=$(( ECHEANCE - $(date +%s) )); [ "$reste" -gt "$DELAI_GH" ] && reste=$DELAI_GH; [ "$reste" -lt 10 ] && reste=10
    timeout "$reste" gh "$@"
}
appels() { awk '{ s += $1 } END { print s + 0 }' "$APPELS_F"; }
heure() { date -u -d "@$1" +%H:%M:%S; }

QUOTA_RESTANT=""; QUOTA_REMISE=""; QUOTA_LIMITE=""
# quota horaire de GITHUB_TOKEN, commun à tous les jobs du dépôt (403 « API rate limit exceeded for installation »), lu
# sans le consommer. Juste seulement après la première écriture du jeton : avant, « rate_limit » donne un autre compteur,
# presque immobile (≈ 4 980 restants sur 5 000), même quand le dépôt n'en a plus que 220 (mesuré le 2026-11-05) ; il
# n'est donc écrit au journal qu'après une écriture. Un 403 sur un appel fait de toute façon attendre ou arrêter (essayer).
lire_quota() {
    local s
    s=$(gh api rate_limit --jq '"\(.resources.core.remaining) \(.resources.core.reset) \(.resources.core.limit)"' 2>/dev/null) || return 1
    read -r QUOTA_RESTANT QUOTA_REMISE QUOTA_LIMITE <<< "$s"
    [[ "$QUOTA_RESTANT" =~ ^[0-9]+$ && "$QUOTA_REMISE" =~ ^[0-9]+$ ]]
}
quota_fiable() { [ "$ECRIT" = 1 ] && lire_quota; }
afficher_quota() { echo "Quota de l'API : $QUOTA_RESTANT appels restants sur ${QUOTA_LIMITE:-?} (remise à zéro à $(heure "$QUOTA_REMISE") UTC)"; }
attendre_remise() {   # RG-11 (APK) : attente de la remise à zéro du quota, écrite au journal du job
    local maintenant attente
    lire_quota || { QUOTA_RESTANT="?"; QUOTA_REMISE=$(( $(date +%s) + 60 )); }
    maintenant=$(date +%s); attente=$(( QUOTA_REMISE - maintenant + 5 ))
    [ "$attente" -lt "$ATTENTE_QUOTA_MIN" ] && attente=$ATTENTE_QUOTA_MIN
    if [ $(( maintenant + attente )) -gt "$ECHEANCE" ]; then
        echo "remise à zéro du quota à $(heure "$QUOTA_REMISE") UTC : au-delà du délai maximal du job"; return 1
    fi
    echo "Quota de l'API épuisé ($QUOTA_RESTANT appels restants) : attente de la remise à zéro, à $(heure "$QUOTA_REMISE") UTC ($attente s)"
    sleep "$attente"
}

# essayer <description> <commande…> : 1 essai + 4 nouvelles tentatives ; limite secondaire : 60 s, puis le double ;
# quota épuisé : APK = attente de la remise à zéro (RG-11), captures = code 3 (fin de l'envoi des captures)
essayer() {
    local desc="$1"; shift
    local n=0 secondaire="$SECONDAIRE" attente code sortie="$TMP/sortie"
    while true; do
        "$@" > "$sortie" 2>&1; code=$?
        cat "$sortie"
        [ "$code" -eq 0 ] && return 0
        [ "$code" -eq 124 ] && echo "délai dépassé : $desc"
        if grep -qiE "secondary rate limit|abuse detection|HTTP 429|too many requests" "$sortie"; then
            attente=$secondaire; secondaire=$(( secondaire * 2 ))
            echo "limite secondaire de l'API : nouvel essai dans $attente s : $desc"
        elif grep -qi "rate limit exceeded" "$sortie"; then
            [ "$MODE" = captures ] && { echo "quota de l'API épuisé : $desc"; return 3; }
            attendre_remise || { echo "ÉCHEC : $desc (quota de l'API épuisé)"; return 1; }
            continue
        elif [ "$n" -ge "${#ATTENTES[@]}" ]; then
            echo "ÉCHEC : $desc (après $n nouvelles tentatives)"; return 1
        else
            attente=${ATTENTES[$n]}; n=$(( n + 1 ))
            echo "nouvelle tentative $n/${#ATTENTES[@]} dans $attente s : $desc"
        fi
        if [ $(( $(date +%s) + attente )) -gt "$ECHEANCE" ]; then echo "ÉCHEC : $desc (délai maximal du job atteint)"; return 1; fi
        sleep "$attente"
    done
}

# ---------------------------------------------------------------- Release
tag=apercu
case "$GITHUB_REF_NAME" in feature/*) tag="apercu-${GITHUB_REF_NAME#feature/}";; claude/*) tag="apercu-${GITHUB_REF_NAME#claude/}";; esac
titre="Aperçu — construction ${GITHUB_RUN_NUMBER} (${GITHUB_SHA::7})"   # RG-04 : commit de l'APK
EMPREINTES="empreintes-${GITHUB_RUN_NUMBER}-${GITHUB_RUN_ATTEMPT:-1}.sha256"   # nom unique : lu sans cache
notes() {   # RG-04 : $1 = construction et commit d'où viennent les captures
    printf '%s\n\n%s\n' "APK Android de test (arm64). Installer : télécharger WaForSpeed.apk sur le téléphone, autoriser l'installation depuis cette source. Branche ${GITHUB_REF_NAME} ; commit de l'APK dans le titre. Signature fixe (WFS-118) : si ton téléphone refuse l'installation (« conflit de package »), désinstalle une dernière fois l'ancienne version ; ensuite chaque nouvel APK s'installe par-dessus et garde ta sauvegarde." \
        "Captures du banc d'essai : $1."
}

RID=""; CIBLE=""; existe=0
declare -A ID=()   # fichier de la Release → identifiant
lire_release() {   # 1 appel : identifiant, commit de l'APK (target_commitish) et fichiers
    local f="$TMP/release" id nom
    if gh api "repos/{owner}/{repo}/releases/tags/$tag" --jq '"\(.id)\t\(.target_commitish)", (.assets[] | "\(.id)\t\(.name)")' > "$f" 2> "$f.err"; then
        existe=1; ID=()
        IFS=$'\t' read -r RID CIBLE < "$f"
        while IFS=$'\t' read -r id nom; do [ -n "$nom" ] && ID["$nom"]=$id; done < <(tail -n +2 "$f")
        return 0
    fi
    if grep -qiE "HTTP 404|not found" "$f" "$f.err"; then existe=0; RID=""; CIBLE=""; ID=(); return 0; fi   # pas encore de Release
    cat "$f" "$f.err"; return 1
}
creer_release() {   # 1 appel ; déjà créée (essai précédent) : succès, la Release est relue ensuite
    local f="$TMP/creation"
    gh api -X POST "repos/{owner}/{repo}/releases" -f tag_name="$tag" -f target_commitish="$GITHUB_SHA" -f name="$titre" \
        -f body="$(notes "en cours de publication")" -F prerelease=true --silent > "$f" 2>&1 && return 0
    grep -qi "already.exists" "$f" && return 0
    cat "$f"; return 1
}
supprimer() {   # supprimer <identifiant> : 1 appel ; déjà absent (essai précédent) : succès
    local f="$TMP/suppression"
    gh api -X DELETE "repos/{owner}/{repo}/releases/assets/$1" --silent > "$f" 2>&1 && return 0
    grep -qiE "HTTP 404|not found" "$f" && return 0
    cat "$f"; return 1
}
renommer() { gh api -X PATCH "repos/{owner}/{repo}/releases/assets/$1" -f name="$2" --silent; }   # 1 appel
deplacer_tag() { git tag -f "$tag" "$GITHUB_SHA" > /dev/null && git push -f origin "refs/tags/$tag"; }   # git, hors quota
maj_titre() { gh api -X PATCH "repos/{owner}/{repo}/releases/$RID" -f name="$titre" -f target_commitish="$GITHUB_SHA" --silent; }
maj_notes() { gh api -X PATCH "repos/{owner}/{repo}/releases/$RID" -f body="$(notes "$1")" --silent; }
plus_recent() {   # plus_recent <commit> : vrai si <commit> (un SHA, jamais un nom de branche) est un descendant strict de celui de ce run
    [[ "$1" =~ ^[0-9a-f]{7,40}$ ]] && [ "$1" != "$GITHUB_SHA" ] && git merge-base --is-ancestor "$GITHUB_SHA" "$1" 2>/dev/null
}

# ---------------------------------------------------------------- APK (RG-05) : au plus 15 appels
publier_apk() {
    MODE=apk
    [ -s "$APK_DIR/WaForSpeed.apk" ] || { echo "::error::$APK_DIR/WaForSpeed.apk absent ou vide"; exit 1; }
    local paquets=("$APK_DIR/WaForSpeed.apk") envois=() p nom prov existants=0
    [ -s "$APK_DIR/WaForSpeed.aab" ] && paquets+=("$APK_DIR/WaForSpeed.aab")   # WFS-22 : AAB signé (Google Play)
    echo "APK : Release $tag, commit ${GITHUB_SHA::7} (construction ${GITHUB_RUN_NUMBER})"
    essayer "lecture de la Release $tag" lire_release || { echo "::error::APK non publié : Release $tag illisible"; exit 1; }
    # garde : quota presque nul (avant toute écriture, le relevé peut être trop haut : un 403 fait alors attendre, essayer)
    if lire_quota && [ "$QUOTA_RESTANT" -lt "$APPELS_APK" ]; then
        attendre_remise || { echo "::error::APK non publié : quota de l'API épuisé"; exit 1; }
        essayer "lecture de la Release $tag" lire_release || { echo "::error::APK non publié : Release $tag illisible"; exit 1; }
    fi
    if [ "$existe" = 0 ]; then   # première publication de la branche : Release créée sans fichier, puis même chemin
        { essayer "création de la Release $tag" creer_release && essayer "lecture de la Release $tag" lire_release && [ "$existe" = 1 ]; } \
            || { echo "::error::APK non publié : création de la Release $tag impossible"; exit 1; }
        echo "Release $tag créée"
    elif plus_recent "$CIBLE"; then   # RG-02 / RG-03 : pas une erreur
        echo "APK : la Release $tag a déjà l'APK d'un commit plus récent (${CIBLE::7}) : rien à publier pour ${GITHUB_SHA::7}"; return 0
    fi
    # 1) les nouveaux paquets sous un nom provisoire, envoyés ensemble (l'ancien APK reste téléchargeable pendant ce temps)
    for p in "${paquets[@]}"; do
        nom=$(basename "$p"); prov="${nom%.*}-${GITHUB_SHA::7}.${nom##*.}"
        cp "$p" "$TMP/$prov"; envois+=("$TMP/$prov")
        [ -n "${ID[$prov]:-}" ] && existants=$(( existants + 1 ))   # nouvel essai du même commit : remplacé (--clobber)
    done
    GH_COUT=$(( 1 + ${#envois[@]} + existants )) essayer "envoi de l'APK sous un nom provisoire" gh release upload "$tag" "${envois[@]}" --clobber \
        || { echo "::error::APK non publié (envoi impossible)"; exit 1; }
    essayer "lecture de la Release $tag" lire_release || { echo "::error::APK non publié : Release $tag illisible après l'envoi"; exit 1; }
    # 2) l'APK puis l'AAB : l'ancien supprimé, le nouveau renommé (deux appels courts)
    for p in "${paquets[@]}"; do
        nom=$(basename "$p"); prov="${nom%.*}-${GITHUB_SHA::7}.${nom##*.}"
        [ -n "${ID[$prov]:-}" ] || { echo "::error::$nom non publié : $prov absent de la Release après l'envoi"; exit 1; }
        if [ -n "${ID[$nom]:-}" ]; then
            essayer "suppression de l'ancien $nom" supprimer "${ID[$nom]}" || { echo "::error::ancien $nom non remplacé"; exit 1; }
        fi
        essayer "renommage de $prov en $nom" renommer "${ID[$prov]}" "$nom" || { echo "::error::$nom non publié (renommage impossible)"; exit 1; }
        echo "$nom publié ($(date -u +%H:%M:%S) UTC)"
    done
    # 3) fichiers provisoires d'une publication interrompue (autre commit) retirés
    for nom in "${!ID[@]}"; do
        case "$nom" in WaForSpeed-*.apk|WaForSpeed-*.aab) ;; *) continue ;; esac
        [[ "$nom" == *"-${GITHUB_SHA::7}."* ]] && continue
        essayer "retrait du fichier provisoire $nom" supprimer "${ID[$nom]}" || echo "::warning::$nom non retiré"
    done
    # 4) tag déplacé sur le commit, titre et commit de la Release (garde RG-02) ; les notes restent au job des captures
    essayer "déplacement du tag $tag" deplacer_tag || echo "::warning::tag $tag non déplacé sur ${GITHUB_SHA::7}"
    essayer "titre de la Release $tag" maj_titre || echo "::warning::titre de la Release $tag non mis à jour"
    local reste=""; quota_fiable && reste=" ; quota restant : $QUOTA_RESTANT sur ${QUOTA_LIMITE:-?}"
    echo "APK en ligne : $SERVEUR/$GH_REPO/releases/download/$tag/WaForSpeed.apk (« $titre », $(appels) appels à l'API$reste)"
}

# ---------------------------------------------------------------- captures (RG-06, RG-11)
telecharger() {   # telecharger <nom> <fichier> : adresse publique de la Release, hors quota de l'API
    curl -fsSL --retry 2 --max-time 60 -o "$2" "$SERVEUR/$GH_REPO/releases/download/$tag/$1"
}
ecrire_empreintes() {   # ce qui est vraiment en ligne (FINAL du mode captures), puis anciens fichiers d'empreintes retirés
    local n cout=2
    {
        echo "# WaForSpeed — empreintes sha256 des captures en ligne (WFS-106, tools/qa/publier_apercu.sh)"
        echo "# commit $GITHUB_SHA construction $GITHUB_RUN_NUMBER"
        for n in "${!FINAL[@]}"; do echo "${FINAL[$n]}  $n"; done | sort -k 2
    } > "$TMP/$EMPREINTES"
    [ -n "${ID[$EMPREINTES]:-}" ] && cout=3
    GH_COUT=$cout essayer "envoi du fichier d'empreintes $EMPREINTES" gh release upload "$tag" "$TMP/$EMPREINTES" --clobber || return 1
    ID["$EMPREINTES"]=${ID[$EMPREINTES]:-nouveau}
    for n in "${!ID[@]}"; do
        [[ "$n" == empreintes-*.sha256 && "$n" != "$EMPREINTES" ]] || continue
        if essayer "retrait de l'ancien fichier d'empreintes $n" supprimer "${ID[$n]}"; then unset "ID[$n]"; else echo "::warning::$n non retiré"; fi
    done
}

publier_captures() {
    MODE=captures
    local captures=() envoyer=() noms=() ratees=() non_tentees=() perimees=() lot=()
    local f n h i code cout existants fin etat raison reste="" empreintes="" commit_e="" id_max=0 inchangees=0 envoyees=0 retirees=0 a_jour=0 arret=""
    declare -A LOCAL=() EN_LIGNE=() FINAL=()
    if [ -d "$CAPTURES_DIR" ]; then
        for f in "$CAPTURES_DIR"/*.png "$CAPTURES_DIR"/*.txt; do [ -s "$f" ] && captures+=("$f"); done
    fi
    echo "Captures : Release $tag, commit ${GITHUB_SHA::7} (construction ${GITHUB_RUN_NUMBER}), banc : ${BANC:-?}"
    if [ ${#captures[@]} -eq 0 ]; then echo "::warning::aucune capture à publier (banc : ${BANC:-?})"; return 0; fi
    while read -r h f; do LOCAL["$(basename "$f")"]=$h; done < <(sha256sum "${captures[@]}")
    essayer "lecture de la Release $tag" lire_release || { echo "::warning::captures non publiées : Release $tag illisible"; return 0; }
    if [ "$existe" = 0 ]; then   # première publication de la branche : la Release vient du job de l'APK (attente hors quota)
        echo "Release $tag absente : attente de l'APK (job publication-apk), $ATTENTE_RELEASE s au plus"
        fin=$(( $(date +%s) + ATTENTE_RELEASE ))
        until curl -fsI --max-time 30 -o /dev/null "$SERVEUR/$GH_REPO/releases/download/$tag/WaForSpeed.apk" 2>/dev/null; do
            [ "$(date +%s)" -ge "$fin" ] && break
            sleep 20
        done
        essayer "lecture de la Release $tag" lire_release || true
        if [ "$existe" = 0 ]; then
            echo "::warning::captures non publiées : la Release $tag n'existe pas (APK non publié ?) ; elles restent dans l'artefact du run"; return 0
        fi
    fi
    # empreintes en ligne : le fichier empreintes-*.sha256 le plus récent ; sans lui (première publication depuis WFS-106),
    # les captures en ligne sont téléchargées pour comparaison ; empreinte inconnue = capture envoyée
    for n in "${!ID[@]}"; do
        if [[ "$n" == empreintes-*.sha256 && "${ID[$n]}" =~ ^[0-9]+$ ]] && [ "${ID[$n]}" -gt "$id_max" ]; then empreintes=$n; id_max=${ID[$n]}; fi
    done
    if [ -n "$empreintes" ] && telecharger "$empreintes" "$TMP/en-ligne.sha256"; then
        commit_e=$(sed -n 's/^# commit \([0-9a-f]\{7,40\}\).*/\1/p' "$TMP/en-ligne.sha256" | head -n 1)
        while read -r h n; do [ -n "$n" ] && [ -n "${ID[$n]:-}" ] && EN_LIGNE["$n"]=$h; done < <(grep -v '^#' "$TMP/en-ligne.sha256")
        echo "Empreintes en ligne : $empreintes (commit ${commit_e::7}, ${#EN_LIGNE[@]} fichiers)"
    else
        for n in "${!LOCAL[@]}"; do [ -n "${ID[$n]:-}" ] && noms+=("$n"); done
        echo "Pas de fichier d'empreintes lisible : ${#noms[@]} captures en ligne téléchargées pour comparaison"
        if [ ${#noms[@]} -gt 0 ]; then
            mkdir -p "$TMP/en-ligne"
            printf '%s\n' "${noms[@]}" | xargs -P 8 -I '{}' curl -fsSL --retry 2 --max-time 60 -o "$TMP/en-ligne/{}" \
                "$SERVEUR/$GH_REPO/releases/download/$tag/{}" 2>/dev/null
            for n in "${noms[@]}"; do [ -s "$TMP/en-ligne/$n" ] && EN_LIGNE["$n"]=$(sha256sum < "$TMP/en-ligne/$n" | cut -d ' ' -f 1); done
        fi
    fi
    if plus_recent "$commit_e"; then   # RG-02 / RG-03 : pas une erreur
        echo "Captures : celles en ligne viennent déjà d'un commit plus récent (${commit_e::7}) : rien à publier pour ${GITHUB_SHA::7}"; return 0
    fi
    for n in "${!EN_LIGNE[@]}"; do FINAL["$n"]=${EN_LIGNE[$n]}; done
    for f in "${captures[@]}"; do
        n=$(basename "$f")
        if [ -n "${ID[$n]:-}" ] && [ "${EN_LIGNE[$n]:-}" = "${LOCAL[$n]}" ]; then inchangees=$(( inchangees + 1 )); else envoyer+=("$f"); unset "FINAL[$n]"; fi
    done
    echo "Captures : ${#captures[@]} au total, ${#envoyer[@]} nouvelles ou modifiées, $inchangees inchangées"
    # avant l'envoi, le fichier d'empreintes ne garde que les captures déjà à jour : il dit toujours ce qui est en ligne
    if [ ${#envoyer[@]} -gt 0 ]; then
        ecrire_empreintes || { echo "::warning::captures non publiées : fichier d'empreintes non envoyé ; elles restent dans l'artefact du run"; return 0; }
    fi
    for (( i = 0; i < ${#envoyer[@]}; i += LOT )); do
        lot=("${envoyer[@]:i:LOT}")
        existants=0; for f in "${lot[@]}"; do [ -n "${ID[$(basename "$f")]:-}" ] && existants=$(( existants + 1 )); done
        cout=$(( 1 + ${#lot[@]} + existants ))   # lecture de la Release, envois, anciennes versions supprimées (--clobber)
        if [ -z "$arret" ]; then
            if quota_fiable; then   # après l'envoi du fichier d'empreintes : quota réel du dépôt (voir lire_quota)
                [ "$i" -eq 0 ] && afficher_quota
                [ $(( QUOTA_RESTANT - cout )) -lt "$RESERVE" ] && arret="quota de l'API presque épuisé : $QUOTA_RESTANT appels restants, les $RESERVE derniers sont gardés pour les APK"
            fi
            if [ -z "$arret" ] && [ $(( $(date +%s) + 120 )) -gt "$ECHEANCE" ]; then
                arret="délai maximal du job presque atteint"
            fi
        fi
        if [ -n "$arret" ]; then
            for f in "${lot[@]}"; do n=$(basename "$f"); non_tentees+=("$n"); [ -n "${EN_LIGNE[$n]:-}" ] && FINAL["$n"]=${EN_LIGNE[$n]}; done
            continue
        fi
        GH_COUT=$cout essayer "captures $(( i + 1 )) à $(( i + ${#lot[@]} )) sur ${#envoyer[@]}" gh release upload "$tag" "${lot[@]}" --clobber; code=$?
        for f in "${lot[@]}"; do
            n=$(basename "$f")
            if [ "$code" -eq 0 ]; then FINAL["$n"]=${LOCAL[$n]}; envoyees=$(( envoyees + 1 )); else ratees+=("$n"); fi
        done
        [ "$code" -eq 3 ] && arret="quota de l'API épuisé"
    done
    # captures périmées (absentes de ce banc) retirées, seulement si tout le banc a réussi (sinon : celles d'un job en échec)
    for n in "${!ID[@]}"; do
        case "$n" in WaForSpeed.apk|WaForSpeed.aab|WaForSpeed-*.apk|WaForSpeed-*.aab|empreintes-*.sha256) continue ;; esac
        [ -n "${LOCAL[$n]:-}" ] || perimees+=("$n")
    done
    if [ ${#perimees[@]} -gt 0 ]; then
        if [ "${BANC:-success}" != success ]; then
            echo "${#perimees[@]} capture(s) absente(s) de ce banc gardée(s) en ligne (banc : $BANC)"
        elif [ -n "$arret" ] || { lire_quota && [ $(( QUOTA_RESTANT - ${#perimees[@]} )) -lt "$RESERVE" ]; }; then
            raison=${arret:-"quota de l'API presque épuisé"}
            echo "::warning::${#perimees[@]} capture(s) périmée(s) gardée(s) en ligne ($raison)"
        else
            for n in "${perimees[@]}"; do
                if essayer "retrait de la capture périmée $n" supprimer "${ID[$n]}"; then unset "FINAL[$n]"; retirees=$(( retirees + 1 )); else echo "::warning::$n (périmée) non retirée"; fi
            done
        fi
    fi
    # fichier d'empreintes final, puis notes de la Release (RG-04 : construction et commit des captures)
    for f in "${captures[@]}"; do n=$(basename "$f"); [ "${FINAL[$n]:-}" = "${LOCAL[$n]}" ] && a_jour=$(( a_jour + 1 )); done
    if [ ${#envoyer[@]} -gt 0 ] || [ "$retirees" -gt 0 ] || [ "$commit_e" != "$GITHUB_SHA" ]; then
        ecrire_empreintes || echo "::warning::fichier d'empreintes final non envoyé : la prochaine publication renverra les captures non vérifiées"
        etat="construction ${GITHUB_RUN_NUMBER}, commit ${GITHUB_SHA::7} ($a_jour/${#captures[@]} à jour"
        [ $(( ${#ratees[@]} + ${#non_tentees[@]} )) -gt 0 ] && etat+=", $(( ${#ratees[@]} + ${#non_tentees[@]} )) seulement dans l'artefact du run"
        essayer "notes de la Release $tag" maj_notes "$etat)" || echo "::warning::notes de la Release $tag non mises à jour"
    else
        echo "Captures déjà publiées pour ce commit : rien à changer"
    fi
    [ ${#ratees[@]} -gt 0 ] && echo "::warning::${#ratees[@]} capture(s) non publiée(s) (envoi en échec) : ${ratees[*]}"
    [ ${#non_tentees[@]} -gt 0 ] && echo "::warning::${#non_tentees[@]} capture(s) non envoyée(s) ($arret) : elles restent dans l'artefact du run"
    quota_fiable && reste=" ; quota restant : $QUOTA_RESTANT sur ${QUOTA_LIMITE:-?}"
    echo "Captures : $envoyees envoyées, $inchangees inchangées, ${#ratees[@]} ratées, ${#non_tentees[@]} non envoyées, $retirees périmées retirées ; $(appels) appels à l'API$reste"
}

case "$PUBLIER" in
    apk) publier_apk ;;
    captures) publier_captures ;;
    "") publier_apk; publier_captures ;;
    *) echo "::error::PUBLIER=$PUBLIER inconnu (apk, captures ou vide)"; exit 1 ;;
esac
exit 0
