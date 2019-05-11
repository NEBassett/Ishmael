#version 430 core

precision highp float;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, rgba32f) writeonly uniform image3D new;
layout(binding = 1, rgba32f) readonly uniform image3D a;
uniform sampler3D b;

uniform int dim;
uniform float delta;

void main()
{
  ivec3 index = ivec3(gl_GlobalInvocationID);
  vec3 fInd = vec3(index+vec3(0.5))/(dim);
  float texel = 1.0/dim;

  float laplacian = (
    texture(b, fInd + vec3(texel,0,0)).x + texture(b, fInd - vec3(texel,0,0)).x +
    texture(b, fInd + vec3(0,texel,0)).x + texture(b, fInd - vec3(0,texel,0)).x +
    texture(b, fInd + vec3(0,0,texel)).x + texture(b, fInd - vec3(0,0,texel)).x -
    texture(b, fInd).x*6
  );

  float div = imageLoad(a, index).x;
  bool a = (div == laplacian);
  float val = 0.0;
  if(abs(div - laplacian) < 0.01)
    val = 1.0;

  imageStore(new, index, vec4(val));
}
