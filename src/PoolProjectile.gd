class_name PoolProjectile
extends Projectile

@onready var pool: ProjectilePool = get_parent()

func _init() -> void:
	hide()
	set_process(false)
	set_physics_process(false)
	set_process_input(false)
	lifetime = 100.0
	velocity = Vector3.ZERO

func activate(position0: Vector3, velocity0: Vector3, lifetime0: float) -> void:
	position = position0
	velocity = velocity0
	lifetime = lifetime0
	set_process(true)
	show()

func timeout() -> void:
	hide()
	set_process(false)
	pool.move_child(self, pool.num_active)
	pool.num_active -= 1
