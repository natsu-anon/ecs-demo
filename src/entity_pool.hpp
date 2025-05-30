#ifndef ENTITY_POOL_HPP
#define ENTITY_POOL_HPP

#include <godot_cpp/classes/node3d.hpp>

namespace godot
{

class EntityPool: public Node3D
{
	GDCLASS(EntityPool, Node3D)

public:
	int num_active;

public:
	EntityPool();
	~EntityPool();

protected:
	static void _bind_methods();

// JAVA WAS A MISTAKE
public:
	int get_num_active() const;
	void set_num_active(int value);
};

}

#endif
