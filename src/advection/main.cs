#version 430 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform int dim;
uniform float scale;
uniform float tdelta;

uniform sampler3D old;
uniform sampler3D velocity;
layout(binding = 0, rgba32f) writeonly uniform image3D new;

void main()
{

    // ivec3 index = ivec3(gl_FragCoord.x, int(gl_FragCoord.y)%dim, int(gl_FragCoord.y)/dim);
    // vec3 fInd = vec3(float(index.x)/float(dim-1), float(index.y)/float(dim-1), float(index.z)/float(dim-1));
    vec3 fInd = (vec3(gl_GlobalInvocationID)+vec3(0.5,0.5,0.5))/(dim);
    vec3 velo = tdelta*texture(velocity, fInd).xyz;

    // if(length(velo) < EPSILON)
    // {
    //   velo = vec3(0);
    // }

    imageStore(
      new,
      ivec3(gl_GlobalInvocationID),
      texture(old, fInd - velo).xyzw
    ); // normalize coordinates and velocity




}
