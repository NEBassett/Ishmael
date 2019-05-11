#version 450 core

out vec4 color;

in vec3 simPos;

uniform float time;
uniform sampler3D fluid;
uniform sampler3D pressure;
uniform sampler3D div;

void main()
{

  float texel = 1.0/40.0;
  vec3 ind = vec3(clamp(simPos.xyz, vec3(1), vec3(18)));
  vec3 fInd = simPos.xyz/40 + vec3(0.5,0.5,0.5)/40;

  float div1 =
    texture(pressure, fInd + vec3(texel,0,0)).x - texture(pressure, fInd - vec3(texel,0,0)).x +
    texture(pressure, fInd + vec3(0,texel,0)).y - texture(pressure, fInd - vec3(0,texel,0)).y +
    texture(pressure, fInd + vec3(0,0,texel)).z - texture(pressure, fInd - vec3(0,0,texel)).z;
  div1 = div1 / 2;

  vec3 grad = 0.5*vec3(
    texture(pressure, fInd + vec3(texel,0,0)).x - texture(pressure, fInd - vec3(texel,0,0)).x,
    texture(pressure, fInd + vec3(0,texel,0)).x - texture(pressure, fInd - vec3(0,texel,0)).x,
    texture(pressure, fInd + vec3(0,0,texel)).x - texture(pressure, fInd - vec3(0,0,texel)).x
  );

  color = vec4(abs(texture(fluid, fInd)));
}
