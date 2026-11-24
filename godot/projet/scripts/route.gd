# WaForSpeed — prototype Godot (WFS-610) : maillage de la route de la Ville à partir du tracé : chaussée (enrobé usé, CC0),
# marquages (axe discontinu, rives continues), bordures de trottoir chanfreinées, trottoirs (béton, CC0) et accotements.
# Découpée en tronçons de 200 m : Godot n'envoie au GPU que ce qui est dans le champ (et à moins de 900 m), avec fondu.
class_name Route
extends RefCounted

const TRONCON := 100          # points par tronçon (100 × 2 m = 200 m)
const TROTTOIR := 4.0         # largeur du trottoir (m)
const BORDURE_H := 0.14       # hauteur de la bordure (m)
const ACCOTEMENT := 40.0      # sol au-delà du trottoir (m)

static func materiau(nom: String, uv_echelle: float) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_texture = load("res://textures/%s-couleur.png" % nom)
	m.normal_enabled = true
	m.normal_texture = load("res://textures/%s-normale.png" % nom)
	m.roughness_texture = load("res://textures/%s-orm.png" % nom)
	m.roughness_texture_channel = BaseMaterial3D.TEXTURE_CHANNEL_GREEN
	m.ao_enabled = true
	m.ao_texture = load("res://textures/%s-orm.png" % nom)
	m.ao_texture_channel = BaseMaterial3D.TEXTURE_CHANNEL_RED
	m.uv1_scale = Vector3(uv_echelle, uv_echelle, 1.0)
	m.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC
	return m

static func peinture_route() -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = Color(0.82, 0.82, 0.78)   # peinture routière blanche, un peu salie
	m.roughness = 0.6
	return m

# bande entre deux décalages latéraux (g < d) à la hauteur h, de l'indice a à b ; v en mètres le long du tracé
static func bande(st: SurfaceTool, tr: Trace, a: int, b: int, g: float, d: float, hg: float, hd: float, uv_large: float) -> void:
	for i in range(a, b):
		var p0 := tr.points[i]
		var p1 := tr.points[i + 1]
		var r0 := Trace.droite_de(tr.caps[i])
		var r1 := Trace.droite_de(tr.caps[i + 1])
		var v0 := i * Trace.PAS / uv_large
		var v1 := (i + 1) * Trace.PAS / uv_large
		var q := [p0 + r0 * g + Vector3(0, hg, 0), p0 + r0 * d + Vector3(0, hd, 0), p1 + r1 * g + Vector3(0, hg, 0), p1 + r1 * d + Vector3(0, hd, 0)]
		var u := [Vector2(g / uv_large, v0), Vector2(d / uv_large, v0), Vector2(g / uv_large, v1), Vector2(d / uv_large, v1)]
		for k in [0, 2, 1, 1, 2, 3]:
			st.set_uv(u[k])
			st.add_vertex(q[k])

static func construire(parent: Node3D, tr: Trace) -> void:
	var enrobe := materiau("enrobe-use", 1.0)
	var beton := materiau("beton-trottoir", 1.0)
	var sol := materiau("beton-trottoir", 1.0)
	sol.albedo_color = Color(0.55, 0.53, 0.5)
	var blanc := peinture_route()
	var w := tr.demi_largeur
	var n := tr.points.size()
	var a := 0
	while a < n - 1:
		var b := mini(a + TRONCON, n - 1)
		var troncon := Node3D.new()
		troncon.name = "troncon_%d" % (a / TRONCON)
		parent.add_child(troncon)
		_ajouter(troncon, enrobe, func(st): bande(st, tr, a, b, -w, w, 0.0, 0.0, 4.0))
		# bordures : face verticale chanfreinée (2 cm) puis dessus, des deux côtés
		_ajouter(troncon, beton, func(st):
			for cote: float in [-1.0, 1.0]:
				var x: float = w * cote
				if cote > 0:
					bande(st, tr, a, b, x, x + 0.02, 0.0, BORDURE_H - 0.02, 1.0)
					bande(st, tr, a, b, x + 0.02, x + TROTTOIR, BORDURE_H, BORDURE_H, 3.0)
				else:
					bande(st, tr, a, b, x - 0.02, x, BORDURE_H - 0.02, 0.0, 1.0)
					bande(st, tr, a, b, x - TROTTOIR, x - 0.02, BORDURE_H, BORDURE_H, 3.0))
		_ajouter(troncon, sol, func(st):
			bande(st, tr, a, b, w + TROTTOIR, w + TROTTOIR + ACCOTEMENT, BORDURE_H, -0.3, 8.0)
			bande(st, tr, a, b, -w - TROTTOIR - ACCOTEMENT, -w - TROTTOIR, -0.3, BORDURE_H, 8.0))
		# marquages : rives continues (15 cm), axe discontinu 3 m / 10 m (ville), 1 cm au-dessus de l'enrobé
		_ajouter(troncon, blanc, func(st):
			bande(st, tr, a, b, -w + 0.25, -w + 0.40, 0.01, 0.01, 1.0)
			bande(st, tr, a, b, w - 0.40, w - 0.25, 0.01, 0.01, 1.0)
			var i := a
			while i < b:
				if i % 7 < 2:
					bande(st, tr, i, mini(i + 1, b), -0.075, 0.075, 0.01, 0.01, 1.0)
				i += 1)
		a = b

static func _ajouter(parent: Node3D, mat: Material, remplir: Callable) -> void:
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	remplir.call(st)
	st.generate_normals()
	st.generate_tangents()
	var mi := MeshInstance3D.new()
	mi.mesh = st.commit()
	mi.material_override = mat
	mi.visibility_range_end = 900.0
	mi.visibility_range_end_margin = 100.0
	mi.visibility_range_fade_mode = GeometryInstance3D.VISIBILITY_RANGE_FADE_SELF
	parent.add_child(mi)
