extends MultiMeshInstance3D

func _ready() -> void:
	var mmesh: MultiMesh = multimesh
	mmesh.instance_count = 20
	for i in range(20):
		mmesh.set_instance_transform(i, Transform3D.IDENTITY.translated(i * Vector3.FORWARD))
	
