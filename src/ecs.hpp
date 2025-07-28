#ifndef ENTITY_COMPONENT_SYSTEM_HPP
#define ENTITY_COMPONENT_SYSTEM_HPP

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include "components.hpp"

// #define ENTITY_CAP 16384
#define ENTITY_CAP 65536

namespace godot {
class Entity3D;

enum Component {
#define X(A, _, __, ___) A,
	COMPONENTS
#undef X
	NUM_COMPONENTS
};

}
VARIANT_ENUM_CAST(Component);


struct {
	godot::Entity3D** entities; // lol lmao even
	void** components;
	// uint64_t* bitmasks;
	int8_t* bitmasks;
	int32_t size;
} ecs_table = {0};

// struct Slice;

namespace godot {

// NOTE: VARIANT_ENUM_CAST has to be outside of godot namespace

class ECS: public Node {
	GDCLASS(ECS, Node)


public:
	ECS();
	~ECS();

	// godot::Entity3D** entity;
	// uint64_t* component;
	// uint16_t size;

private:
	int max_threads;

public:
	static int32_t activate_entity(Entity3D* entity);
	static void add_component(const int32_t id, const Component component);

#define X(_, __, NAME, GODOT_TYPE) static void set_##NAME(const int32_t id, const GODOT_TYPE value);
	COMPONENTS
#undef X

	void _process(const double delta) override;


protected:
	static void _bind_methods();

// private:
// 	void set_slices(Slice* slices, const uint16_t num) const;

};

}

#endif // ENTITY_COMPONENT_SYSTEM_HPP
