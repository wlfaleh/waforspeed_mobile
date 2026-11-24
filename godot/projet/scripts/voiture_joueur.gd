# WaForSpeed — prototype Godot (WFS-610) : voiture du joueur (BMW M4 du jeu actuel, assets/cars/m4.wfm converti).
# Modèle « bicyclette » à vitesse variable : puissance et masse de la fiche (510 ch, 1 725 kg, empattement 2,85 m),
# adhérence limitée (accélération latérale ≤ 1,0 g), braquage réduit avec la vitesse, freinage ≈ 1 g, traînée de l'air.
# Calculée à chaque image (_process, pas fixe : aucun à-coup entre physique et affichage), roues qui tournent et braquent.
class_name VoitureJoueur
extends Node3D

const MASSE := 1725.0
const PUISSANCE := 375000.0      # W (510 ch)
const EMPATTEMENT := 2.85
const RAYON_ROUE := 0.34
const G := 9.81
const ADHERENCE := 1.0           # g
const CX_S := 0.68               # Cx × surface frontale (m²)

var tr: Trace
var vitesse := 0.0               # m/s, avance
var cap := 0.0                   # rad, même sens que le tracé
var braquage := 0.0              # rad, roues avant
var indice := 0                  # point du tracé le plus proche
var gaz := 0.0
var frein := 0.0
var volant := 0.0                # −1 (gauche) … 1 (droite)
var roues := {}
var rotation_roues := 0.0

func preparer(trace: Trace, modele: PackedScene, couleur: Color) -> void:
	tr = trace
	var m := modele.instantiate()
	add_child(m)
	for nom in ["roue_av_g", "roue_av_d", "roue_ar_g", "roue_ar_d"]:
		var r := m.find_child(nom, true, false)
		if r:
			roues[nom] = r
	_peindre(m, couleur)
	var e := tr.echantillon(20.0, 3.75)   # voie de droite, 20 m après le départ
	position = e["pos"]
	cap = e["cap"]

static func _peindre(n: Node, couleur: Color) -> void:
	for c in n.get_children():
		_peindre(c, couleur)
	if n is MeshInstance3D:
		var mi := n as MeshInstance3D
		for s in mi.mesh.get_surface_count():
			var mat := mi.mesh.surface_get_material(s)
			if mat and mat.resource_name == "peinture":
				var p := (mat as StandardMaterial3D).duplicate() as StandardMaterial3D
				p.albedo_color = couleur
				p.metallic = 0.45
				p.roughness = 0.32
				p.clearcoat_enabled = true
				p.clearcoat = 0.8
				mi.set_surface_override_material(s, p)

func _process(dt: float) -> void:
	dt = minf(dt, 0.05)
	# ---- moteur, freins, air
	var v := absf(vitesse)
	var traction := gaz * minf(ADHERENCE * G, PUISSANCE / (MASSE * maxf(v, 3.0)))
	var air := CX_S * 0.6 * v * v / MASSE
	var roulement := 0.15 + 0.012 * v
	if vitesse > 0.5 or frein == 0.0:
		var acc := traction - air * signf(vitesse) - roulement * signf(vitesse) - frein * 0.95 * G
		vitesse += acc * dt
		if frein > 0.0 and vitesse < 0.0:
			vitesse = 0.0
		if gaz == 0.0 and absf(vitesse) < 0.3:
			vitesse = 0.0
	else:   # frein tenu à l'arrêt : marche arrière lente
		vitesse = maxf(vitesse - frein * 3.0 * dt, -6.0)
	# ---- direction : braquage maxi réduit avec la vitesse, suivi doux du volant (3,5 rad/s)
	var maxi := lerpf(0.55, 0.07, clampf(v / 60.0, 0.0, 1.0))
	braquage = move_toward(braquage, volant * maxi, 3.5 * maxi * dt * 2.0)
	var lacet := vitesse * tan(braquage) / EMPATTEMENT
	var lacet_max := ADHERENCE * G / maxf(v, 1.0)          # au-delà, les pneus glissent : le virage s'élargit
	lacet = clampf(lacet, -lacet_max, lacet_max)
	cap += lacet * dt
	position += Trace.avant_de(cap) * vitesse * dt
	# ---- sol et bordures : hauteur du tracé, trottoirs infranchissables (bordure = choc, perte de vitesse)
	indice = tr.plus_proche(position, indice)
	var p := tr.points[indice]
	var droite := Trace.droite_de(tr.caps[indice])
	var lat := (position - p).dot(droite)
	var limite := tr.demi_largeur - 1.0
	if absf(lat) > limite:
		position -= droite * (absf(lat) - limite) * signf(lat)
		vitesse *= 0.97
	position.y = lerpf(position.y, p.y, minf(1.0, dt * 10.0))
	rotation = Vector3(0.0, -cap, 0.0)
	# ---- roues
	rotation_roues -= vitesse / RAYON_ROUE * dt
	for nom in roues:
		var r: Node3D = roues[nom]
		r.rotation = Vector3(rotation_roues, -braquage if nom.begins_with("roue_av") else 0.0, 0.0)

func distance_parcourue() -> float:
	return indice * Trace.PAS
