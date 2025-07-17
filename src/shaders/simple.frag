#version 450 core
#define MLEVEL 3

layout(location = 0) in vec2 frag_pos;

struct Node {
    vec4 color;
    uint addr[8];
};

struct QueryResult {
    uint at_level; // 0: At deepest level, MLEVEL: at root, because the level is in the unit of branches
    vec4 color;
};

layout(std430, binding = 3) buffer asdf {
    Node nodes[];
};

out vec4 frag_color;

vec4 report = vec4(0.2, 0.0, 0.0, 1.0);

vec2 slab_test(vec3 cor1, vec3 cor2, vec3 pos, vec3 dir_inv) {
    vec3 t1 = (cor1 - pos) * dir_inv;
    vec3 t2 = (cor2 - pos) * dir_inv;

    float tmin = min(t1.x, t2.x);
    float tmax = max(t1.x, t2.x);

    tmin = max(tmin, min(min(t1.y, t2.y), tmax));
    tmax = min(tmax, max(max(t1.y, t2.y), tmin));

    tmin = max(tmin, min(min(t1.z, t2.z), tmax));
    tmax = min(tmax, max(max(t1.z, t2.z), tmin));

    // report = vec4(tmin, tmin, tmin, 1.0);

    return vec2(tmin, tmax);
}

vec4 slab_test_min(vec3 cor1, vec3 cor2, vec3 pos, vec3 dir_inv) {
    vec3 t1 = (cor1 - pos) * dir_inv;
    vec3 t2 = (cor2 - pos) * dir_inv;

    float tmin = min(t1.x, t2.x);
    float tmax = max(t1.x, t2.x);

    vec3 which = vec3(1.0, 0.0, 0.0); // Which axis was limiting

    float temp1 = min(min(t1.y, t2.y), tmax);
    tmin = max(tmin, temp1);
    which = (tmin > temp1) ? which : vec3(0.0, 1.0, 0.0);
    tmax = min(tmax, max(max(t1.y, t2.y), tmin));

    temp1 = min(min(t1.z, t2.z), tmax);
    tmin = max(tmin, temp1);
    which = (tmin > temp1) ? which : vec3(0.0, 0.0, 1.0);
    tmax = min(tmax, max(max(t1.z, t2.z), tmin));

    return vec4(tmin, which);
}

vec4 slab_test_max(vec3 cor1, vec3 cor2, vec3 pos, vec3 dir_inv) {
    vec3 t1 = (cor1 - pos) * dir_inv;
    vec3 t2 = (cor2 - pos) * dir_inv;

    float tmin = min(t1.x, t2.x);
    float tmax = max(t1.x, t2.x);

    vec3 which = vec3(1.0, 0.0, 0.0); // Which axis was limiting

    tmin = max(tmin, min(min(t1.y, t2.y), tmax));
    float temp1 = max(max(t1.y, t2.y), tmin);
    tmax = min(tmax, temp1);
    which = (tmin < temp1) ? which : vec3(0.0, 1.0, 0.0);

    tmin = max(tmin, min(min(t1.z, t2.z), tmax));
    temp1 = max(max(t1.z, t2.z), tmin);
    tmax = min(tmax, temp1);
    which = (tmin < temp1) ? which : vec3(0.0, 0.0, 1.0);

    return vec4(tmin, which);
}

vec3 snap_pos(vec3 pos, uint level, uint max_level) {
    return floor(pos * float(1 << (max_level - level))) * pow(0.5, float(max_level - level));
}

uvec3 pos_to_bitmask(vec3 pos, uint max_level) {
    return uvec3(pos * float(1 << max_level)); // IDK?
}

float level_to_size(uint level, uint max_level) {
    return pow(0.5, float(max_level - level));
}

uint bitmask_to_index(uvec3 bitmask, uint level) {
    uvec3 temp = ((bitmask >> (level - 1)) & 1) << uvec3(2, 1, 0); // There is minus one because it does not make sence to find children node at level 0
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

        n_idx = new_idx;
    }

    return QueryResult(1, nodes[n_idx].color);
}

vec4 raymarch(vec3 origin, vec3 dir) {
    uint level = MLEVEL; // Need to set this through uniform or ssbo content
    vec3 dir_inv = vec3(1.0) / dir;
    // vec3 dir_sign = vec3(
    //     dir.x >= 0.f ? 1.f : -1.f,
    //     dir.y >= 0.f ? 1.f : -1.f,
    //     dir.y >= 0.f ? 1.f : -1.f
    // );
    vec3 dir_sign = mix( /*false*/ vec3(-1.0), vec3(0.0), lessThanEqual(vec3(0.0), dir));

    vec2 minmax = slab_test(vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0), origin, dir_inv);
    minmax.x = max(0.0, minmax.x);

    bool intersected = minmax.y > minmax.x;

    if (!intersected) {
        return vec4(1.0, 0.0, 0.0, 1.0);
    }

    vec3 cur_pos = origin + dir * minmax.x;
    uint iters = 0;

    vec3 which_axis_hit = slab_test_min(vec3(0.0), vec3(1.0), origin, dir_inv).yzw;

    do {
        vec3 biased = cur_pos + dir * level_to_size(0, MLEVEL) * 0.01;
        // vec3 biased = cur_pos;
        cur_pos = biased;
        QueryResult result = query(biased, MLEVEL);

        if (result.color.a > 0.0) {
            // return result.color;
            return vec4(0.0, 1.0, 0.0, 1.0);
        }

        // return vec4(vec3(level_to_size(result.at_level, MLEVEL)), 1.0);

        float size = level_to_size(result.at_level, MLEVEL);

        vec3 cur_vox_start = snap_pos(
                biased,
                result.at_level,
                MLEVEL
            );

        // vec3 sgn = mix(vec3(1.0), dir_sign, equal(cur_vox_start, cur_pos));
        // vec3 sgn = vec3(1.0);
        vec3 sgn = min(vec3(1.0), (which_axis_hit * dir_sign));
        // return vec4(which_axis_hit, 1.0);

        vec3 cur_vox_end = cur_vox_start + sgn * size;

        vec4 new_minmax = slab_test_max(
                cur_vox_start,
                cur_vox_end,
                biased,
                dir_inv
            );

        cur_pos = new_minmax.x * dir;
        which_axis_hit = new_minmax.yzw;

        iters++;
    }
    while (all(lessThan(cur_pos, vec3(1.0))) && all(lessThan(vec3(0.0), cur_pos)) && iters < 100);

    if (iters >= 100) {
        return vec4(cur_pos, 1.0);
    }
    // return vec4(cur_pos, 1.0);
    return vec4(0.0, 0.0, 1.0, 1.0);
}

void main() {
    // frag_color = raymarch(vec3(-1.0, 0.5, 0.5), normalize(vec3(1.0, frag_pos.xy/2.0)));
    // frag_color = vec4(frag_pos.xy, 0.0, 1.0);
    // raymarch(vec3(-1.0, frag_pos.xy), vec3(1.0, 0.0, 0.0));
    
    report  = raymarch(vec3(-1.0, 0.5, 0.5), normalize(vec3(1.0, frag_pos.xy/2.0)));
    frag_color = report;
}
