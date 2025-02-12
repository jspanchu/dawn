#version 310 es
precision highp float;
precision highp int;

layout(rgba32i) uniform highp writeonly iimage2DArray arg_0;
void textureStore_f05928() {
  ivec2 arg_1 = ivec2(1);
  uint arg_2 = 1u;
  ivec4 arg_3 = ivec4(1);
  imageStore(arg_0, ivec3(arg_1, int(arg_2)), arg_3);
}

void fragment_main() {
  textureStore_f05928();
}

void main() {
  fragment_main();
  return;
}
#version 310 es

layout(rgba32i) uniform highp writeonly iimage2DArray arg_0;
void textureStore_f05928() {
  ivec2 arg_1 = ivec2(1);
  uint arg_2 = 1u;
  ivec4 arg_3 = ivec4(1);
  imageStore(arg_0, ivec3(arg_1, int(arg_2)), arg_3);
}

void compute_main() {
  textureStore_f05928();
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  compute_main();
  return;
}
