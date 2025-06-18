#pragma once
#include <Jolt/Jolt.h>

constexpr float PHYSICS_MAX_DT = 200.0f;
constexpr float PHYSICS_GRAVITATIONAL_CONSTANT = 9.81f;

// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
constexpr JPH::uint PHYSICS_MAX_BODIES = 65536; // Old: 16384

// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
constexpr JPH::uint PHYSICS_MUTEX_COUNT = 0;

// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
constexpr JPH::uint PHYSICS_MAX_BODY_PAIRS = 65536; // old: 16384

// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
constexpr JPH::uint PHYSICS_MAX_CONTACT_CONSTRAINTS = 10240; // Old: 8192

// Pre-allocating 10 MB to avoid having to do allocations during the physics update.
// Memory pool used for Physics Update
constexpr JPH::uint PHYSICS_TEMP_ALLOCATOR_SIZE = 10 * 1024 * 1024;

// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable.
// Do 1 collision step per 1 / 60th of a second (round up).
constexpr float PHYSICS_STEPS_PER_SECOND = 1.0f / 60.0f;

// If the game runs at 1 FPS then physics is the least of our concerns
constexpr int PHYSICS_MAX_STEPS_PER_FRAME = 10;
