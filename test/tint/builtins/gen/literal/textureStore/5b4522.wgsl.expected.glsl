#version 310 es
precision highp float;
precision highp int;

layout(rgba8ui) uniform highp writeonly uimage2D arg_0;
void textureStore_5b4522() {
  imageStore(arg_0, ivec2(uvec2(1u, 0u)), uvec4(1u));
}

void fragment_main() {
  textureStore_5b4522();
}

void main() {
  fragment_main();
  return;
}
#version 310 es

layout(rgba8ui) uniform highp writeonly uimage2D arg_0;
void textureStore_5b4522() {
  imageStore(arg_0, ivec2(uvec2(1u, 0u)), uvec4(1u));
}

void compute_main() {
  textureStore_5b4522();
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  compute_main();
  return;
}
