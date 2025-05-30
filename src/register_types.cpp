#include "register_types.hpp"

#include "ecs.hpp"
#include "entity3d.hpp"
// #include "entity_pool.hpp"
// #include "entity_projectile_pool.hpp"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_ecsdemo_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_RUNTIME_CLASS(ECS);
	GDREGISTER_RUNTIME_CLASS(Entity3D);
	// GDREGISTER_RUNTIME_CLASS(EntityPool);
	// GDREGISTER_RUNTIME_CLASS(EntityProjectilePool);
}

void uninitialize_ecsdemo_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT ecsdemo_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_ecsdemo_module);
	init_obj.register_terminator(uninitialize_ecsdemo_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
