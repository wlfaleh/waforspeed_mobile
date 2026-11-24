# WaForSpeed — prototype Godot (WFS-610) : tracé de la Ville (donnees/ville.json, tiré du code du jeu actuel par
# outils/extraire_trace.cpp) : un point tous les 2 m, x y z en mètres, cap th (x += sin th, z −= cos th le long du tracé).
class_name Trace
extends RefCounted

const PAS := 2.0
var points: PackedVector3Array = PackedVector3Array()
var caps: PackedFloat32Array = PackedFloat32Array()
var demi_largeur := 7.5
var longueur := 0.0

func charger(chemin: String) -> void:
	var texte := FileAccess.get_file_as_string(chemin)
	var d: Dictionary = JSON.parse_string(texte)
	demi_largeur = float(d["demi_largeur_m"])
	for p in d["points"]:
		points.append(Vector3(float(p[0]), float(p[1]), float(p[2])))
		caps.append(float(p[3]))
	longueur = (points.size() - 1) * PAS

static func avant_de(cap: float) -> Vector3:
	return Vector3(sin(cap), 0.0, -cos(cap))

static func droite_de(cap: float) -> Vector3:
	return Vector3(cos(cap), 0.0, sin(cap))

# position et cap à l'abscisse s (mètres), lat mètres à droite de l'axe ; interpolation linéaire entre deux points
func echantillon(s: float, lat: float = 0.0) -> Dictionary:
	s = clampf(s, 0.0, longueur - 0.001)
	var f := s / PAS
	var i := int(f)
	var t := f - i
	var a := points[i].lerp(points[i + 1], t)
	var cap := lerp_angle(caps[i], caps[i + 1], t)
	return {"pos": a + droite_de(cap) * lat, "cap": cap}

# indice du point le plus proche de p, cherché autour de l'indice précédent (la voiture avance de quelques mètres par image)
func plus_proche(p: Vector3, depart: int) -> int:
	var meilleur := depart
	var dmin := INF
	for i in range(maxi(0, depart - 30), mini(points.size(), depart + 30)):
		var d := Vector2(points[i].x - p.x, points[i].z - p.z).length_squared()
		if d < dmin:
			dmin = d
			meilleur = i
	return meilleur
