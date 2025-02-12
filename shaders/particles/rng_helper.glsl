// https://www.shadertoy.com/view/Msf3WH
vec2 hash_simplex_2D(vec2 p) // replace this by something better
{
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float HashFloat(uint seed) {
    seed ^= 2747636419u;
    seed *= 2654435769u;
    seed ^= seed >> 16;
    seed *= 2654435769u;
    seed ^= seed >> 16;
    return (float(seed & 0x007FFFFFu) / 0x007FFFFF); // Normalize to [0,1)
}

vec3 HashVec3(uint seed) {
    return vec3(
    HashFloat(seed),
    HashFloat(seed * 31u + 1u), // Perturb the seed with a prime number
    HashFloat(seed * 127u + 19u) // Perturb with a different prime
    );
}