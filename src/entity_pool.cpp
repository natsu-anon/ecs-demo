#include "entity_pool.hpp"
#include <godot_cpp/core/class_db.hpp>


using namespace godot;

EntityPool::EntityPool()
{
	num_active = 0;
}

EntityPool::~EntityPool()
{
	/* TODO */
}
int EntityPool::get_num_active() const
{
	return num_active;
}

void EntityPool::set_num_active(int value)
{
	num_active = value;
}

void EntityPool::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_num_active"), &EntityPool::get_num_active);
	ClassDB::bind_method(D_METHOD("set_num_active", "num_active"), &EntityPool::set_num_active);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "num_active"), "set_num_active", "get_num_active");
}
