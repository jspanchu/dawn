#version 310 es
precision highp float;
precision highp int;

layout(r32ui) uniform highp uimage2D arg_0;
void textureStore_32d3d6() {
  imageStore(arg_0, ivec2(uvec2(1u, 0u)), uvec4(1u));
}

void fragment_main() {
  textureStore_32d3d6();
}

void main() {
  fragment_main();
  return;
}
#version 310 es

layout(r32ui) uniform highp uimage2D arg_0;
void textureStore_32d3d6() {
  imageStore(arg_0, ivec2(uvec2(1u, 0u)), uvec4(1u));
}

void compute_main() {
  textureStore_32d3d6();
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  compute_main();
  return;
}
