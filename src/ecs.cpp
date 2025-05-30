#include "ecs.hpp"
#include "entity3d.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>
#include <threads.h>
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

#define X(_, __, A,  ___) static pool_t A##_pool;
COMPONENTS
#undef X
pool_t* component_pool[Component::NUM_COMPONENTS];
// static pool_t entity_component_pool;

ECS::ECS() {
	if (Engine::get_singleton()->is_editor_hint()) return;
	max_threads = OS::get_singleton()->get_processor_count();
	// initialize the entity table
	const size_t size = sizeof *ecs_table.entities + NUM_COMPONENTS * sizeof *ecs_table.components + sizeof *ecs_table.bitmasks;
	ecs_table.entities = malloc(ENTITY_CAP * size);
	assert(entity_table.entity && "Failed to allocate memory for entity_table.");
	ecs_table.components = (void**)(ecs_table.entities + NUM_COMPONENTS * ENTITY_CAP);
	ecs_table.bitmasks = (uint64_t*)(ecs_table.components + ENTITY_CAP);
	// initialize all the pools
#define X(ENUM, TYPE, NAME, __)							\
	pool_init(&NAME##_pool, sizeof(TYPE), ENTITY_CAP);	\
	component_pool[ENUM] = &NAME##_pool;
	COMPONENTS
#undef X
	// // see Entity.hpp
	// pool_init(&entity_component_pool, sizeof(void*), ENTITY_CAP);
	// initialize the update list
	// update_list.indices = malloc(ENTITY_CAP * sizeof update_list.indices);
	update_list.size = 0;
}

ECS::~ECS() {
	/* WHY FREE? */
	std::free(res_arena.allocation);
	std::free(arg_arena.allocation);
	// std::free(update_list.indices);
}


uint16_t ECS::activate_entity(Entity3D* entity) {
	// if (entity == NULL) { return ENTITY_CAP; }
	const uint16_t i = ecs_table.size++;
	ecs_table.entities[i] = entity;
	ecs_table.bitmasks[i] = 0; // zero before distributing
	memset(ecs_table.components + i, 0x00, NUM_COMPONENTS * sizeof *ecs_table.components);
	entity->ecs_id = i;
	entity->show();
	// entity->component = pool_calloc(&entity_component_pool);
	return i;
}

void ECS::add_component(const uint16_t ecs_id, const Component component)
{
	// size_t* components = (size_t*)ecs_table.components + ecs_id;
	// components[component] = pool_calloc(component_pool[component]);
	ecs_table.bitmasks[ecs_id] |= 1 << component;
}

void ECS::set_position(const uint16_t id, const Vector3 &value) {
	Position temp = { .x = value.x, .y = value.y, .z = value.z };
	size_t* components = (size_t*)ecs_table.components[id];
	memcpy(components + POSITION, &temp, sizeof(Position));
}

void ECS::set_velocity(const uint16_t id, const Vector3 &value) {
	Velocity temp = { .x = value.x, .y = value.y, .z = value.z };
	size_t* components = (size_t*)ecs_table.components[id];
	memcpy(components + VELOCITY, &temp, sizeof(Velocity));
	// Velocity* v = (Velocity*)(components + VELOCITY);
	// print_line("v", id, " = (", v->x , ", ", v->y, ", ", v->z, ")");
}

void ECS::set_lifetime(const uint16_t id, const float &value) {
	size_t* components = (size_t*)ecs_table.components[id];
	memcpy(components + LIFETIME, &value, sizeof(Lifetime));
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
	print_line("> processing ", ecs_table.size, " entities");
	// these aliases are actually safe
	// print_line("ECS._process ", delta);
	Entity3D** entities = ecs_table.entities;
	void* components[NUM_COMPONENTS] = ecs_table.components;
	uint64_t* bitmasks = ecs_table.bitmasks;
	// FIRST: free before anything else
	uint64_t bitmask = 1 << FREE_ENTITY;
	update_list.size = 0;
	// print_line("> FREE");
	for (uint16_t i = 0; i < ecs_table.size; ++i)
	{
		if (bitmasks[i] & bitmask)
		{
			mark_update(i);
		}
	}
	// print_line(" - marked");
	if (update_list.size > 0)
	{
		const uint16_t n = update_list.size;
		print_line(" (1) freeing ", n, " entities");
		// free the components for reuse
		for (uint16_t i = 0; i < NUM_COMPONENTS; ++i)
		{
			for (uint16_t j = 0; j < n; ++j)
			{
				const uint16_t k = update_list.indices[j];
				pool_free(component_pool[i], (size_t*)components[k] + i);
			}
		}
		// hide the node & move it to end of the entity pool
		EntityPool* entity_pool = (EntityPool*)entities[0]->get_parent();
		for (uint16_t i = 0; i < n; ++i)
		{
			Entity3D* e = entities[update_list.indices[i]];
			e->hide();
			// pool_free(&entity_component_pool, e->component);
			// e->component = NULL;
			entity_pool->move_child(e, -1);
		}
		entity_pool->num_active -= n;
		// finally, shrink the entity table--repeatedly copy the last element to the freed spot.
		// can't brainlessly multithread :(
		// so also free the entity's component address array as well since that can't be multithreaded without a mutex either.
		for (uint16_t i = n - 1; i >= 0; --i)
		{
			const uint16_t j = update_list.indices[i];
			const uint16_t m = --ecs_table.size;
			if (j < m)
			{
				entities[j] = entities[m];
				entities[j]->ecs_id = j;
				bitmasks[j] = bitmasks[m];
				memcpy(components + j, components + m, NUM_COMPONENTS * sizeof *components);
			}
		}
	}
	// update positions
	// print_line("> updating positions");
	// bitmask = (1 << POSITION) | (1 << VELOCITY);
	// NOTE: just use translate, smoothbrain.  You're working with Nodes so why not?
	bitmask = 1 << VELOCITY;
	update_list.size = 0;
	for (uint16_t i = 0; i < ecs_table.size; ++i)
	{
		if (bitmasks[i] & bitmask)
		{
			mark_update(i);
		}
	}
	// print_line(" - marked");
	if (update_list.size > 0)
	{
		// struct Argument {
		// 	Position position;
		// 	Velocity velocity;
		// };
		const uint16_t n = update_list.size;
		print_line(" (2) translating ", n, " entities");
		Velocity* args = arena_scratch(&arg_arena, n * sizeof *args);
		Velocity* res = arena_scratch(&res_arena, n * sizeof *res);
		// print_line(" - scratch allocated");
		for (uint16_t i = 0; i < n; ++i)
		{
			const uint16_t j = update_list.indices[i];
			// const size_t* comps = (size_t*)components + update_list.indices[i];
			const size_t* comps = (size_t*)components + j;
			memcpy(args + i, comps + VELOCITY, sizeof(Velocity));
			print_line("v", j, " = (", args[i].x , ", ", args[i].y, ", ", args[i].z, ")");
			// memcpy(&args[i].position, entities[index]->component[POSITION], sizeof(Position));
			// memcpy(&args[i].velocity, entities[index]->component[VELOCITY], sizeof(Velocity));
		}
		// print_line(" - arg buffer copied");
		for (uint16_t i = 0; i < n; ++i)
		{
			// const Position arg_pos = args[i].position;
			// const Velocity arg_vel = args[i].velocity;
			const Velocity v0 = args[i];
			Velocity* v1 = res + i;
			v1->x = delta * v0.x;
			v1->y = delta * v0.y;
			v1->z = delta * v0.z;
		}
		// print_line(" - results");
		// for (uint16_t i = 0; i < n; ++i)
		// {
		// 	const uint16_t index = update_list.indices[i];
		// 	memcpy(entities[index]->component[POSITION], res + i, sizeof(Position));
		// }
		for (uint16_t i = 0; i < n; ++i)
		{
			// const uint16_t j = update_list.indices[i];
			const Velocity v = res[i];
			const Vector3 vec3 = Vector3(v.x, v.y, v.z);
			entities[update_list.indices[i]]->global_translate(vec3);
		}
		// print_line(" - applied results -- NO CACHE");
		// print_line(" - update game positions");
	}
	// update lifetimes
	// print_line("> updating lifetimes");
	bitmask = 1 << LIFETIME;
	update_list.size = 0;
	for (uint16_t i = 0; i < ecs_table.size; ++i)
	{
		if (ecs_table.bitmasks[i] & bitmask)
		{
			mark_update(i);
		}
	}
	// print_line(" - marked");
	if (update_list.size > 0)
	{
		const uint16_t n = update_list.size;
		print_line(" (3) ticking ", n, "lifetimes");
		Lifetime* args = arena_scratch(&arg_arena, n * sizeof *args);
		Lifetime* res = arena_scratch(&res_arena, n * sizeof *res);
		// print_line(" - scratch allocated");
		for (uint16_t i = 0; i < n; ++i)
		{
			const size_t* comps = (size_t*)components + update_list.indices[i];
			memcpy(args + i, comps + LIFETIME, sizeof(Lifetime));
		}
		// print_line(" - arg buffer copied");
		for (uint16_t i = 0; i < n; ++i)
		{
			res[i].value = args[i].value - delta;
		}
		// print_line(" - results");
		for (uint16_t i = 0; i < n; ++i)
		{
			const uint16_t j = update_list.indices[i];
			size_t* comps = (size_t*)components + j;
			memcpy(comps + LIFETIME, args + i, sizeof(LIFETIME));
			// D E V I L I S H T A C T I C
			bitmasks[j] |= ~(res[i].bits >> 31) << FREE_ENTITY;
		}
		// print_line(" - results copied back to entities");
	}
	// print_line("> end");
}
