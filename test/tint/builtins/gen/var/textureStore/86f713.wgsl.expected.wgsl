@group(1) @binding(0) var arg_0 : texture_storage_3d<rgba16sint, read_write>;

fn textureStore_86f713() {
  var arg_1 = vec3<i32>(1i);
  var arg_2 = vec4<i32>(1i);
  textureStore(arg_0, arg_1, arg_2);
}

@vertex
fn vertex_main() -> @builtin(position) vec4<f32> {
  textureStore_86f713();
  return vec4<f32>();
}

@fragment
fn fragment_main() {
  textureStore_86f713();
}

@compute @workgroup_size(1)
fn compute_main() {
  textureStore_86f713();
}
