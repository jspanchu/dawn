#version 310 es
precision highp float;
precision highp int;

layout(r32f) uniform highp image2DArray arg_0;
void textureStore_312f27() {
  imageStore(arg_0, ivec3(ivec2(1), int(1u)), vec4(1.0f));
}

void fragment_main() {
  textureStore_312f27();
}

void main() {
  fragment_main();
  return;
}
#version 310 es

layout(r32f) uniform highp image2DArray arg_0;
void textureStore_312f27() {
  imageStore(arg_0, ivec3(ivec2(1), int(1u)), vec4(1.0f));
}

void compute_main() {
  textureStore_312f27();
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  compute_main();
  return;
}
