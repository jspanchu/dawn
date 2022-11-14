#version 310 es
#extension GL_AMD_gpu_shader_half_float : require

struct frexp_result_f16 {
  float16_t fract;
  int exp;
};

frexp_result_f16 tint_frexp(float16_t param_0) {
  frexp_result_f16 result;
  result.fract = frexp(param_0, result.exp);
  return result;
}


void frexp_5257dd() {
  float16_t arg_0 = 1.0hf;
  frexp_result_f16 res = tint_frexp(arg_0);
}

vec4 vertex_main() {
  frexp_5257dd();
  return vec4(0.0f);
}

void main() {
  gl_PointSize = 1.0;
  vec4 inner_result = vertex_main();
  gl_Position = inner_result;
  gl_Position.y = -(gl_Position.y);
  gl_Position.z = ((2.0f * gl_Position.z) - gl_Position.w);
  return;
}
#version 310 es
#extension GL_AMD_gpu_shader_half_float : require
precision mediump float;

struct frexp_result_f16 {
  float16_t fract;
  int exp;
};

frexp_result_f16 tint_frexp(float16_t param_0) {
  frexp_result_f16 result;
  result.fract = frexp(param_0, result.exp);
  return result;
}


void frexp_5257dd() {
  float16_t arg_0 = 1.0hf;
  frexp_result_f16 res = tint_frexp(arg_0);
}

void fragment_main() {
  frexp_5257dd();
}

void main() {
  fragment_main();
  return;
}
#version 310 es
#extension GL_AMD_gpu_shader_half_float : require

struct frexp_result_f16 {
  float16_t fract;
  int exp;
};

frexp_result_f16 tint_frexp(float16_t param_0) {
  frexp_result_f16 result;
  result.fract = frexp(param_0, result.exp);
  return result;
}


void frexp_5257dd() {
  float16_t arg_0 = 1.0hf;
  frexp_result_f16 res = tint_frexp(arg_0);
}

void compute_main() {
  frexp_5257dd();
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  compute_main();
  return;
}
