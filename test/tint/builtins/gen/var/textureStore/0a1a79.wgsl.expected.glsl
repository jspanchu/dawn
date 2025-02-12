#version 310 es
precision highp float;
precision highp int;

layout(rgba8ui) uniform highp writeonly uimage2DArray arg_0;
void textureStore_0a1a79() {
  uvec2 arg_1 = uvec2(1u);
  int arg_2 = 1;
  uvec4 arg_3 = uvec4(1u);
  imageStore(arg_0, ivec3(uvec3(arg_1, uint(arg_2))), arg_3);
}

void fragment_main() {
  textureStore_0a1a79();
}

void main() {
  fragment_main();
  return;
}
#version 310 es

layout(rgba8ui) uniform highp writeonly uimage2DArray arg_0;
void textureStore_0a1a79() {
  uvec2 arg_1 = uvec2(1u);
  int arg_2 = 1;
  uvec4 arg_3 = uvec4(1u);
  imageStore(arg_0, ivec3(uvec3(arg_1, uint(arg_2))), arg_3);
}

void compute_main() {
  textureStore_0a1a79();
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  compute_main();
  return;
}
