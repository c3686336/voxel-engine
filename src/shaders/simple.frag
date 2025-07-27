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
layout(location = 8) uniform uint n_models;
layout(location = 9) uniform vec4 m_albedo;
layout(location = 10) uniform float m_metallicity;
layout(location = 11) uniform float m_roughness;

vec3 base_refl = m_albedo.xyz;

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

    precise vec3 tminvec = min(min(t1, t2), INF);
    precise vec3 tmaxvec = max(max(t1, t2), -INF);

    float tmin = max(tminvec.x, max(tminvec.y, tminvec.z));
    float tmax = min(tmaxvec.x, min(tmaxvec.y, tmaxvec.z));

    return vec2(tmin, tmax);
}

vec2 slab_test(vec3 cor1, vec3 cor2, vec3 pos, vec3 dir_inv, out bvec3 min_hit_dir, out bvec3 max_hit_dir) {
    // https://tavianator.com/2015/ray_box_nan.html
    precise vec3 t1 = (cor1 - pos) * dir_inv;
    precise vec3 t2 = (cor2 - pos) * dir_inv;

    precise vec3 tminvec = min(min(t1, t2), INF);
    precise vec3 tmaxvec = max(max(t1, t2), -INF);

    float tmin = max(tminvec.x, max(tminvec.y, tminvec.z));
    float tmax = min(tmaxvec.x, min(tmaxvec.y, tmaxvec.z));

    min_hit_dir = equal(tminvec, vec3(tmin));
    max_hit_dir = equal(tmaxvec, vec3(tmax));

    return vec2(tmin, tmax);
}

vec3 snap_pos_down(vec3 pos, uint level, uint max_level) {
    return floor(pos * pow(2.0, float(max_level - level))) * pow(0.5, float(max_level - level));
}

