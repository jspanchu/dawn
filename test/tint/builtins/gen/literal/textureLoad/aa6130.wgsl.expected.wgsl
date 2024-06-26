@group(0) @binding(0) var<storage, read_write> prevent_dce : vec4<i32>;

@group(1) @binding(0) var arg_0 : texture_storage_2d<rg32sint, read_write>;

fn textureLoad_aa6130() -> vec4<i32> {
  var res : vec4<i32> = textureLoad(arg_0, vec2<i32>(1i));
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = textureLoad_aa6130();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = textureLoad_aa6130();
}
