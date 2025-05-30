extends Node3D

@export var projectile_speed: float = 10
@export var projectile_lifetime: float = 10.0
@export var num_total: int = 100
var projectile = preload("res://entity_projectile.tscn")
var num_active: int = 0
var sum: float = 0
var freq: float

func _ready() -> void:
	freq = projectile_lifetime / num_total
	num_active =  0
	print("Spawn frequency: ", freq)

func _enter_tree() -> void:
	for i in range(num_total):
		pass
		var entity: Entity3D = projectile.instantiate()
		#e.hide()
		add_child(entity)
	num_active = 0

func _exit_tree() -> void:
	for i in range(num_total):
		get_child(i).queue_free()
	num_active = 0

func _process(delta: float) -> void:
	sum += delta
	while sum > freq && num_active < num_total:
		spawn_projectile()
		sum -= freq

func spawn_projectile() -> void:
	var entity: Entity3D = get_child(num_active)
	var id: int = ECS.activate_entity(entity)
	ECS.add_component(id, ECS.VELOCITY)
	ECS.set_velocity(id, projectile_speed * Vector3.FORWARD)
	num_active += 1
