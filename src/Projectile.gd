class_name Projectile
extends Node3D

var lifetime: float
var velocity: Vector3

func _init() -> void:
	set_physics_process(false)

func _process(delta: float) -> void:
	translate(delta * velocity)
	lifetime -= delta
	if lifetime < 0.0:
		timeout()

func timeout() -> void:
	queue_free()
