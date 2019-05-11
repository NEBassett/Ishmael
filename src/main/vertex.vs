#version 450 core

layout (location = 0) in vec4 inPos;

out vec3 simPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform sampler3D fluid;

void main()
{
    simPos = inPos.xyz;
    gl_Position = proj*view*model*vec4(inPos.xyz/4 + vec3(-3,-6.3,-14), 1.0);
}
