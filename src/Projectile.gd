class_name Projectile
extends Node3D

var lifetime: float
var velocity: Vector3
var flag: bool = false

func _process(delta: float) -> void:
	translate(delta * velocity)
	lifetime -= delta
	if lifetime < 0.0 && flag:
		var pool: ProjectilePool = get_parent()
		# print("recycle: ", get_index())
		hide()
		set_process(false)
		pool.move_child(self, -1)
		pool.num_active -= 1
		flag = false
