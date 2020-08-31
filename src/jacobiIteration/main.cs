#version 430 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform int dim;
uniform float beta;
uniform float alpha;

uniform sampler3D old;
layout(binding = 1, rgba32f) writeonly uniform image3D new;
layout(binding = 2, rgba32f) readonly uniform image3D poissonTarget;

void main()
{
  ivec3 index = ivec3(gl_GlobalInvocationID);
  vec3 fInd = vec3(index+vec3(0.5))/(dim);
  float texel = 1.0/dim;

  imageStore(new, index, vec4(
    texture(old, fInd + vec3(texel,0,0)) +
    texture(old, fInd + vec3(-texel,0,0)) +
    texture(old, fInd + vec3(0,texel,0)) +
    texture(old, fInd + vec3(0,-texel,0)) +
    texture(old, fInd + vec3(0,0,texel)) +
    texture(old, fInd + vec3(0,0,-texel)) +
    alpha*imageLoad(poissonTarget, index))/beta);
}
