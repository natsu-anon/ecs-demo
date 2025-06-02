class_name ProjectilePool
extends Node3D

@export var projectile_speed: float = 10
@export var projectile_lifetime: float = 10.0
@export var num_total: int = 100
var projectile = preload("res://classic_projectile.tscn")
@onready var num_active: int = 0
var sum: float = 0
var freq: float
@export var multi_mesh_instance: MultiMeshInstance3D
var mmesh

func _ready() -> void:
	freq = projectile_lifetime / num_total
	freq += 0.001
	print("Spawn frequency: ", freq)
	#multimesh.instance_count = num_total
	mmesh = multi_mesh_instance.multimesh
	mmesh.instance_count = num_total
	mmesh.visible_instance_count = 0

func _enter_tree() -> void:
	for i in range(num_total):
		var p: Projectile = projectile.instantiate()
		p.hide()
		p.set_process(false)
		p.set_physics_process(false)
		p.set_process_input(false)
		add_child(p)
	num_active = 0

func _exit_tree() -> void:
	for i in range(num_total):
		get_child(i + 1).queue_free()
	num_active = 0

func _process(delta: float) -> void:
	sum += delta
	while sum > freq && num_active < num_total:
		spawn_projectile()
		sum -= freq
	mmesh.visible_instance_count = num_active
	for i in range(num_active):
		mmesh.set_instance_transform(i, get_child(i).global_transform)

func spawn_projectile() -> void:
	var p: Projectile = get_child(num_active)
	p.position = position
	p.velocity = projectile_speed * Vector3.FORWARD
	p.lifetime = projectile_lifetime
	p.flag = true
	p.show()
	p.set_process(true)
	num_active += 1
	# total += 1
	# print("active: ", num_active, ", total: ", total)
