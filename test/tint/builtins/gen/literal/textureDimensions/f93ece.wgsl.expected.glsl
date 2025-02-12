#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430) buffer prevent_dce_block_ssbo {
  uvec3 inner;
  uint pad;
} prevent_dce;

layout(rgba32f) uniform highp writeonly image3D arg_0;
uvec3 textureDimensions_f93ece() {
  uvec3 res = uvec3(imageSize(arg_0));
  return res;
}

void fragment_main() {
  prevent_dce.inner = textureDimensions_f93ece();
}

void main() {
  fragment_main();
  return;
}
#version 310 es

layout(binding = 0, std430) buffer prevent_dce_block_ssbo {
  uvec3 inner;
  uint pad;
} prevent_dce;

layout(rgba32f) uniform highp writeonly image3D arg_0;
uvec3 textureDimensions_f93ece() {
  uvec3 res = uvec3(imageSize(arg_0));
  return res;
}

void compute_main() {
  prevent_dce.inner = textureDimensions_f93ece();
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  compute_main();
  return;
}
