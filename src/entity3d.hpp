#ifndef ENTITY3D_HPP
#define ENTITY3D_HPP

#include <godot_cpp/classes/node3d.hpp>
#include "ecs.hpp"

namespace godot {

class Entity3D: public Node3D {
	GDCLASS(Entity3D, Node3D)


public:
	Entity3D();
	// ~Entity();

public:
	int ecs_id;
	// void* components[NUM_COMPONENTS];
	// void add_component(const uint16_t entity_id, const Component c_id);

	// :^)
	// #define X(_, __, NAME, GODOT_TYPE) void set_##NAME(const GODOT_TYPE& value) const;
	// COMPONENTS
	// #undef X

public:
	void set_ecs_id(const int value);
	int get_ecs_id() const;


protected:
	static void _bind_methods();
};

}

#endif
