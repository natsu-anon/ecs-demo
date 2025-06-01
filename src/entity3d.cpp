#include "entity3d.hpp"
#include <string.h>
// #include <godot_cpp/classes/engine.hpp>
// #include <godot_cpp/core/class_db.hpp>
// #include "allocators/pool.h"
// #include "components.hpp"
// #include "ecs.hpp"

using namespace godot;

Entity3D::Entity3D() {
	hide();
	set_process(false);
	set_physics_process(false);
	set_process_input(false);
	ecs_id = ~0;
	// memset(&components, 0x00, NUM_COMPONENTS * sizeof(void*)); // it be that way
	// component = NULL;
}

// void Entity3D::add_component(const uint16_t entity_id, const Component c_id) {
// 	component[c_id] = pool_calloc(component_pool[c_id]);
// 	entity_table.bitmask[entity_id] |= 1 << c_id;
// }


// #define X(ENUM, _, NAME, GODOT_TYPE)							  \
// 	void Entity::set_##NAME(const GODOT_TYPE& value) const {  \
// 		memcpy(component[ENUM], &value, sizeof(NAME)); \
// 	}
// #undef X

// void Entity3D ::set_position(const Vector3 &value) const {
// 	Position temp  = { .x = value.x, .y = value.y, .z = value.z };
// 	memcpy(component[POSITION], &temp, sizeof(Position));
// }
// void Entity3D ::set_velocity(const Vector3 &value) const {
// 	Velocity temp  = { .x = value.x, .y = value.y, .z = value.z };
// 	memcpy(component[VELOCITY], &temp, sizeof(Velocity));
// }
// void Entity3D ::set_lifetime(const float &value) const {
// 	memcpy(component[LIFETIME], &value, sizeof(Lifetime));
// }

void Entity3D::set_ecs_id(const int value) { ecs_id = value; }
int Entity3D::get_ecs_id() const { return ecs_id; }

void Entity3D::_bind_methods() {
	// ClassDB::bind_method(D_METHOD("add_component", "entity_id", "component_id"), &Entity::add_component);
	// #define X(ENUM, _, NAME, GODOT_TYPE)											\
	// 	ClassDB::bind_method(D_METHOD("set_"#NAME, "new_value"), &Entity::set_##NAME);
	// COMPONENTS
	// #undef X
	ClassDB::bind_method(D_METHOD("set_ecs_id", "ecs_id"), &Entity3D::set_ecs_id);
	ClassDB::bind_method(D_METHOD("get_ecs_id"), &Entity3D::get_ecs_id);
	// ADD_PROPERTY(PropertyInfo(Variant::INT, "ecs_id"), "set_ecs_id", "get_ecs_id");
}
