SKIP: FAILED

#version 310 es

layout(rgba8ui) uniform highp uimage3D arg_0;
layout(binding = 0, std430) buffer prevent_dce_block_ssbo {
  uvec3 inner;
} prevent_dce;

void textureDimensions_475c10() {
  uvec3 res = uvec3(imageSize(arg_0));
  prevent_dce.inner = res;
}

vec4 vertex_main() {
  textureDimensions_475c10();
  return vec4(0.0f);
}

void main() {
  gl_PointSize = 1.0;
  vec4 inner_result = vertex_main();
  gl_Position = inner_result;
  gl_Position.y = -(gl_Position.y);
  gl_Position.z = ((2.0f * gl_Position.z) - gl_Position.w);
  return;
}
Error parsing GLSL shader:
ERROR: 0:3: 'rgba8ui' : format requires readonly or writeonly memory qualifier 
ERROR: 1 compilation errors.  No code generated.



#version 310 es
precision highp float;

layout(rgba8ui) uniform highp uimage3D arg_0;
layout(binding = 0, std430) buffer prevent_dce_block_ssbo {
  uvec3 inner;
} prevent_dce;

void textureDimensions_475c10() {
  uvec3 res = uvec3(imageSize(arg_0));
  prevent_dce.inner = res;
}

void fragment_main() {
  textureDimensions_475c10();
}

void main() {
  fragment_main();
  return;
}
Error parsing GLSL shader:
ERROR: 0:4: 'rgba8ui' : format requires readonly or writeonly memory qualifier 
ERROR: 1 compilation errors.  No code generated.



#version 310 es

layout(rgba8ui) uniform highp uimage3D arg_0;
layout(binding = 0, std430) buffer prevent_dce_block_ssbo {
  uvec3 inner;
} prevent_dce;

void textureDimensions_475c10() {
  uvec3 res = uvec3(imageSize(arg_0));
  prevent_dce.inner = res;
}

void compute_main() {
  textureDimensions_475c10();
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  compute_main();
  return;
}
Error parsing GLSL shader:
ERROR: 0:3: 'rgba8ui' : format requires readonly or writeonly memory qualifier 
ERROR: 1 compilation errors.  No code generated.



