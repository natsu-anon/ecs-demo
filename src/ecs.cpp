#include "ecs.hpp"
#include "entity3d.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#ifdef _WIN32 // wtf microsoft
#include <string.h>
#else
#include <strings.h>
#endif
#include <threads.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/os.hpp>
#include "components.hpp"
#include "entity_pool.hpp"
#include "allocators/arena.h"
#include "allocators/pool.h"


static struct {
	int32_t indices[ENTITY_CAP];
	int32_t size;
} update_list = {0};

struct Span
{
	int32_t i;
	int32_t n;
};

struct DeltaSpan
{
	double delta;
	int32_t i;
	int32_t n;
};


static arena_t res_arena = {0};
static arena_t arg_arena = {0};


using namespace godot;

#define X(_, __, A,  ___) static pool_t A##_pool = {0};
COMPONENTS
#undef X
pool_t* component_pool[NUM_COMPONENTS];

ECS::ECS() {
	if (Engine::get_singleton()->is_editor_hint()) return;
	max_threads = OS::get_singleton()->get_processor_count();
	// initialize the entity table
	// const size_t size = sizeof *ecs_table.entities + sizeof *ecs_table.bitmasks;
	const size_t size = sizeof *ecs_table.entities + NUM_COMPONENTS * sizeof *ecs_table.components + sizeof *ecs_table.bitmasks;
	ecs_table.entities = (Entity3D**)malloc(ENTITY_CAP * size);
	assert(entity_table.entity && "Failed to allocate memory for the ecs_table");
	ecs_table.components = (void**)(ecs_table.entities + ENTITY_CAP);
	ecs_table.bitmasks = (int8_t*)(ecs_table.components + NUM_COMPONENTS * ENTITY_CAP);
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
#define X(_, __, NAME, ___) std::free(NAME##_pool.allocation);
	COMPONENTS
#undef X
}

int32_t ECS::activate_entity(Entity3D* entity) {
	// if (entity == NULL) { return ENTITY_CAP; }
	if (ecs_table.size < ENTITY_CAP) {
		const int32_t i = ecs_table.size++;
		ecs_table.entities[i] = entity;
		ecs_table.bitmasks[i] = 0;
		// I could memset the actual components, but I don't have to(see add_component)
		entity->ecs_id = i;
		entity->show();
		return i;
	}
	else {
		fprintf(stderr, "ENTITY OVERFLOW!");
		assert(0);
		return -1;
	}
}

void ECS::add_component(const int32_t id, const Component component)
{
	ecs_table.components[NUM_COMPONENTS * id + component] = pool_calloc(component_pool[component]);
	ecs_table.bitmasks[id] |= 1 << component;
}

void ECS::set_position(const int32_t id, const Vector3 &value) {
	Position temp = { .x = value.x, .y = value.y, .z = value.z };
	memcpy(ecs_table.components[NUM_COMPONENTS * id + POSITION], &temp, sizeof(Velocity));
}

void ECS::set_velocity(const int32_t id, const Vector3 &value) {
	Velocity temp = { .x = value.x, .y = value.y, .z = value.z };
	memcpy(ecs_table.components[NUM_COMPONENTS * id + VELOCITY], &temp, sizeof(Velocity));
}

