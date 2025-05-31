#include "ecs.hpp"
#include "entity3d.hpp"
#include <stdio.h>
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
#include "godot_cpp/variant/transform3d.hpp"


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
pool_t address_pool = {0};
// static pool_t entity_component_pool;

ECS::ECS() {
	if (Engine::get_singleton()->is_editor_hint()) return;
	max_threads = OS::get_singleton()->get_processor_count();
	// initialize the entity table
	const size_t size = sizeof *ecs_table.entities + sizeof *ecs_table.bitmasks;
	ecs_table.entities = malloc(ENTITY_CAP * size);
	assert(entity_table.entity && "Failed to allocate memory for entity_table.");
	// ecs_table.components = (void**)(ecs_table.entities + NUM_COMPONENTS * ENTITY_CAP);
	// ecs_table.bitmasks = (uint64_t*)(ecs_table.components + ENTITY_CAP);
	ecs_table.bitmasks = (uint64_t*)(ecs_table.entities + ENTITY_CAP);
	// initialize all the pools
#define X(ENUM, TYPE, NAME, __)							\
	pool_init(&NAME##_pool, sizeof(TYPE), ENTITY_CAP);	\
	component_pool[ENUM] = &NAME##_pool;
	COMPONENTS
#undef X
	// see Entity.hpp
	pool_init(&address_pool, NUM_COMPONENTS * sizeof(void*), ENTITY_CAP);
	// initialize the update list
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
	// memset(ecs_table.components + i, 0x00, NUM_COMPONENTS * sizeof *ecs_table.components);
	// entity->ecs_id = i;
	// entity->components = pool_calloc(&address_pool);
	entity->show();
	return i;
}

void ECS::add_component(const uint16_t ecs_id, const Component component)
{
	// size_t* components = (size_t*)ecs_table.components + ecs_id;
	// components[component] = pool_calloc(component_pool[component]);
	ecs_table.entities[ecs_id]->components[component] = pool_calloc(component_pool[component]);
	ecs_table.bitmasks[ecs_id] |= 1 << component;
}

void ECS::set_position(const uint16_t id, const Vector3 &value) {
	Position temp = { .x = value.x, .y = value.y, .z = value.z };
	Position* dest =  ecs_table.entities[id]->components[POSITION];
	memcpy(dest, &temp, sizeof(Position));
}

void ECS::set_velocity(const uint16_t id, const Vector3 &value) {
	Velocity temp = { .x = value.x, .y = value.y, .z = value.z };
	Velocity* dest =  ecs_table.entities[id]->components[VELOCITY];
	memcpy(dest, &temp, sizeof(Velocity));
	// Velocity* v = (Velocity*)(components + VELOCITY);
	// print_line("v", id, " = (", v->x , ", ", v->y, ", ", v->z, ")");
}

void ECS::set_lifetime(const uint16_t id, const float &value) {
	Lifetime* dest =  ecs_table.entities[id]->components[LIFETIME];
	dest->value = value;
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
	// print_line("> processing ", ecs_table.size, " entities");
	// these aliases are actually safe
	// print_line("ECS._process ", delta);
	Entity3D** entities = ecs_table.entities;
	// void* components[NUM_COMPONENTS] = ecs_table.components;
	uint64_t* bitmasks = ecs_table.bitmasks;
	// FIRST: free before anything else
	// print_line("Free entity: ", FREE_ENTITY);
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
	// update_list.size = 0;
	// print_line(" - marked");
	if (update_list.size > 0)
	{
		const uint16_t n = update_list.size;
		// print_line(">freeing ", n, " entities");
		// free the components for reuse
		for (uint16_t i = 0; i < NUM_COMPONENTS; ++i)
		{
			for (uint16_t j = 0; j < n; ++j)
			{
				const uint16_t k = update_list.indices[j];
				pool_free(component_pool[i], entities[k]->components[i]);
			}
		}
		// print_line(" - returned components to pool");
		// hide the node & move it to end of the entity pool
		EntityPool* entity_pool = (EntityPool*)entities[0]->get_parent();
		for (uint16_t i = 0; i < n; ++i)
		{
			Entity3D* e = entities[update_list.indices[i]];
			e->hide();
			pool_free(&address_pool, e->components);
			memset(e->components, 0x00, NUM_COMPONENTS * sizeof(void*)); // it be that way
			entity_pool->move_child(e, -1);
		}
		// print_line(" - num_active0: ", entity_pool->num_active);
		entity_pool->num_active -= n;
		// print_line(" - num_active1: ", entity_pool->num_active);
		// print_line(" - returned Entity3D nodes to pool");
		// finally, shrink the entity table--repeatedly copy the last element to the freed spot.
		// can't brainlessly multithread :(
		// so also free the entity's component address array as well since that can't be multithreaded without a mutex either.
		// for (uint16_t i = n - 1; i >= 0; --i) // LOL LMAO
		for (int32_t i = n - 1; i >= 0; --i)
		{
			const uint16_t j = update_list.indices[i];
			const uint16_t m = --ecs_table.size;
			if (j < m)
			{
				entities[j] = entities[m];
				// entities[j]->ecs_id = j;
				bitmasks[j] = bitmasks[m];
				// memcpy(components + j, components + m, NUM_COMPONENTS * sizeof *components);
			}
		}
		// print_line(" - swap & shrunk");
	}
	// update positions
	// print_line("> updating positions");
	// bitmask = (1 << POSITION) | (1 << VELOCITY);
	// NOTE: just use translate, smoothbrain.  You're working with Nodes so why not?
	bitmask = (1 << POSITION) | (1 << VELOCITY);
	update_list.size = 0;
	for (uint16_t i = 0; i < ecs_table.size; ++i)
	{
		if ((bitmasks[i] & bitmask) == bitmask)
		{
			mark_update(i);
		}
	}
	// print_line(" - marked");
	if (update_list.size > 0)
	{
		struct Argument {
			Position position;
			Velocity velocity;
		};
		const uint16_t n = update_list.size;
		Argument* args = arena_scratch(&arg_arena, n * sizeof *args);
		Position* res = arena_scratch(&res_arena, n * sizeof *res);
		// print_line(" - scratch allocated");
		for (uint16_t i = 0; i < n; ++i)
		{
			const uint16_t j = update_list.indices[i];
			memcpy(&args[i].position, entities[j]->components[POSITION], sizeof(Position));
			memcpy(&args[i].velocity, entities[j]->components[VELOCITY], sizeof(Velocity));
		}
		// print_line(" - arg buffer copied");
		for (uint16_t i = 0; i < n; ++i)
		{
			const Position p0 = args[i].position;
			const Velocity v0 = args[i].velocity;
			Position* p1 = res + i;
			p1->x = p0.x + delta * v0.x;
			p1->y = p0.y + delta * v0.y;
			p1->z = p0.z + delta * v0.z;
		}
		// print_line(" - results");
		for (uint16_t i = 0; i < n; ++i)
		{
			const uint16_t j = update_list.indices[i];
			memcpy(entities[j]->components[POSITION], res + i, sizeof(Position));
			const Vector3 vec3 = Vector3(res[i].x, res[i].y, res[i].z);
			entities[j]->set_position(vec3);
		}
		// for (uint16_t i = 0; i < n; ++i)
		// {
		// 	// const uint16_t j = update_list.indices[i];
		// 	// const Position pos = res[i];
		// 	const Vector3 vec3 = Vector3(res[i].x, res[i].y, res[i].z);
		// 	entities[update_list.indices[i]]->set_position(vec3);
		// }
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
		// print_line(" (3) ticking ", n, "lifetimes");
		Lifetime* args = arena_scratch(&arg_arena, n * sizeof *args);
		Lifetime* res = args;
		// Lifetime* res = arena_scratch(&res_arena, n * sizeof *res);
		// print_line(" - scratch allocated");
		for (uint16_t i = 0; i < n; ++i)
		{
			const uint16_t j = update_list.indices[i];
			memcpy(args + i, entities[j]->components[LIFETIME], sizeof(Lifetime));
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
			memcpy(entities[j]->components[LIFETIME], res + i, sizeof(Lifetime));
			// D E V I L I S H T A C T I C
			bitmasks[j] |= (res[i].bits >> 31) << FREE_ENTITY;
		}
		// print_line(" - results copied back to entities");
	}
	// print_line("> end");
}
