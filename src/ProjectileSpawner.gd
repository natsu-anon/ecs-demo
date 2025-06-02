class_name ProjectileSpawner
extends Node3D

@export var projectile_speed: float = 10
@export var projectile_lifetime: float = 10.0
@export var num_total: int = 100
var projectile = preload("res://basic_projectile.tscn")
var sum: float = 0
var freq: float


func _ready() -> void:
	freq = projectile_lifetime / num_total
	print("Spawn frequency: ", freq)

func _process(delta: float) -> void:
	sum += delta
	while sum > freq:
		spawn_projectile()
		sum -= freq

func spawn_projectile() -> void:
	var p: Projectile = projectile.instantiate()
	add_child(p)
	p.global_position = position
	p.velocity = projectile_speed * Vector3.FORWARD
	p.lifetime = projectile_lifetime
