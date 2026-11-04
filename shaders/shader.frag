#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

void main() {
    // The model has no normals, so each face's direction comes from how the world
    // position changes between neighboring pixels. That gives one tone per triangle.
    vec3 normal = normalize(cross(dFdy(fragWorldPos), dFdx(fragWorldPos)));

    vec3 toLight = normalize(vec3(1.0, 0.4, 2.0));
    float diffuse = max(dot(normal, toLight), 0.0);
    float light = 0.25 + 0.75 * diffuse;   // 0.25 keeps faces turned away from going black

    vec4 color = texture(texSampler, fragTexCoord);
    outColor = vec4(normal * 0.5 + 0.5, 1.0);
}
