extends Node3D

var projectile = preload("res://entity_projectile.tscn")

func _ready() -> void:
	for i in range(4000):
		var entity: Entity3D = projectile.instantiate()
		add_child(entity)
		entity.global_translate(i * Vector3.LEFT)
		var id = ECS.activate_entity(entity)
		ECS.add_component(id, ECS.POSITION)
		ECS.set_position(id, position)
		ECS.add_component(id, ECS.VELOCITY)
		ECS.set_velocity(id, 10.0 * Vector3.FORWARD)
	return
