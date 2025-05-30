#ifndef ENTITY_PROJECTILE_POOL_HPP
#define ENTITY_PROJECTILE_POOL_HPP

#include "entity_pool.hpp"

namespace godot
{

class EntityProjectilePool: public EntityPool
{
	GDCLASS(EntityProjectilePool, EntityPool)

public:
	EntityProjectilePool();
	~EntityProjectilePool();

public:
	#define PROPS \
		X(speed) \
		X(lifetime)
	#define X(A) double A;
	PROPS
	#undef X

protected:
	static void _bind_methods();

public:
	void spawn_projectile();

	#define X(A) \
		void set_##A(double value); \
		double get_##A() const;
	PROPS
	#undef X


};

}

#endif /* ENTITY_PROJECTILE_POOL_HPP */
