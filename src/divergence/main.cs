#version 430 core


layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform int dim;
uniform float delta;

uniform sampler3D old;
layout(binding = 0, rgba32f) writeonly uniform image3D new;

void main()
{
  ivec3 index = ivec3(gl_GlobalInvocationID);
  vec3 fInd = vec3(index+vec3(0.5))/(dim);
  float texel = 1.0/dim;

  imageStore(new, index, vec4(delta*0.5*(
    texture(old, fInd + vec3(texel,0,0)).x - texture(old, fInd - vec3(texel,0,0)).x +
    texture(old, fInd + vec3(0,texel,0)).y - texture(old, fInd - vec3(0,texel,0)).y +
    texture(old, fInd + vec3(0,0,texel)).z - texture(old, fInd - vec3(0,0,texel)).z
    )));
}
