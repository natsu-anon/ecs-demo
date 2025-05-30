#include "entity_projectile_pool.hpp"
#include <cstdint>
#include <godot_cpp/core/class_db.hpp>
#include "entity3d.hpp"

using namespace godot;

EntityProjectilePool::EntityProjectilePool()
{
	/* TODO */
}

EntityProjectilePool::~EntityProjectilePool()
{
	/* TODO */
}

void EntityProjectilePool::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("spawn_projectile"), &EntityProjectilePool::spawn_projectile);
#define X(A) \
	ClassDB::bind_method(D_METHOD("get_"#A), &EntityProjectilePool::get_##A); \
	ClassDB::bind_method(D_METHOD("set_"#A, #A), &EntityProjectilePool::set_##A); \
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "projectile_"#A), "set_"#A, "get_"#A);
	PROPS
#undef X
}

void EntityProjectilePool::spawn_projectile()
{
	// Entity* entity = (Entity*)get_child(num_active++);
	// const uint16_t id = ECS::cache_entity(entity);
	// entity->add_component(id, POSITION);
	// entity->add_component(id, VELOCITY);
	// // entity->add_component(id, LIFETIME);
	// entity->setPosition(get_global_position());
	// entity->setVelocity(Vector3(0.0, 0.0, -speed));
	// // entity->setLifetime(lifetime);
	// entity->show();
}

#define X(A) \
	void EntityProjectilePool::set_##A(double value) { A = value; } \
	double EntityProjectilePool::get_##A() const { return A; }
PROPS
#undef X