vec3 snap_pos_up(vec3 pos, uint level, uint max_level) {
    return ceil(pos * pow(2.0, float(max_level - level))) * pow(0.5, float(max_level - level));
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

QueryResult query(uint svodag_index, uvec3 bitmask, uint max_level) {
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

QueryResult query(uint svodag_index, vec3 pos, uint max_level) {
    return query(svodag_index, pos_to_bitmask(pos, max_level), max_level);
}

bool raymarch_model(uint svodag_index, uint level, vec3 cur_pos, vec3 bias, vec3 dir, vec3 dir_inv, bvec3 entry_norm, out vec3 hit_pos, out QueryResult hit_query, out vec3 normal) {
    uint iters = 0;

    bvec3 limiting_axis_min = entry_norm;
    bvec3 limiting_axis_max = entry_norm;

    do {
        QueryResult result = query(svodag_index, cur_pos, level);

        float size = level_to_size(result.at_level, level);

        vec3 cur_vox_start = snap_pos_down(
            cur_pos,
            result.at_level,
            level
            );

        vec3 cur_vox_end = cur_vox_start + vec3(size);

        if (result.node.mat_id != 0) {
            normal = -sign(dir) * vec3(limiting_axis_max);
            hit_pos = cur_pos; // - bias;
            hit_query = result;

            return true;
        }

        vec2 minmax = slab_test(
            cur_vox_start,
            cur_vox_end,
            cur_pos,
            dir_inv,
            limiting_axis_min,
            limiting_axis_max
            );

        minmax = max(vec2(0.0, 0.0), minmax);

        cur_pos += minmax.y * dir + bias + vec3(size) * 0.01 * (minmax.y == 0.0? vec3(limiting_axis_max) * sign(dir) : vec3(0.0));

        iters++;
    }
    while (all(lessThan(cur_pos, vec3(1.0))) && all(lessThan(vec3(0.0), cur_pos)) && iters < 100);

    return false;
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

        bvec3 limiting_axis_min;
        bvec3 limiting_axis_max;
        
        vec2 minmax_modelsp = slab_test(vec3(0.0), vec3(1.0), origin_modelsp.xyz, dir_inv_modelsp, limiting_axis_min, limiting_axis_max);
        
        minmax_modelsp.x = max(0.0, minmax_modelsp.x);

        bool intersected = minmax_modelsp.y > minmax_modelsp.x;

        vec4 min_intersect_displacement_worldsp = model_mat * dir_modelsp * minmax_modelsp.x;

        if (!intersected || hit_dist_squared < dot(min_intersect_displacement_worldsp, min_intersect_displacement_worldsp)) {
            continue;
        }

        vec4 bias_modelsp = level_to_size(0, level) * bias_amt * dir_modelsp;
        vec4 cur_pos_modelsp = clamp(origin_modelsp + dir_modelsp * minmax_modelsp.x - bias_modelsp, 0.0, 1.0);

        vec3 hit_pos_candidate_modelsp;
        QueryResult hit_query_candidate;
        vec3 normal_candidate_modelsp;

        bool result = raymarch_model(
                svodag_index,
                level,
                cur_pos_modelsp.xyz,
                bias_modelsp.xyz,
                dir_modelsp.xyz,
                dir_inv_modelsp.xyz,
                limiting_axis_min,
                hit_pos_candidate_modelsp,
                hit_query_candidate,
                normal_candidate_modelsp
            );

        if (!result) continue;

        vec4 hit_pos_candidate_worldsp = model_mat * vec4(hit_pos_candidate_modelsp, 1.0);
        float hit_dist_squared_candidate = dot(hit_pos_candidate_worldsp - origin, hit_pos_candidate_worldsp - origin);
        vec3 normal_candidate_worldsp = normalize(normal_mat * normal_candidate_modelsp);

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

vec3 flambert(vec3 albedo) {
    return albedo / PI;
}

float D(vec3 N, vec3 H, float alpha) {
    float numerator = alpha * alpha;

    float a = max(dot(N, H), 0.0);
    float sqterm = a*a * (numerator - 1.0) + 1.0;

    return numerator / max(PI * sqterm * sqterm, 0.000001);
}

float G1(vec3 N, vec3 X, float alpha) {
    float numerator = max(dot(N, X), 0.0);

    float k = alpha / 2.0;
    float denominator = numerator * (1.0 - k) + k;

    return numerator / max(denominator, 0.000001);
}

float G(vec3 N, vec3 V, vec3 L, float alpha) {
    return G1(N, V, alpha) * G1(N, L, alpha);
}

vec3 F(vec3 V, vec3 H, vec3 F0) {
    return F0 + (vec3(1.0) - F0) * pow(1 - max(dot(V, H), 0.0), 5.0);
}

vec3 fcook_torrance(vec3 N, vec3 V, vec3 L, vec3 H, vec3 F0, float alpha) {
    float d = D(N, H, alpha);
    float g = G(N, V, L, alpha);
    vec3 f = F(V, H, F0);

    float numeator = 4.0 * max(dot(V, N), 0.0) * max(dot(L, N), 0.0);
    
    return d * g * f / max(numeator, 0.000001);
}

vec3 brdf(vec3 N, vec3 V, vec3 L, vec3 albedo, vec3 F0, float roughness, float metallicity) {
    vec3 H = normalize(N + L);
    
    float alpha = roughness * roughness;
    vec3 ks = F(V, H, F0);
    vec3 kd = (vec3(1.0) - ks) * (1.0 - metallicity);

    return kd * flambert(albedo) + fcook_torrance(N, V, L, H, F0, alpha);
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
            first_hit_pos + 0.001 * vec4(first_hit_normal, 0.0), vec4(SUN_DIR, 0.0),
            shadow_hit_pos, shadow_hit_query, shadow_hit_normal, shadow_hit_index
            );

        if (shadow_result) {
            frag_color = vec4(0.0, 0.0, 0.0, 0.0);
        } else {
            vec3 F0 = mix(vec3(0.04), m_albedo.rgb, m_metallicity);
            frag_color = vec4(
                brdf(
                    first_hit_normal,
                    -normalize(cam_ray_dir.xyz),
                    SUN_DIR,
                    m_albedo.rgb,
                    F0,
                    m_roughness,
                    m_metallicity
                    ) * max(dot(first_hit_normal, SUN_DIR), 0.0),
                1.0
                );
        }
    } else {
        frag_color = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
