#version 430 core

precision highp float;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, rgba32f) writeonly uniform image3D new;

void main()
{
  ivec3 index = ivec3(gl_GlobalInvocationID);
  imageStore(new, index, vec4(0));
}
