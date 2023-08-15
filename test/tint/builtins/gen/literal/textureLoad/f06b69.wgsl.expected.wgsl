enable chromium_experimental_read_write_storage_texture;

@group(1) @binding(0) var arg_0 : texture_storage_1d<r32sint, read>;

fn textureLoad_f06b69() {
  var res : vec4<i32> = textureLoad(arg_0, 1i);
  prevent_dce = res;
}

@group(2) @binding(0) var<storage, read_write> prevent_dce : vec4<i32>;

@vertex
fn vertex_main() -> @builtin(position) vec4<f32> {
  textureLoad_f06b69();
  return vec4<f32>();
}

@fragment
fn fragment_main() {
  textureLoad_f06b69();
}

@compute @workgroup_size(1)
fn compute_main() {
  textureLoad_f06b69();
}
