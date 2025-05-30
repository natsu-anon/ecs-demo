#include "components.hpp"
#include "allocators/pool.h"

#define X(_, __, A, ___) pool_t A##_pool = {0};
COMPONENTS
#undef X
