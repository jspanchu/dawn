#version 310 es
precision highp float;
precision highp int;

layout(rgba32ui) uniform highp writeonly uimage2DArray arg_0;
void textureStore_8e0479() {
  imageStore(arg_0, ivec3(ivec2(1), 1), uvec4(1u));
}

void fragment_main() {
  textureStore_8e0479();
}

void main() {
  fragment_main();
  return;
}
#version 310 es

layout(rgba32ui) uniform highp writeonly uimage2DArray arg_0;
void textureStore_8e0479() {
  imageStore(arg_0, ivec3(ivec2(1), 1), uvec4(1u));
}

void compute_main() {
  textureStore_8e0479();
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  compute_main();
  return;
}
