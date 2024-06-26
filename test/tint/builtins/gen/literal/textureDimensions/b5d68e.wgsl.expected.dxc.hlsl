RWByteAddressBuffer prevent_dce : register(u0);
RWTexture2DArray<int4> arg_0 : register(u0, space1);

uint2 textureDimensions_b5d68e() {
  uint3 tint_tmp;
  arg_0.GetDimensions(tint_tmp.x, tint_tmp.y, tint_tmp.z);
  uint2 res = tint_tmp.xy;
  return res;
}

void fragment_main() {
  prevent_dce.Store2(0u, asuint(textureDimensions_b5d68e()));
  return;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store2(0u, asuint(textureDimensions_b5d68e()));
  return;
}
