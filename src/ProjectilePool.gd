class_name ProjectilePool
extends ProjectileSpawner

var pool_projectile = preload("res://pool_projectile.tscn")
@export var multi_mesh_instance: MultiMeshInstance3D
@onready var num_active: int = 0
var mmesh

func _ready() -> void:
	super()
	#multimesh.instance_count = num_total
	mmesh = multi_mesh_instance.multimesh
	mmesh.instance_count = num_total
	mmesh.visible_instance_count = 0

func _enter_tree() -> void:
	for i in range(num_total):
		var p: PoolProjectile = pool_projectile.instantiate()
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
	var p: PoolProjectile = get_child(num_active)
	p.activate(position, projectile_speed * Vector3.FORWARD, projectile_lifetime)
	num_active += 1
