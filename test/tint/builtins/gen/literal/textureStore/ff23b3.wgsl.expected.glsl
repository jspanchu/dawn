#version 310 es
precision highp float;
precision highp int;

layout(rgba16ui) uniform highp writeonly uimage2D arg_0;
void textureStore_ff23b3() {
  imageStore(arg_0, ivec2(1, 0), uvec4(1u));
}

void fragment_main() {
  textureStore_ff23b3();
}

void main() {
  fragment_main();
  return;
}
#version 310 es

layout(rgba16ui) uniform highp writeonly uimage2D arg_0;
void textureStore_ff23b3() {
  imageStore(arg_0, ivec2(1, 0), uvec4(1u));
}

void compute_main() {
  textureStore_ff23b3();
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  compute_main();
  return;
}
