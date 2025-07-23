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
layout(location = 7) uniform uint model_select;
layout(location = 8) uniform uint n_models;

struct Node {
    uint mat_id;
    uint addr[8];
};

struct SvodagMetaData {
    mat4 model_inv; // World space -> Model space
    mat4 model; // Model space -> World space
    mat4 model_norm; // Model space normal -> World space normal
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

    uint n_idx = 0;
    for (uint i = max_level; i >= 0; i--) {
        uint index = bitmask_to_index(bitmask, i);
        uint new_idx = nodes[n_idx + svodag_index].addr[index];

        if (new_idx == 0) {
            return QueryResult(i, nodes[n_idx + svodag_index]);
        }
        // We are guaranteed to get new_idx == 0 when i == 0

        n_idx = new_idx;
    }

    // Just to be safe
    return QueryResult(0, nodes[n_idx]);
}

bool raymarch_model(uint svodag_index, uint level, vec3 cur_pos, vec3 dir, vec3 dir_inv, vec3 bias, out vec3 hit_pos, out QueryResult hit_query, out vec3 normal) {
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

        vec2 minmax = slab_test(
                cur_vox_start,
                cur_vox_end,
                cur_pos,
                dir_inv
            );

        cur_pos += minmax.y * dir + bias;

        iters++;
    }
    while (all(lessThan(cur_pos, vec3(1.0))) && all(lessThan(vec3(0.0), cur_pos)) && iters < 100);

    return false;
}

bool raymarch(uint index, vec3 origin, vec3 dir, out vec3 hit_pos, out QueryResult hit_query, out vec3 normal) {
    uint level = metadata[index].max_level;
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

    return raymarch_model(svodag_index, level, cur_pos, dir, dir_inv, bias, hit_pos, hit_query, normal);

    // if (iters >= 100) {
    //     // return vec4(0.0, 0.0, 1.0, 1.0);
    // }
}

bool trace(vec4 origin, vec4 dir, out vec4 hit_pos, out QueryResult hit_query, out vec3 normal, out uint hit_model_index) {
    float hit_dist_squared = INF;

    for (int i = 0; i < n_models; i++) {
        uint level = metadata[i].max_level;
        uint svodag_index = metadata[i].at_index;
        mat4 model_inv_mat = metadata[i].model_inv;
        mat4 model_mat = metadata[i].model;
        mat3 normal_mat = mat3(metadata[i].model_norm);

        vec4 dir_modelsp = normalize(model_inv_mat * dir); // It does not make sence to normalize homogeneous vec4 that represents point, ie .w == 1.0, so this is correct
        vec4 origin_modelsp = model_inv_mat * origin;
        precise vec3 dir_inv_modelsp = 1.0 / dir_modelsp.xyz; // Avoid divide-by-zero

        vec2 minmax_modelsp = slab_test(vec3(0.0), vec3(1.0), origin_modelsp.xyz, dir_inv_modelsp);
        minmax_modelsp.x = max(0.0, minmax_modelsp.x);

        bool intersected = minmax_modelsp.y > minmax_modelsp.x;

        vec4 min_intersect_displacement_worldsp = model_mat * dir_modelsp * minmax_modelsp.x;

        if (!intersected || hit_dist_squared < dot(min_intersect_displacement_worldsp, min_intersect_displacement_worldsp)) {
            continue;
        }

        vec4 bias_modelsp = level_to_size(0, level) * bias_amt * dir_modelsp;
        vec4 cur_pos_modelsp = origin_modelsp + dir_modelsp * minmax_modelsp.x + bias_modelsp;

        vec3 hit_pos_candidate_modelsp;
        QueryResult hit_query_candidate;
        vec3 normal_candidate_modelsp;

        bool result = raymarch_model(
                svodag_index,
                level,
                cur_pos_modelsp.xyz,
                dir_modelsp.xyz,
                dir_inv_modelsp.xyz,
                bias_modelsp.xyz,
                hit_pos_candidate_modelsp,
                hit_query_candidate,
                normal_candidate_modelsp
            );

        if (!result) continue;

        vec4 hit_pos_candidate_worldsp = model_mat * vec4(hit_pos_candidate_modelsp, 1.0);
        float hit_dist_squared_candidate = dot(hit_pos_candidate_worldsp - origin, hit_pos_candidate_worldsp - origin);
        vec3 normal_candidate_worldsp = normal_mat * normal_candidate_modelsp;

        bool closer = hit_dist_squared_candidate < hit_dist_squared;

        hit_pos = closer ?
            hit_pos_candidate_worldsp : hit_pos;
        normal = closer ?
            normal_candidate_worldsp : normal;
        hit_query = closer ?
            hit_query_candidate : hit_query;
        hit_model_index = closer ?
            i : hit_model_index;
        hit_dist_squared = min(hit_dist_squared_candidate, hit_dist_squared);
    }

    return !isinf(hit_dist_squared);
}

void main() {
    vec4 cam_ray_origin = vec4(camera_pos, 1.0);
    vec4 cam_ray_dir = vec4(frag_pos.x * camera_right + frag_pos.y * camera_up + camera_dir, 0.0);

    vec4 first_hit_pos;
    QueryResult first_hit_query;
    vec3 first_hit_normal;
    uint first_hit_index;

    bool result = trace(
        cam_ray_origin, cam_ray_dir,
        first_hit_pos, first_hit_query, first_hit_normal, first_hit_index
        );

    if (result) {
        // Cast a shadow ray
        vec4 shadow_hit_pos;
        QueryResult shadow_hit_query;
        vec3 shadow_hit_normal;
        uint shadow_hit_index;

        bool shadow_result = trace(
            first_hit_pos + bias_amt * vec4(first_hit_normal, 0.0), vec4(SUN_DIR, 0.0),
            shadow_hit_pos, shadow_hit_query, shadow_hit_normal, shadow_hit_index
            );

        if (shadow_result) {
            frag_color = vec4(0.0, 0.0, 0.0, 0.0);
        } else {
            frag_color = vec4(SUN_COLOR * dot(SUN_DIR, first_hit_normal), 1.0);
        }
    } else {
        frag_color = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
