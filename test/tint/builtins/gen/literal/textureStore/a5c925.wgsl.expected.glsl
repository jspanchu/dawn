#version 310 es
precision highp float;
precision highp int;

layout(rgba8i) uniform highp writeonly iimage3D arg_0;
void textureStore_a5c925() {
  imageStore(arg_0, ivec3(uvec3(1u)), ivec4(1));
}

void fragment_main() {
  textureStore_a5c925();
}

void main() {
  fragment_main();
  return;
}
#version 310 es

layout(rgba8i) uniform highp writeonly iimage3D arg_0;
void textureStore_a5c925() {
  imageStore(arg_0, ivec3(uvec3(1u)), ivec4(1));
}

void compute_main() {
  textureStore_a5c925();
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  compute_main();
  return;
}
