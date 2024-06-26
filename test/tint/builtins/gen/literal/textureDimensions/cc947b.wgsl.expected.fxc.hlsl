RWByteAddressBuffer prevent_dce : register(u0);
RWTexture3D<float4> arg_0 : register(u0, space1);

uint3 textureDimensions_cc947b() {
  uint3 tint_tmp;
  arg_0.GetDimensions(tint_tmp.x, tint_tmp.y, tint_tmp.z);
  uint3 res = tint_tmp;
  return res;
}

void fragment_main() {
  prevent_dce.Store3(0u, asuint(textureDimensions_cc947b()));
  return;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store3(0u, asuint(textureDimensions_cc947b()));
  return;
}
