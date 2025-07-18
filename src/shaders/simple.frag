#version 450 core
#define MLEVEL 8
#define INF 1.0/0.0

layout(location = 0) in vec2 frag_pos;

struct Node {
    vec4 color;
    uint addr[8];
};

struct SvodagMetaData {
    mat4 model_inv;
    uint max_depth;
    uint at_index;
};

struct QueryResult {
    uint at_level; // 0: At deepest level, MLEVEL: at root, because the level is in the unit of branches
    vec4 color;
};

layout(std430, binding = 3) buffer one {
    Node nodes[];
};

layout(std430, binding = 2) buffer two {
    SvodagMetaData metadata[];
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

QueryResult query(vec3 pos, uint max_level) {
    uvec3 bitmask = pos_to_bitmask(pos, max_level);

    uint n_idx = 1;
    for (uint i = max_level; i >= 0; i--) {
        uint index = bitmask_to_index(bitmask, i);
        uint new_idx = nodes[n_idx].addr[index];

        if (new_idx == 0) {
            return QueryResult(i, nodes[n_idx].color);
        }
        // We are guaranteed to get new_idx == 0 when i == 0

        n_idx = new_idx;
    }

    // Just to be safe
    return QueryResult(0, nodes[n_idx].color);
}

vec4 raymarch(vec3 origin, vec3 dir) {
    uint level = MLEVEL; // Need to set this through uniform or ssbo content
    vec3 dir_inv = vec3(1.0) / dir;

    vec2 minmax = slab_test(vec3(0.0), vec3(1.0), origin, dir_inv);
    minmax.x = max(0.0, minmax.x);

    bool intersected = minmax.y >= minmax.x;

    if (!intersected) {
        return vec4(1.0, 0.0, 0.0, 1.0);
    }

    vec3 cur_pos = origin + dir * minmax.x;
    uint iters = 0;

    vec3 bias = level_to_size(0, MLEVEL)*0.01 * dir;

    do {
        vec3 biased = cur_pos + bias;
        QueryResult result = query(biased, level);

        if (result.color.a > 0.0) {
            return result.color;
        }

        float size = level_to_size(result.at_level, level);

        vec3 cur_vox_start = snap_pos_down(
            biased,
            result.at_level,
            level
            );

        vec3 cur_vox_end = cur_vox_start + vec3(size);

        minmax = slab_test(
            cur_vox_start,
            cur_vox_end,
            biased,
            dir_inv
            );

        cur_pos = biased + minmax.y * dir;

        iters++;
    }
    while (all(lessThan(cur_pos, vec3(1.0))) && all(lessThan(vec3(0.0), cur_pos)) && iters < 100);

    // if (iters >= 100) {
    //     // return vec4(0.0, 0.0, 1.0, 1.0);
    // }

    return vec4(0.0, 0.0, 1.0, 1.0);
}

void main() {
    frag_color = raymarch(vec3(-1, 0.5, 0.5), vec3(1.0, frag_pos.xy/2));
}
