#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

layout(binding = 1) uniform sampler2D iChannel0;

// Rotating 3D cube shader compatible with metalshader
// Procedurally generates geometry and animation

mat4 rotationMatrix(vec3 axis, float angle) {
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

mat4 perspectiveMatrix(float fov, float aspect, float near, float far) {
    float f = 1.0 / tan(fov / 1.0);
    return mat4(
        f / aspect, 0.0, 0.0, 0.0,
        0.0, f, 0.0, 0.0,
        0.0, 0.0, (far + near) / (near - far), -1.0,
        0.0, 0.0, (2.0 * far * near) / (near - far), 0.0
    );
}

// Ray-box intersection for cube rendering (safe version)
float boxIntersect(vec3 ro, vec3 rd, vec3 boxSize) {
    // Prevent division by zero
    vec3 m = 1.0 / (rd + vec3(1e-8));
    vec3 n = m * ro;
    vec3 k = abs(m) * boxSize;
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;
    float tN = max(max(t1.x, t1.y), t1.z);
    float tF = min(min(t2.x, t2.y), t2.z);
    if (tN > tF || tF < 0.0) return -1.0;
    return tN;
}

void main() {
    // Normalize coordinates
    vec2 uv = (fragCoord - 0.5 * ubo.iResolution.xy) / ubo.iResolution.y;

    // Camera setup
    vec3 ro = vec3(0.0, 0.0, 5.0); // Ray origin
    vec3 rd = normalize(vec3(uv, -1.5)); // Ray direction

    // Rotate the entire scene (camera + ray) to animate the cube
    float angle = ubo.iTime;
    mat4 rot = rotationMatrix(normalize(vec3(1.0, 1.0, 0.0)), angle);
    ro = (rot * vec4(ro, 1.0)).xyz;
    rd = normalize((rot * vec4(rd, 0.0)).xyz);

    // Cube intersection
    vec3 boxSize = vec3(0.8);
    float t = boxIntersect(ro, rd, boxSize);

    vec3 col = vec3(0.005, 0.005, 0.01); // Background
    // vec3 col = vec3(0.1, 0.1, 0.15); // Background

    if (t > 0.0) {
        // Hit point
        vec3 pos = ro + rd * t;

        // Calculate normal for lighting
        vec3 absPos = abs(pos);
        float maxComp = max(max(absPos.x, absPos.y), absPos.z);

        vec3 normal = vec3(0.0);
        if (abs(absPos.x - maxComp) < 0.001) {
            normal = vec3(sign(pos.x), 0.0, 0.0);
        } else if (abs(absPos.y - maxComp) < 0.001) {
            normal = vec3(0.0, sign(pos.y), 0.0);
        } else {
            normal = vec3(0.0, 0.0, sign(pos.z));
        }

        // Create smooth color gradients based on position within the cube
        // Normalize position to [0,1] range for gradient
        vec3 normPos = (pos / boxSize) * 0.5 + 0.5;

        // Base color with smooth gradients across all axes
        vec3 baseColor = vec3(
            0.5 + 0.5 * sin(normPos.x * 3.14159 + ubo.iTime * 0.5),
            0.5 + 0.5 * sin(normPos.y * 3.14159 + ubo.iTime * 0.7 + 2.0),
            0.5 + 0.5 * sin(normPos.z * 3.14159 + ubo.iTime * 0.3 + 4.0)
        );

        // Add directional lighting
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.5));
        float diffuse = max(dot(normal, lightDir), 0.0);
        float ambient = 0.3;
        float lighting = ambient + diffuse * 0.7;

        // Apply lighting to color
        col = baseColor * lighting;

        // Add subtle specular highlight
        vec3 viewDir = normalize(-rd);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        col += vec3(spec * 0.3);

        // Subtle edge highlighting for depth
        vec3 grid = step(0.95, fract(pos * 5.0));
        float edge = max(max(grid.x, grid.y), grid.z);
        col = mix(col, col * 1.2, edge * 0.3);
    }

    fragColor = vec4(col, 1.0);
}
