#version 430 core

precision highp float;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform int dim;
uniform float scale;

uniform sampler3D old;
layout(binding = 0, rgba32f) writeonly uniform image3D new;

void main()
{
  ivec3 index = ivec3(gl_GlobalInvocationID);
  ivec3 interior = index;

  if(index.x == dim-1)
  {
    interior = interior - ivec3(1,0,0);
  } else {
    if(index.x == 0)
    {
      interior = interior + ivec3(1,0,0);
    }
  }
  if(index.y == dim-1)
  {
    interior = interior - ivec3(0,1,0);
  } else {
    if(index.y == 0)
    {
      interior = interior + ivec3(0,1,0);
    }
  }
  if(index.z == dim-1)
  {
    interior = interior - ivec3(0,0,1);
  } else {
    if(index.z == 0)
    {
      interior = interior + ivec3(0,0,1);
    }
  }

  if(interior != index)
  {
    imageStore(new, index, scale*texelFetch(old, interior, 0));
  } else {
    imageStore(new, index, texelFetch(old, index, 0));
  }
}
