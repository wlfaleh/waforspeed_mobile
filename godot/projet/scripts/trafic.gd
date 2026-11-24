# WaForSpeed — prototype Godot (WFS-610) : trafic de la Ville. Les voitures sont toutes créées au chargement (aucune création
# pendant la course) et roulent sur leur voie à 35–50 km/h ; elles freinent derrière la voiture qui les précède (2 s d'écart).
# Apparition sans « pop » : au-delà de 300 m elles ne sont pas dessinées ; entre 300 et 360 m elles apparaissent en fondu
# (visibility range de Godot, mode SELF : tramage qui monte progressivement).
class_name Trafic
extends Node3D

const N := 48
const VISIBLE := 300.0
const FONDU := 60.0
const VOIES := [-5.6, -1.9, 1.9, 5.6]   # mètres à droite de l'axe ; négatif = sens inverse (2 voies par sens)

var tr: Trace
var voitures: Array = []   # {noeud, s, voie, v, vmax, roues, rot}

func preparer(trace: Trace, modeles: Array) -> void:
	tr = trace
	var alea := RandomNumberGenerator.new()
	alea.seed = 20261124
	var couleurs := [Color(0.85, 0.85, 0.86), Color(0.08, 0.08, 0.09), Color(0.45, 0.47, 0.5), Color(0.6, 0.08, 0.07),
					 Color(0.1, 0.2, 0.45), Color(0.75, 0.72, 0.66)]
	for i in N:
		var voie: float = VOIES[i % VOIES.size()]
		var s := 80.0 + alea.randf() * (tr.longueur - 160.0)
		var modele: PackedScene = modeles[alea.randi() % modeles.size()]
		var n := Node3D.new()
		var m := modele.instantiate()
		n.add_child(m)
		VoitureJoueur._peindre(m, couleurs[alea.randi() % couleurs.size()])
		_distance(m)
		add_child(n)
		var roues := []
		for nom in ["roue_av_g", "roue_av_d", "roue_ar_g", "roue_ar_d"]:
			var r := m.find_child(nom, true, false)
			if r:
				roues.append(r)
		var vmax := alea.randf_range(35.0, 50.0) / 3.6
		voitures.append({"noeud": n, "s": s, "voie": voie, "v": vmax, "vmax": vmax, "roues": roues, "rot": 0.0})
	voitures.sort_custom(func(a, b): return a["s"] < b["s"])

static func _distance(n: Node) -> void:
	for c in n.get_children():
		_distance(c)
	if n is GeometryInstance3D:
		var g := n as GeometryInstance3D
		g.visibility_range_end = VISIBLE + FONDU
		g.visibility_range_end_margin = FONDU
		g.visibility_range_fade_mode = GeometryInstance3D.VISIBILITY_RANGE_FADE_SELF

func _process(dt: float) -> void:
	dt = minf(dt, 0.05)
	for v in voitures:
		var sens := 1.0 if v["voie"] > 0.0 else -1.0
		# écart avec la voiture devant, même voie (liste triée par s ; quelques voisines suffisent)
		var ecart := INF
		for w in voitures:
			if w != v and w["voie"] == v["voie"]:
				var d: float = (w["s"] - v["s"]) * sens
				if d > 0.0 and d < ecart:
					ecart = d
		var cible: float = v["vmax"] if ecart > maxf(12.0, v["v"] * 2.0) else maxf(0.0, (ecart - 7.0) / 2.0)
		v["v"] = move_toward(v["v"], cible, (2.0 if cible > v["v"] else 6.0) * dt)
		v["s"] += v["v"] * dt * sens
		if v["s"] > tr.longueur - 20.0:
			v["s"] = 40.0
		elif v["s"] < 20.0:
			v["s"] = tr.longueur - 40.0
		var e := tr.echantillon(v["s"], v["voie"])
		var n: Node3D = v["noeud"]
		n.position = e["pos"]
		n.rotation = Vector3(0.0, -e["cap"] + (PI if sens < 0.0 else 0.0), 0.0)
		v["rot"] -= v["v"] / 0.32 * dt
		for r in v["roues"]:
			(r as Node3D).rotation = Vector3(v["rot"], 0.0, 0.0)
