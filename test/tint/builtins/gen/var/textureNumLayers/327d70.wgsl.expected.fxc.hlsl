RWByteAddressBuffer prevent_dce : register(u0);
RWTexture2DArray<uint4> arg_0 : register(u0, space1);

uint textureNumLayers_327d70() {
  uint3 tint_tmp;
  arg_0.GetDimensions(tint_tmp.x, tint_tmp.y, tint_tmp.z);
  uint res = tint_tmp.z;
  return res;
}

void fragment_main() {
  prevent_dce.Store(0u, asuint(textureNumLayers_327d70()));
  return;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store(0u, asuint(textureNumLayers_327d70()));
  return;
}
