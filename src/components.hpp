#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <godot_cpp/classes/ref.hpp>
#include <stdint.h>

#define COMPONENTS \
	X(POSITION, Position, position, Vector3)									\
	X(VELOCITY, Velocity, velocity, Vector3)							\
	X(LIFETIME, Lifetime, lifetime, float)

struct Position {
	float x;
	float y;
	float z;
};

struct Velocity {
	float x;
	float y;
	float z;
};

// struct Lifetime {
// 	float value;
// };


union Lifetime {
	float value;
	uint32_t bits;
};

#endif /* End COMPONENTS_H */
