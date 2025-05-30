class_name TestProjectile
extends Entity3D

func _ready():
	show()
	var id: int = ECS.activate_entity(self)
	ECS.add_component(id, ECS.VELOCITY)
	ECS.set_velocity(id, 0.1 * Vector3.FORWARD)
