#include "ecs.hpp"
#include "entity3d.hpp"
#include <cstdint>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#ifdef _WIN32 // wtf microsoft
#include <string.h>
#else
#include <strings.h>
#endif
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/os.hpp>
#include "components.hpp"
#include "entity_pool.hpp"
#include "allocators/pool.h"


static struct {
	uint16_t indices[ENTITY_CAP];
	uint16_t size;
} update_list = {0};

static arena_t res_arena = {0};
static arena_t arg_arena = {0};


using namespace godot;

#define X(_, __, A,  ___) static pool_t A##_pool = {0};
COMPONENTS
#undef X
pool_t* component_pool[Component::NUM_COMPONENTS];

ECS::ECS() {
	if (Engine::get_singleton()->is_editor_hint()) return;
	// max_threads = OS::get_singleton()->get_processor_count();
	// initialize the entity table
	// const size_t size = sizeof *ecs_table.entities + sizeof *ecs_table.bitmasks;
	const size_t size = sizeof *ecs_table.entities + NUM_COMPONENTS * sizeof *ecs_table.components + sizeof *ecs_table.bitmasks;
	ecs_table.entities = (Entity3D**)malloc(ENTITY_CAP * size);
	assert(entity_table.entity && "Failed to allocate memory for the ecs_table");
	ecs_table.components = (void**)(ecs_table.entities + ENTITY_CAP);
	ecs_table.bitmasks = (uint8_t*)(ecs_table.components + NUM_COMPONENTS * ENTITY_CAP);
	ecs_table.size = 0;
	// initialize all the pools
#define X(ENUM, TYPE, NAME, __)							\
	pool_init(&NAME##_pool, sizeof(TYPE), ENTITY_CAP);	\
	component_pool[ENUM] = &NAME##_pool;
	COMPONENTS
#undef X
}

ECS::~ECS() {
	std::free(res_arena.allocation);
	std::free(arg_arena.allocation);
	std::free(ecs_table.entities);
}


uint16_t ECS::activate_entity(Entity3D* entity) {
	// if (entity == NULL) { return ENTITY_CAP; }
	if (ecs_table.size == ENTITY_CAP - 1)
	{
		fprintf(stderr, "ENTITY OVERFLOW!");
		assert(0);
	}
	const uint16_t i = ecs_table.size++;
	ecs_table.entities[i] = entity;
	ecs_table.bitmasks[i] = 0;
	// I could memset the actual components, but I don't have to(see add_component)
	entity->ecs_id = i;
	entity->show();
	return i;
}

void ECS::add_component(const uint16_t id, const Component component)
{
	ecs_table.components[NUM_COMPONENTS * id + component] = pool_calloc(component_pool[component]);
	ecs_table.bitmasks[id] |= 1 << component;
}

void ECS::set_position(const uint16_t id, const Vector3 &value) {
	Position temp = { .x = value.x, .y = value.y, .z = value.z };
	memcpy(ecs_table.components[NUM_COMPONENTS * id + POSITION], &temp, sizeof(Velocity));
}

void ECS::set_velocity(const uint16_t id, const Vector3 &value) {
	Velocity temp = { .x = value.x, .y = value.y, .z = value.z };
	memcpy(ecs_table.components[NUM_COMPONENTS * id + VELOCITY], &temp, sizeof(Velocity));
}

void ECS::set_lifetime(const uint16_t id, const float &value) {
	memcpy(ecs_table.components[NUM_COMPONENTS * id + LIFETIME], &value, sizeof(Lifetime));
}


void ECS::_bind_methods() {
	ClassDB::bind_static_method("ECS", D_METHOD("activate_entity", "entity"), &ECS::activate_entity);
	ClassDB::bind_static_method("ECS", D_METHOD("add_component", "ecs_id", "component"), &ECS::add_component);
#define X(ENUM, _, NAME, __)											\
	BIND_ENUM_CONSTANT(ENUM);											\
	ClassDB::bind_static_method("ECS", D_METHOD("set_"#NAME, "ecs_id", "value"), &ECS::set_##NAME);
	COMPONENTS
#undef X
}

inline static void mark_update(uint16_t index) {
	update_list.indices[update_list.size++] = index;
}

#define FREE_ENTITY NUM_COMPONENTS
void ECS::_process(const double delta) {
	// these aliases are actually free-safe
	Entity3D** entities = ecs_table.entities;
	void** components = ecs_table.components;
	uint8_t* bitmasks = ecs_table.bitmasks;
	// FIRST: free before anything else
	uint8_t mask = 1 << FREE_ENTITY;
	update_list.size = 0;
	for (uint16_t i = 0; i < ecs_table.size; ++i) {
		if (bitmasks[i] & mask) {
			mark_update(i);
		}
	}
	if (update_list.size > 0) {
		const uint16_t n = update_list.size;
		// printf("\n> freeing %d out of %d entities\n", n, ecs_table.size);
		// recycle the components
		for (uint8_t i = 0; i < NUM_COMPONENTS; ++i) {
			for (uint16_t j = 0; j < n; ++j) {
				const uint16_t k = update_list.indices[j];
				pool_free(component_pool[i], components[NUM_COMPONENTS * k + i]);
			}
		}
		// NOTE: skip zeroing the to-be-freed bitmasks since ecs_table.size will be shrunk
		// hide the node & move it to end of the entity pool
		EntityPool* entity_pool = (EntityPool*)entities[0]->get_parent();
		for (int32_t i = n - 1; i >= 0; --i) {
			Entity3D* e = entities[update_list.indices[i]];
			e->hide();
			entity_pool->move_child(e, --entity_pool->num_active);
		}
		// finally, shrink the entity table by repeatedly copying the last element to the closest freed spot.
		// So can't brainlessly multithread :(
		// So also free the entity's component address array as well since that can't be multithreaded without a mutex either.
		// for (uint16_t i = n - 1; i >= 0; --i) // LOL LMAO
		const size_t sizeof_components = NUM_COMPONENTS * sizeof *components; // hoist
		for (int32_t i = n - 1; i >= 0; --i) {
			const uint16_t j = update_list.indices[i];
			const uint16_t m = --ecs_table.size;
			if (j < m) {
				entities[j] = entities[m];
				bitmasks[j] = bitmasks[m];
				// copy over the component addresses
				memcpy(components + j * NUM_COMPONENTS, components + m * NUM_COMPONENTS, sizeof_components);
				entities[j]->ecs_id = j;
			}
		}
	}
	// update positions
	mask = (1 << POSITION) | (1 << VELOCITY);
	update_list.size = 0;
	for (uint16_t i = 0; i < ecs_table.size; ++i) {
		if ((bitmasks[i] & mask) == mask) {
			mark_update(i);
		}
	}
	if (update_list.size > 0) {
		const uint16_t n = update_list.size;
		struct Argument {
			Position position;
			Velocity velocity;
		};
		// fuck you too, c++
		Argument* args = (Argument*)arena_scratch(&arg_arena, n * sizeof *args);
		Position* res = (Position*)arena_scratch(&res_arena, n * sizeof *res);
		for (uint16_t i = 0; i < n; ++i) {
			const uint16_t j = update_list.indices[i];
			const uint32_t k = NUM_COMPONENTS * j;
			memcpy(&args[i].position, components[k + POSITION], sizeof(Position));
			memcpy(&args[i].velocity, components[k + VELOCITY], sizeof(Velocity));
		}
		for (uint16_t i = 0; i < n; ++i) {
			const Position p0 = args[i].position;
			const Velocity v0 = args[i].velocity;
			Position* p1 = res + i;
			p1->x = p0.x + delta * v0.x;
			p1->y = p0.y + delta * v0.y;
			p1->z = p0.z + delta * v0.z;
		}
		for (uint16_t i = 0; i < n; ++i) {
			const uint16_t j = update_list.indices[i];
			const uint32_t k = NUM_COMPONENTS * j;
			memcpy(components[k + POSITION], res + i, sizeof(Position));
			const Vector3 vec3 = Vector3(res[i].x, res[i].y, res[i].z);
			entities[j]->set_global_position(vec3);
		}
	}
	// update lifetimes
	mask = 1 << LIFETIME;
	update_list.size = 0;
	for (uint16_t i = 0; i < ecs_table.size; ++i)
	{
		if (bitmasks[i] & mask) {
			mark_update(i);
		}
	}
	if (update_list.size > 0)
	{
		const uint16_t n = update_list.size;
		// "just use generic."  A phrase used by small-souled bugmen.
		Lifetime* args = (Lifetime*)arena_scratch(&arg_arena, n * sizeof *args);
		for (uint16_t i = 0; i < n; ++i)
		{
			const uint16_t j = update_list.indices[i];
			memcpy(args + i, components[NUM_COMPONENTS * j + LIFETIME], sizeof(Lifetime));
		}
		for (uint16_t i = 0; i < n; ++i)
		{
			args[i].value -= delta;
		}
		for (uint16_t i = 0; i < n; ++i)
		{
			// D E V I L I S H T A C T I C
			const uint16_t j = update_list.indices[i];
			memcpy(components[NUM_COMPONENTS * j + LIFETIME], args + i, sizeof(Lifetime));
			bitmasks[j] |= (args[i].bits >> 31) << FREE_ENTITY;
		}
	}
}
