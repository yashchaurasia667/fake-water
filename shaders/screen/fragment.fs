#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D u_screenTexture;
uniform sampler2D u_depthTexture;

uniform float u_near;
uniform float u_far;
uniform float u_fogDensity;
uniform vec3  u_fogColor;
uniform float u_horizonBand;     // how many units above/below horizon the fog fades (0.05–0.3)
uniform mat4  u_invProjection;
uniform mat4  u_invViewRot;      // inverse of view rotation (transpose of mat3(view))

float lineariseDepth(float d)
{
    return (2.0 * u_near * u_far) / (u_far + u_near - d * (u_far - u_near));
}

void main()
{
    vec3  sceneColor = texture(u_screenTexture, TexCoord).rgb;
    float rawDepth   = texture(u_depthTexture,  TexCoord).r;

    if (rawDepth >= 1.0)
    {
        // Reconstruct world-space view direction for this skybox pixel.
        // This follows the camera so the horizon band is always at the actual horizon
        // regardless of camera pitch or yaw.
        vec2 ndc      = TexCoord * 2.0 - 1.0;
        vec4 clipDir  = vec4(ndc, 1.0, 1.0);
        vec4 viewDir  = u_invProjection * clipDir;
        vec3 worldDir = normalize(mat3(u_invViewRot) * (viewDir.xyz / viewDir.w));

        // worldDir.y == 0 is the exact horizon, positive is sky, negative is below.
        // smoothstep: full fog at/below horizon, fades to clear at u_horizonBand above it.
        float horizonFog = 1.0 - smoothstep(-u_horizonBand, u_horizonBand, worldDir.y);
        FragColor = vec4(mix(sceneColor, u_fogColor, horizonFog), 1.0);
        return;
    }

    float dist = lineariseDepth(rawDepth);
    float fog  = 1.0 - exp(-pow(u_fogDensity * dist, 2.0));
    fog        = clamp(fog, 0.0, 1.0);

    FragColor = vec4(mix(sceneColor, u_fogColor, fog), 1.0);
}