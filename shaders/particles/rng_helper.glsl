// https://www.shadertoy.com/view/Msf3WH
vec2 hash_simplex_2D(vec2 p) // replace this by something better
{
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}