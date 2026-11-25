# WaForSpeed — prototype Godot (WFS-610) : scène de comparaison. Route de la Ville (tracé du jeu actuel), M4 du joueur,
# trafic, caméra de course, ciel et soleil, commandes tactiles et clavier, compteur d'images par seconde.
# Commandes tactiles : moitié gauche de l'écran = volant (glisser le doigt à gauche / à droite) ; en bas à droite : GAZ
# (grand bouton) et FREIN (à sa gauche). Clavier (PC) : flèches ou ZQSD.
extends Node3D

var tr := Trace.new()
var joueur: VoitureJoueur
var camera: Camera3D
var hud: Label
var mesure: Label
var cam_pos := Vector3.ZERO
var doigt_volant := -1
var x_volant := 0.0
var doigts_pedales := {}   # index → "gaz" | "frein"
var t_mesure := 0.0
var images := 0
var pire := 0.0

func _ready() -> void:
	tr.charger("res://donnees/ville.json")
	_environnement()
	var route := Node3D.new()
	route.name = "Route"
	add_child(route)
	Route.construire(route, tr)
	joueur = VoitureJoueur.new()
	joueur.name = "Joueur"
	add_child(joueur)
	joueur.preparer(tr, load("res://voitures/m4.glb"), Color(0.05, 0.22, 0.62))
	var trafic := Trafic.new()
	trafic.name = "Trafic"
	add_child(trafic)
	trafic.preparer(tr, [load("res://voitures/corsa.glb"), load("res://voitures/tcross.glb"), load("res://voitures/model3.glb"),
						 load("res://voitures/golf.glb"), load("res://voitures/p208.glb")])
	camera = Camera3D.new()
	camera.fov = 62.0
	camera.near = 0.1
	camera.far = 1500.0
	add_child(camera)
	camera.current = true
	cam_pos = joueur.position + Trace.avant_de(joueur.cap) * -6.5 + Vector3(0, 2.3, 0)
	_hud()

func _environnement() -> void:
	var env := Environment.new()
	var ciel := Sky.new()
	var mat := ProceduralSkyMaterial.new()
	mat.sky_top_color = Color(0.25, 0.45, 0.78)
	mat.sky_horizon_color = Color(0.68, 0.76, 0.86)
	mat.ground_horizon_color = Color(0.6, 0.62, 0.64)
	mat.ground_bottom_color = Color(0.3, 0.3, 0.3)
	mat.sun_angle_max = 20.0
	ciel.sky_material = mat
	env.background_mode = Environment.BG_SKY
	env.sky = ciel
	env.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
	env.reflected_light_source = Environment.REFLECTION_SOURCE_SKY
	env.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	env.tonemap_exposure = 1.0
	env.glow_enabled = true
	env.glow_intensity = 0.4
	env.fog_enabled = true
	env.fog_light_color = Color(0.7, 0.76, 0.84)
	env.fog_density = 0.0012
	env.fog_aerial_perspective = 0.6
	var we := WorldEnvironment.new()
	we.environment = env
	add_child(we)
	var soleil := DirectionalLight3D.new()
	soleil.rotation_degrees = Vector3(-38.0, 35.0, 0.0)
	soleil.light_energy = 1.15
	soleil.light_color = Color(1.0, 0.96, 0.9)
	soleil.shadow_enabled = true
	soleil.directional_shadow_max_distance = 90.0
	soleil.directional_shadow_mode = DirectionalLight3D.SHADOW_PARALLEL_2_SPLITS
	add_child(soleil)

func _hud() -> void:
	var couche := CanvasLayer.new()
	add_child(couche)
	hud = Label.new()
	hud.position = Vector2(24, 16)
	hud.add_theme_font_size_override("font_size", 40)
	hud.add_theme_color_override("font_outline_color", Color.BLACK)
	hud.add_theme_constant_override("outline_size", 8)
	couche.add_child(hud)
	mesure = Label.new()
	mesure.position = Vector2(24, 70)
	mesure.add_theme_font_size_override("font_size", 22)
	mesure.add_theme_color_override("font_outline_color", Color.BLACK)
	mesure.add_theme_constant_override("outline_size", 6)
	couche.add_child(mesure)
	var aide := Label.new()
	aide.text = "Prototype Godot 4.3 — gauche : volant (glisser)   droite : FREIN | GAZ"
	aide.anchor_top = 1.0
	aide.anchor_bottom = 1.0
	aide.position = Vector2(24, -40)
	aide.add_theme_font_size_override("font_size", 18)
	aide.add_theme_color_override("font_outline_color", Color.BLACK)
	aide.add_theme_constant_override("outline_size", 5)
	couche.add_child(aide)
	for p in [["FREIN", 0.62, Color(0.8, 0.15, 0.1, 0.35)], ["GAZ", 0.81, Color(0.1, 0.7, 0.2, 0.35)]]:
		var b := Label.new()
		b.text = p[0]
		b.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		b.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
		b.anchor_left = p[1]
		b.anchor_right = p[1] + 0.17
		b.anchor_top = 0.55
		b.anchor_bottom = 0.95
		b.add_theme_font_size_override("font_size", 30)
		var fond := StyleBoxFlat.new()
		fond.bg_color = p[2]
		fond.set_corner_radius_all(24)
		b.add_theme_stylebox_override("normal", fond)
		couche.add_child(b)

