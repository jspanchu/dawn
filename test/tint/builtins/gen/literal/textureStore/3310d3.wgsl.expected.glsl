#version 310 es
precision highp float;
precision highp int;

layout(rgba8_snorm) uniform highp writeonly image2DArray arg_0;
void textureStore_3310d3() {
  imageStore(arg_0, ivec3(ivec2(1), 1), vec4(1.0f));
}

void fragment_main() {
  textureStore_3310d3();
}

void main() {
  fragment_main();
  return;
}
#version 310 es

layout(rgba8_snorm) uniform highp writeonly image2DArray arg_0;
void textureStore_3310d3() {
  imageStore(arg_0, ivec3(ivec2(1), 1), vec4(1.0f));
}

void compute_main() {
  textureStore_3310d3();
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  compute_main();
  return;
}