void ECS::set_lifetime(const int32_t id, const float &value) {
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

inline static void mark_update(int32_t index) {
	update_list.indices[update_list.size++] = index;
}

static void set_spans(Span* spans, const int32_t num_threads, const int32_t n) {
	const int32_t div = n / num_threads;
	const int32_t mod = n % num_threads;
	spans->i = 0;
	spans->n = div + (mod > 0);
	for (uint8_t i = 1; i < num_threads - 1; ++i) {
		const int32_t k = spans[i - 1].n;
		spans[i].i = k;
		spans[i].n = k + div + (mod > i);
	}
	if (num_threads > 1)
	{
		spans[num_threads - 1].i = spans[num_threads - 2].n;
		spans[num_threads - 1].n = n;
	}
}

static void set_delta_spans(DeltaSpan* spans, const int32_t num_threads, const int32_t n, const double delta) {
	const int32_t div = n / num_threads;
	const int32_t mod = n % num_threads;
	spans->delta = delta;
	spans->i = 0;
	spans->n = div + (mod > 0);
	for (uint8_t i = 1; i < num_threads - 1; ++i) {
		const int32_t k = spans[i - 1].n;
		spans[i].i = k;
		spans[i].n = k + div + (mod > i);
		spans[i].delta = delta;
	}
	if (num_threads > 1)
	{
		spans[num_threads - 1].i = spans[num_threads - 2].n;
		spans[num_threads - 1].n = n;
		spans[num_threads - 1].delta = delta;
	}
}

static int free_components(void* args)
{
	const Component c = *(Component*)args;
	// const int8_t pool_index = *(int8_t*)args;
	void** components = ecs_table.components;
	for (int32_t j = 0; j < update_list.size; ++j)
	{
		const int32_t k = update_list.indices[j];
		pool_free(component_pool[c], components[NUM_COMPONENTS * k + c]);
	}
	return 0;
}

static int hide_nodes(void* args)
{
	const Span* span = (Span*)args;
	const int32_t n = span->n;
	Entity3D** entities = ecs_table.entities;
	for (int32_t i = span->i; i < n; ++i)
	{
		entities[update_list.indices[i]]->hide();
	}
	return 0;
}

struct PositionVelocity {
	Position position;
	Velocity velocity;
};

static int populate_position_update_buffers(void* args) {
	const DeltaSpan* span = (DeltaSpan*)args;
	const int32_t n = span->n;
	PositionVelocity* dest = (PositionVelocity*)arg_arena.allocation;
	void** components = ecs_table.components;
	for (int32_t i = span->i; i < n; ++i) {
		const int32_t j = update_list.indices[i];
		const uint32_t k = NUM_COMPONENTS * j;
		memcpy(&dest[i].position, components[k + POSITION], sizeof(Position));
		memcpy(&dest[i].velocity, components[k + VELOCITY], sizeof(Velocity));
	}
	return 0;
}

static int update_positions(void* args) {
	const DeltaSpan* span = (DeltaSpan*)args;
	const double delta = span->delta;
	const int32_t n = span->n;
	PositionVelocity* src = (PositionVelocity*)arg_arena.allocation;
	Position* dest = (Position*)res_arena.allocation;
	for (int32_t i = span->i; i < n; ++i) {
		const Position p0 = src[i].position;
		const Velocity v0 = src[i].velocity;
		Position* p1 = dest + i;
		p1->x = p0.x + delta * v0.x;
		p1->y = p0.y + delta * v0.y;
		p1->z = p0.z + delta * v0.z;
	}
	return 0;
}

static int sync_positions(void* args) {
	const DeltaSpan* span = (DeltaSpan*)args;
	const int32_t n = span->n;
	Position* res = (Position*)res_arena.allocation;
	void** components = ecs_table.components;
	Entity3D** entities = ecs_table.entities;
	for (int32_t i = span->i; i < n; ++i) {
		const int32_t j = update_list.indices[i];
		const uint32_t k = NUM_COMPONENTS * j;
		memcpy(components[k + POSITION], res + i, sizeof(Position));
	}
	return 0;
}

static int populate_lifetime_update_buffer(void* args) {
	const DeltaSpan* span = (DeltaSpan*)args;
	const int32_t n = span->n;
	Lifetime* buf = (Lifetime*)arg_arena.allocation;
	void** components = ecs_table.components;
	for (int32_t i = span->i; i < n; ++i) {
		const int32_t j = update_list.indices[i];
		const int32_t k = NUM_COMPONENTS * j;
		memcpy(buf + i, components[k + LIFETIME], sizeof(Lifetime));
	}
	return 0;
}

#define FREE_ENTITY NUM_COMPONENTS
static int update_lifetimes(void* args) {
	const DeltaSpan* span = (DeltaSpan*)args;
	const int32_t n = span->n;
	const double delta = span->delta;
	Lifetime* buf = (Lifetime*)arg_arena.allocation;
	for (int32_t i = span->i; i < n; ++i) {
		buf[update_list.indices[i]].value -= delta;
	}
	return 0;
}

static int sync_lifetimes(void* args) {
	const DeltaSpan* span = (DeltaSpan*)args;
	const int32_t n = span->n;
	Lifetime* buf = (Lifetime*)arg_arena.allocation;
	void** components = ecs_table.components;
	int8_t* bitmasks = ecs_table.bitmasks;
	for (int32_t i = span->i; i < n; ++i) {
		const int32_t j = update_list.indices[i];
		const int32_t k = NUM_COMPONENTS * j;
		memcpy(components[k + LIFETIME], buf + i, sizeof(Lifetime));
		bitmasks[j] |= (buf[i].bits >> 31) << FREE_ENTITY;
	}
	return 0;
}

void ECS::_process(const double delta) {
	// these aliases are actually free safe
	int8_t* bitmasks = ecs_table.bitmasks;
	Entity3D** entities = ecs_table.entities;
	thrd_t* threads = (thrd_t*)alloca(max_threads * sizeof *threads);
	int t_res;
	// FIRST: free before anything else
	uint8_t mask = 1 << FREE_ENTITY;
	update_list.size = 0;
	for (int32_t i = 0; i < ecs_table.size; ++i) {
		if (bitmasks[i] & mask) {
			mark_update(i);
		}
	}
	if (update_list.size > 0) {
		void** components = ecs_table.components;
		const int32_t n = update_list.size;
		if (max_threads >= NUM_COMPONENTS) {
			Component* c = (Component*)alloca(NUM_COMPONENTS * sizeof *c);
			for (int8_t i = 0; i < NUM_COMPONENTS; ++i) {
				c[i] = (Component)i;
				thrd_create(threads + i, free_components, c + i);
			}
			for (int8_t i = 0; i < NUM_COMPONENTS; ++i) {
				thrd_join(threads[i], &t_res);
			}
		}
		else {
			// HOMEWORK ASSIGNMENT!
		}
		// NOTE: skip zeroing the to-be-freed bitmasks since ecs_table.size will be shrunk
		// hide the nodes
		// finally, move the entities to end of the entity pool
		// AND, shrink the entity pool
		// AND, shrink the entity table by repeatedly copying the last element to the closest freed spot.
		// So can't brainlessly multithread :(
		// EntityPool* entity_pool = (EntityPool*)entities[0]->get_parent();
		// for (int32_t i = n - 1; i >= 0; --i) {
		// 	Entity3D* e = entities[update_list.indices[i]];
		// 	e->hide();
		// 	entity_pool->move_child(e, entity_pool->num_active--);
		// }
		// const size_t sizeof_components = NUM_COMPONENTS * sizeof *components; // hoist
		EntityPool* entity_pool = (EntityPool*)entities[0]->get_parent();
		const size_t sizeof_components = NUM_COMPONENTS * sizeof *components; // hoist
		for (int32_t i = n - 1; i >= 0; --i) {
			const int32_t j = update_list.indices[i];
			entities[j]->hide();
			entity_pool->move_child(entities[j], entity_pool->num_active--);
			const int32_t m = --ecs_table.size;
			if (j < m) {
				entities[j] = entities[m];
				bitmasks[j] = bitmasks[m];
				// copy over the component addresses
				memcpy(components + j * NUM_COMPONENTS, components + m * NUM_COMPONENTS, sizeof_components);
				entities[j]->ecs_id = j;
			}
		}
	}
	mask = (1 << POSITION) | (1 << VELOCITY);
	update_list.size = 0;
	for (int32_t i = 0; i < ecs_table.size; ++i) {
		if ((bitmasks[i] & mask) == mask) {
			mark_update(i);
		}
	}
	if (update_list.size > 0) {
		const int32_t n = update_list.size;
		DeltaSpan* spans = (DeltaSpan*)alloca(max_threads * sizeof *spans);
		set_delta_spans(spans, max_threads, n, delta);
		PositionVelocity* arg = (PositionVelocity*)arena_scratch(&arg_arena, n * sizeof(PositionVelocity));
		Position* res = (Position*)arena_scratch(&res_arena, n * sizeof *res);
		for (int8_t t = 0; t < max_threads; ++t) {
			thrd_create(threads + t, populate_position_update_buffers, spans + t);
		}
		for (int8_t t = 0; t < max_threads; ++t) {
			thrd_join(threads[t], &t_res);
		}
		for (int8_t t = 0; t < max_threads; ++t) {
			thrd_create(threads + t, update_positions, spans + t);
		}
		for (int8_t t = 0; t < max_threads; ++t) {
			thrd_join(threads[t], &t_res);
		}
		for (int8_t t = 0; t < max_threads; ++t) {
			thrd_create(threads + t, sync_positions, spans + t);
		}
		for (int8_t t = 0; t < max_threads; ++t) {
			thrd_join(threads[t], &t_res);
		}
		// NOTE: godot doesnt want to brain-off multithread this :(
		for (int32_t i = 0; i < n; ++i) {
			const Vector3 vec3 = Vector3(res[i].x, res[i].y, res[i].z);
			entities[update_list.indices[i]]->set_global_position(vec3);
		}
	}
	// update lifetimes
	mask = 1 << LIFETIME;
	update_list.size = 0;
	for (int32_t i = 0; i < ecs_table.size; ++i) {
		if (ecs_table.bitmasks[i] & mask) {
			mark_update(i);
		}
	}
	if (update_list.size > 0) {
		const int32_t n = update_list.size;
		arena_scratch(&arg_arena, n * sizeof(Lifetime));
		DeltaSpan* spans = (DeltaSpan*)alloca(max_threads * sizeof *spans);
		set_delta_spans(spans, max_threads, n, delta);
		// for (int8_t t = 0; t < max_threads; ++t) {
		// 	printf("spans%d: [%d, %d)\n", t, spans[t].i, spans[t].n);
		// }
		for (int8_t t = 0; t < max_threads; ++t) {
			thrd_create(threads + t, populate_lifetime_update_buffer, spans + t);
		}
		for (int8_t t = 0; t < max_threads; ++t) {
			thrd_join(threads[t], &t_res);
		}
		for (int8_t t = 0; t < max_threads; ++t) {
			thrd_create(threads + t, update_lifetimes, spans + t);
		}
		for (int8_t t = 0; t < max_threads; ++t) {
			thrd_join(threads[t], &t_res);
		}
		for (int8_t t = 0; t < max_threads; ++t) {
			thrd_create(threads + t, sync_lifetimes, spans + t);
		}
		for (int8_t t = 0; t < max_threads; ++t) {
			thrd_join(threads[t], &t_res);
		}
	}
}