func _pedale(pos: Vector2) -> String:
	var t := get_viewport().get_visible_rect().size
	if pos.y < t.y * 0.5 or pos.x < t.x * 0.6:
		return ""
	return "gaz" if pos.x >= t.x * 0.8 else "frein"

func _input(ev: InputEvent) -> void:
	var t := get_viewport().get_visible_rect().size
	if ev is InputEventScreenTouch:
		var e := ev as InputEventScreenTouch
		if e.pressed:
			if e.position.x < t.x * 0.5 and doigt_volant < 0:
				doigt_volant = e.index
				x_volant = e.position.x
			else:
				var p := _pedale(e.position)
				if p != "":
					doigts_pedales[e.index] = p
		else:
			if e.index == doigt_volant:
				doigt_volant = -1
				joueur.volant = 0.0
			doigts_pedales.erase(e.index)
	elif ev is InputEventScreenDrag:
		var d := ev as InputEventScreenDrag
		if d.index == doigt_volant:
			joueur.volant = clampf((d.position.x - x_volant) / (t.x * 0.12), -1.0, 1.0)

func _process(dt: float) -> void:
	# commandes : tactile, sinon clavier
	var gaz := 1.0 if doigts_pedales.values().has("gaz") else 0.0
	var frein := 1.0 if doigts_pedales.values().has("frein") else 0.0
	if Input.is_key_pressed(KEY_UP) or Input.is_key_pressed(KEY_Z) or Input.is_key_pressed(KEY_W):
		gaz = 1.0
	if Input.is_key_pressed(KEY_DOWN) or Input.is_key_pressed(KEY_S):
		frein = 1.0
	if OS.get_environment("WFS_CAPTURE") != "":
		gaz = 1.0   # captures de relecture : gaz tenu
	joueur.gaz = gaz
	joueur.frein = frein
	if doigt_volant < 0:
		var k := 0.0
		if Input.is_key_pressed(KEY_LEFT) or Input.is_key_pressed(KEY_Q) or Input.is_key_pressed(KEY_A):
			k -= 1.0
		if Input.is_key_pressed(KEY_RIGHT) or Input.is_key_pressed(KEY_D):
			k += 1.0
		joueur.volant = move_toward(joueur.volant, k, dt * 4.0)
	# caméra de course : derrière et au-dessus, suit en douceur (ressort exponentiel), champ qui s'ouvre avec la vitesse
	var arriere := Trace.avant_de(joueur.cap) * -(6.2 + absf(joueur.vitesse) * 0.03)
	var cible := joueur.position + arriere + Vector3(0, 2.1, 0)
	cam_pos = cam_pos.lerp(cible, 1.0 - exp(-dt * 16.0))   # retard ≈ v / 16 : la voiture reste grande à haute vitesse
	camera.position = cam_pos
	camera.look_at(joueur.position + Trace.avant_de(joueur.cap) * 4.0 + Vector3(0, 1.0, 0), Vector3.UP)
	camera.fov = lerpf(62.0, 74.0, clampf(absf(joueur.vitesse) / 70.0, 0.0, 1.0))
	# affichage : vitesse ; mesure : images par seconde et pire image sur 2 s
	hud.text = "%d km/h" % int(round(absf(joueur.vitesse) * 3.6))
	images += 1
	t_mesure += dt
	pire = maxf(pire, dt)
	if t_mesure >= 2.0:
		mesure.text = "%.0f img/s   pire image %.0f ms   %s" % [images / t_mesure, pire * 1000.0, RenderingServer.get_video_adapter_name()]
		images = 0
		t_mesure = 0.0
		pire = 0.0

# ---- captures pour la relecture (banc GitHub) : WFS_CAPTURE=<dossier> : gaz tenu, images à 3 s, 8 s et 14 s, puis fin
var t_capture := 0.0
var captures_faites := 0
func _physics_process(dt: float) -> void:
	var dossier := OS.get_environment("WFS_CAPTURE")
	if dossier == "":
		return
	t_capture += dt
	var instants := [3.0, 8.0, 14.0]
	if captures_faites < instants.size() and t_capture >= instants[captures_faites]:
		captures_faites += 1
		var n := captures_faites
		await RenderingServer.frame_post_draw
		get_viewport().get_texture().get_image().save_png("%s/godot-%d.png" % [dossier, n])
		print("capture %d : %d km/h, %s" % [n, int(joueur.vitesse * 3.6), mesure.text])
		if n == instants.size():
			get_tree().quit()
