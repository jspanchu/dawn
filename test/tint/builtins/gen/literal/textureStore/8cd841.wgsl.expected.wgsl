@group(1) @binding(0) var arg_0 : texture_storage_2d_array<r32uint, read_write>;

fn textureStore_8cd841() {
  textureStore(arg_0, vec2<i32>(1i), 1u, vec4<u32>(1u));
}

@vertex
fn vertex_main() -> @builtin(position) vec4<f32> {
  textureStore_8cd841();
  return vec4<f32>();
}

@fragment
fn fragment_main() {
  textureStore_8cd841();
}

@compute @workgroup_size(1)
fn compute_main() {
  textureStore_8cd841();
}
