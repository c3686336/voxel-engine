#version 450 core
#define INF 1.0/0.0
#define PI 3.1415926538
#define SUN_DIR normalize(vec3(0.3, 0.7, 0.5))
#define SUN_COLOR vec3(1.0, 1.0, 1.0)

layout(location = 0) in vec2 frag_pos;

layout(location = 1) uniform vec3 camera_pos;
layout(location = 2) uniform vec3 camera_dir;
layout(location = 5) uniform vec3 camera_right;
layout(location = 4) uniform vec3 camera_up;
layout(location = 6) uniform float bias_amt;

struct Node {
    uint mat_id;
    uint addr[8];
};

struct SvodagMetaData {
    mat4 model_inv;
    uint max_level;
    uint at_index;
};

struct QueryResult {
    uint at_level; // 0: At deepest level, MLEVEL: at root, because the level is in the unit of branches
    Node node;
};

struct SimpleMaterial {
    vec4 albedo;
};

layout(std430, binding = 3) buffer one {
    Node nodes[];
};

layout(std430, binding = 2) buffer two {
    SvodagMetaData metadata[];
};

layout(std430, binding = 6) buffer matids {
    SimpleMaterial materials[];
};

out vec4 frag_color;

vec2 slab_test(vec3 cor1, vec3 cor2, vec3 pos, vec3 dir_inv) {
    // https://tavianator.com/2015/ray_box_nan.html
    precise vec3 t1 = (cor1 - pos) * dir_inv;
    precise vec3 t2 = (cor2 - pos) * dir_inv;

    precise float tmin = min(min(t1.x, t2.x), INF);
    precise float tmax = max(max(t1.x, t2.x), -INF);

    // Eliminates NaN problem
    tmin = max(tmin, min(min(t1.y, t2.y), tmax));
    tmax = min(tmax, max(max(t1.y, t2.y), tmin));

    tmin = max(tmin, min(min(t1.z, t2.z), tmax));
    tmax = min(tmax, max(max(t1.z, t2.z), tmin));

    return vec2(tmin, tmax);
}

vec3 snap_pos_down(vec3 pos, uint level, uint max_level) {
    return floor(pos * pow(2.0, float(max_level - level))) * pow(0.5, float(max_level - level));
}

uvec3 pos_to_bitmask(vec3 pos, uint max_level) {
    return uvec3(pos * float(1 << max_level)); // IDK?
}

float level_to_size(uint level, uint max_level) {
    return pow(0.5, float(max_level - level));
}

uint bitmask_to_index(uvec3 bitmask, uint level) {
    uvec3 temp = (uvec3(bitmask >> (level - 1)) & uint(1)) << uvec3(2, 1, 0); // There is minus one because it does not make sence to find children node at level 0
    return temp.x + temp.y + temp.z;
}

QueryResult query(uint svodag_index, vec3 pos, uint max_level) {
    uvec3 bitmask = pos_to_bitmask(pos, max_level);

    uint n_idx = svodag_index;
    for (uint i = max_level; i >= 0; i--) {
        uint index = bitmask_to_index(bitmask, i);
        uint new_idx = nodes[n_idx].addr[index];

        if (new_idx == 0) {
            return QueryResult(i, nodes[n_idx]);
        }
        // We are guaranteed to get new_idx == 0 when i == 0

        n_idx = new_idx;
    }

    // Just to be safe
    return QueryResult(0, nodes[n_idx]);
}

bool raymarch(uint index, vec3 origin, vec3 dir, out vec3 hit_pos, out QueryResult hit_query, out vec3 normal) {
    uint level = metadata[index].max_level; // Need to set this through uniform or ssbo content
    uint svodag_index = metadata[index].at_index;
    precise vec3 dir_inv = vec3(1.0) / dir;

    vec2 minmax = slab_test(vec3(0.0), vec3(1.0), origin, dir_inv);
    minmax.x = max(0.0, minmax.x);

    bool intersected = minmax.y > minmax.x;

    if (!intersected) {
        return false;
    }

    vec3 bias = level_to_size(0, level) * bias_amt * dir;
    vec3 cur_pos = origin + dir * minmax.x + bias;
    uint iters = 0;

    do {
        QueryResult result = query(svodag_index, cur_pos, level);

        if (result.node.mat_id != 0) {
            hit_pos = cur_pos - bias;
            hit_query = result;

            vec3 minpos = snap_pos_down(
                cur_pos,
                result.at_level,
                level
                );
            float size = level_to_size(result.at_level, level);
            vec3 maxpos = minpos + size;
            // Scale cur_pos to -1, -1, -1 ~ 1, 1, 1
            vec3 vox_pos = (2.0 * (cur_pos - minpos) / size) - 1.0;
            
            normal = normalize(step(0.999, vox_pos) - step(0.999, -vox_pos));
            return true;
        }

        float size = level_to_size(result.at_level, level);

        vec3 cur_vox_start = snap_pos_down(
                cur_pos,
                result.at_level,
                level
            );

        vec3 cur_vox_end = cur_vox_start + vec3(size);

        minmax = slab_test(
                cur_vox_start,
                cur_vox_end,
                cur_pos,
                dir_inv
            );

        cur_pos += minmax.y * dir + bias;

        iters++;
    }
    while (all(lessThan(cur_pos, vec3(1.0))) && all(lessThan(vec3(0.0), cur_pos)) && iters < 100);

    // if (iters >= 100) {
    //     // return vec4(0.0, 0.0, 1.0, 1.0);
    // }

    return false;
}

void main() {
    vec4 origin = metadata[0].model_inv * vec4(camera_pos, 1.0);
    vec4 dir = metadata[0].model_inv * vec4(frag_pos.x * camera_right + frag_pos.y * camera_up + camera_dir, 0.0);

    vec3 hit_pos;
    QueryResult hit_query;
    vec3 normal;

    bool result = raymarch(
            0,
            origin.xyz,
            normalize(dir.xyz),
            hit_pos,
            hit_query,
            normal
        );

    if (result) {
        // Shadow pass
        vec3 occluder_pos;
        QueryResult _ignore;
        vec3 _normal;
        
        bool shadow_result = raymarch(
            0,
            hit_pos + bias_amt * normal,
            SUN_DIR,
            occluder_pos,
            _ignore,
            _normal
            );

        if (shadow_result) {
            frag_color = vec4(0.0, 0.0, 0.0, 1.0);
        } else {
            frag_color = materials[hit_query.node.mat_id].albedo * vec4(SUN_COLOR, 1.0) * dot(normal, SUN_DIR);
        }
        
    } else {
        frag_color = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
