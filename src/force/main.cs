#version 430 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform int dim;
uniform float tdelta;

uniform sampler3D old;
uniform sampler3D fluid;
layout(binding = 0, rgba32f) writeonly uniform image3D new;

void main()
{
  ivec3 index = ivec3(gl_GlobalInvocationID);
  ivec3 center = ivec3(dim/2);
  float m = (1.0/(length(index-center)+0.1))/float(dim);

  imageStore(new, index, vec4(texelFetch(old, index, 0).xyz - m*vec3(0,tdelta*68.8,0), 0));
}
