mat4 GetSkinMatrix(mat2x4 bone)
{
    vec4 r = bone[0];
    vec4 t = bone[1];

    return mat4(
    1.0 - (2.0 * r.y * r.y) - (2.0 * r.z * r.z),
    (2.0 * r.x * r.y) + (2.0 * r.w * r.z),
    (2.0 * r.x * r.z) - (2.0 * r.w * r.y),
    0.0,

    (2.0 * r.x * r.y) - (2.0 * r.w * r.z),
    1.0 - (2.0 * r.x * r.x) - (2.0 * r.z * r.z),
    (2.0 * r.y * r.z) + (2.0 * r.w * r.x),
    0.0,

    (2.0 * r.x * r.z) + (2.0 * r.w * r.y),
    (2.0 * r.y * r.z) - (2.0 * r.w * r.x),
    1.0 - (2.0 * r.x * r.x) - (2.0 * r.y * r.y),
    0.0,

    2.0 * (-t.w * r.x + t.x * r.w - t.y * r.z + t.z * r.y),
    2.0 * (-t.w * r.y + t.x * r.z + t.y * r.w - t.z * r.x),
    2.0 * (-t.w * r.z - t.x * r.y + t.y * r.x + t.z * r.w),
    1);
}
