class_name EntityProjectilePool
extends EntityPool

@export var projectile_speed: float = 10
@export var projectile_lifetime: float = 10.0
@export var num_total: int = 100
@export var multi_mesh_instance: MultiMeshInstance3D
var projectile = preload("res://entity_projectile.tscn")
var sum: float = 0
var freq: float
@onready var mmesh = multi_mesh_instance.multimesh

func _ready() -> void:
	freq = projectile_lifetime / num_total
	freq += 0.001
	num_active =  0
	print("Spawn frequency: ", freq)
	mmesh.instance_count = num_total
	mmesh.visible_instance_count = -1

func _enter_tree() -> void:
	for i in range(num_total):
		var entity: Entity3D = projectile.instantiate()
		add_child(entity)

func _exit_tree() -> void:
	for i in range(num_total):
		get_child(i).queue_free()

func _process(delta: float) -> void:
	sum += delta
	while sum > freq && num_active < num_total:
		spawn_projectile()
		sum -= freq
	mmesh.visible_instance_count = num_active
	for i in range(num_active):
		mmesh.set_instance_transform(i, get_child(i).global_transform)

func spawn_projectile() -> void:
	var entity: Entity3D = get_child(num_active)
	num_active += 1
	var id: int = ECS.activate_entity(entity)
	ECS.add_component(id, ECS.POSITION)
	ECS.set_position(id, position)
	ECS.add_component(id, ECS.VELOCITY)
	ECS.set_velocity(id, projectile_speed * Vector3.FORWARD)
	ECS.add_component(id, ECS.LIFETIME)
	ECS.set_lifetime(id, projectile_lifetime)
