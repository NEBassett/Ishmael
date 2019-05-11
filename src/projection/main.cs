#version 430 core

precision highp float;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform int dim;
uniform float delta;

uniform sampler3D pressure;
layout(binding = 0, rgba32f) writeonly uniform image3D new;
layout(binding = 1, rgba32f) readonly uniform image3D velocity;

void main()
{
  ivec3 index = ivec3(gl_GlobalInvocationID);
  vec3 fInd = vec3(index+vec3(0.5))/(dim);
  float texel = 1.0/dim;

  imageStore(new, index, vec4(imageLoad(velocity, index).xyz - 0.5*vec3(
    texture(pressure, fInd + vec3(texel,0,0)).x - texture(pressure, fInd - vec3(texel,0,0)).x,
    texture(pressure, fInd + vec3(0,texel,0)).x - texture(pressure, fInd - vec3(0,texel,0)).x,
    texture(pressure, fInd + vec3(0,0,texel)).x - texture(pressure, fInd - vec3(0,0,texel)).x
    ), 1));
}
