class_name Projectile
extends Node3D

var lifetime: float
var velocity: Vector3
var flag: bool = false
#@export var mmesh: MultiMeshInstance3D

func _init():
	set_physics_process(false)

func _process(delta: float) -> void:
	translate(delta * velocity)
	#mmesh.transform = transform
	lifetime -= delta
	if lifetime < 0.0 && flag:
		var pool: ProjectilePool = get_parent()
		# print("recycle: ", get_index())
		hide()
		set_process(false)
		pool.move_child(self, pool.num_active)
		pool.num_active -= 1
		flag = false
