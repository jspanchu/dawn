#version 310 es
precision highp float;
precision highp int;

layout(r32i) uniform highp writeonly iimage2D arg_0;
void textureStore_804942() {
  imageStore(arg_0, ivec2(1), ivec4(1));
}

void fragment_main() {
  textureStore_804942();
}

void main() {
  fragment_main();
  return;
}
#version 310 es

layout(r32i) uniform highp writeonly iimage2D arg_0;
void textureStore_804942() {
  imageStore(arg_0, ivec2(1), ivec4(1));
}

void compute_main() {
  textureStore_804942();
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  compute_main();
  return;
}
