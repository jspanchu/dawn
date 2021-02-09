// Copyright 2020 The Tint Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memory>

#include "gtest/gtest.h"
#include "src/ast/call_expression.h"
#include "src/ast/call_statement.h"
#include "src/ast/float_literal.h"
#include "src/ast/identifier_expression.h"
#include "src/ast/intrinsic_texture_helper_test.h"
#include "src/ast/member_accessor_expression.h"
#include "src/ast/scalar_constructor_expression.h"
#include "src/ast/sint_literal.h"
#include "src/ast/stage_decoration.h"
#include "src/ast/struct.h"
#include "src/ast/struct_block_decoration.h"
#include "src/ast/struct_member.h"
#include "src/ast/type_constructor_expression.h"
#include "src/ast/uint_literal.h"
#include "src/ast/variable.h"
#include "src/type/array_type.h"
#include "src/type/bool_type.h"
#include "src/type/depth_texture_type.h"
#include "src/type/f32_type.h"
#include "src/type/i32_type.h"
#include "src/type/matrix_type.h"
#include "src/type/multisampled_texture_type.h"
#include "src/type/pointer_type.h"
#include "src/type/sampled_texture_type.h"
#include "src/type/sampler_type.h"
#include "src/type/struct_type.h"
#include "src/type/u32_type.h"
#include "src/type/vector_type.h"
#include "src/type/void_type.h"
#include "src/type_determiner.h"
#include "src/writer/spirv/builder.h"
#include "src/writer/spirv/spv_dump.h"
#include "src/writer/spirv/test_helper.h"

namespace tint {
namespace writer {
namespace spirv {
namespace {

using IntrinsicBuilderTest = TestHelper;

template <typename T>
using IntrinsicBuilderTestWithParam = TestParamHelper<T>;

struct IntrinsicData {
  std::string name;
  std::string op;
};
inline std::ostream& operator<<(std::ostream& out, IntrinsicData data) {
  out << data.name;
  return out;
}

using IntrinsicBoolTest = IntrinsicBuilderTestWithParam<IntrinsicData>;
TEST_P(IntrinsicBoolTest, Call_Bool) {
  auto param = GetParam();

  auto* var = Global("v", ast::StorageClass::kPrivate, ty.vec3<bool>());

  auto* expr = Call(param.name, "v");
  WrapInFunction(expr);

  spirv::Builder& b = Build();

  b.push_function(Function{});
  ASSERT_TRUE(b.GenerateGlobalVariable(var)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 6u) << b.error();
  EXPECT_EQ(DumpInstructions(b.types()), R"(%4 = OpTypeBool
%3 = OpTypeVector %4 3
%2 = OpTypePointer Private %3
%5 = OpConstantNull %3
%1 = OpVariable %2 Private %5
)");
  EXPECT_EQ(DumpInstructions(b.functions()[0].instructions()),
            R"(%7 = OpLoad %3 %1
%6 = )" + param.op +
                " %4 %7\n");
}
INSTANTIATE_TEST_SUITE_P(IntrinsicBuilderTest,
                         IntrinsicBoolTest,
                         testing::Values(IntrinsicData{"any", "OpAny"},
                                         IntrinsicData{"all", "OpAll"}));

using IntrinsicFloatTest = IntrinsicBuilderTestWithParam<IntrinsicData>;
TEST_P(IntrinsicFloatTest, Call_Float_Scalar) {
  auto param = GetParam();

  auto* var = Global("v", ast::StorageClass::kPrivate, ty.f32());

  auto* expr = Call(param.name, "v");
  WrapInFunction(expr);

  spirv::Builder& b = Build();

  b.push_function(Function{});
  ASSERT_TRUE(b.GenerateGlobalVariable(var)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpInstructions(b.types()), R"(%3 = OpTypeFloat 32
%2 = OpTypePointer Private %3
%4 = OpConstantNull %3
%1 = OpVariable %2 Private %4
%6 = OpTypeBool
)");
  EXPECT_EQ(DumpInstructions(b.functions()[0].instructions()),
            R"(%7 = OpLoad %3 %1
%5 = )" + param.op +
                " %6 %7\n");
}

TEST_P(IntrinsicFloatTest, Call_Float_Vector) {
  auto param = GetParam();

  auto* var = Global("v", ast::StorageClass::kPrivate, ty.vec3<f32>());

  auto* expr = Call(param.name, "v");
  WrapInFunction(expr);

  spirv::Builder& b = Build();

  b.push_function(Function{});
  ASSERT_TRUE(b.GenerateGlobalVariable(var)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 6u) << b.error();
  EXPECT_EQ(DumpInstructions(b.types()), R"(%4 = OpTypeFloat 32
%3 = OpTypeVector %4 3
%2 = OpTypePointer Private %3
%5 = OpConstantNull %3
%1 = OpVariable %2 Private %5
%8 = OpTypeBool
%7 = OpTypeVector %8 3
)");
  EXPECT_EQ(DumpInstructions(b.functions()[0].instructions()),
            R"(%9 = OpLoad %3 %1
%6 = )" + param.op +
                " %7 %9\n");
}
INSTANTIATE_TEST_SUITE_P(IntrinsicBuilderTest,
                         IntrinsicFloatTest,
                         testing::Values(IntrinsicData{"isNan", "OpIsNan"},
                                         IntrinsicData{"isInf", "OpIsInf"}));

using IntrinsicIntTest = IntrinsicBuilderTestWithParam<IntrinsicData>;
TEST_P(IntrinsicIntTest, Call_SInt_Scalar) {
  auto param = GetParam();

  auto* var = Global("v", ast::StorageClass::kPrivate, ty.i32());

  auto* expr = Call(param.name, "v");
  WrapInFunction(expr);

  spirv::Builder& b = Build();

  b.push_function(Function{});
  ASSERT_TRUE(b.GenerateGlobalVariable(var)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpInstructions(b.types()), R"(%3 = OpTypeInt 32 1
%2 = OpTypePointer Private %3
%4 = OpConstantNull %3
%1 = OpVariable %2 Private %4
)");
  EXPECT_EQ(DumpInstructions(b.functions()[0].instructions()),
            R"(%6 = OpLoad %3 %1
%5 = )" + param.op +
                " %3 %6\n");
}

TEST_P(IntrinsicIntTest, Call_SInt_Vector) {
  auto param = GetParam();

  auto* var = Global("v", ast::StorageClass::kPrivate, ty.vec3<i32>());

  auto* expr = Call(param.name, "v");
  WrapInFunction(expr);

  spirv::Builder& b = Build();

  b.push_function(Function{});
  ASSERT_TRUE(b.GenerateGlobalVariable(var)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 6u) << b.error();
  EXPECT_EQ(DumpInstructions(b.types()), R"(%4 = OpTypeInt 32 1
%3 = OpTypeVector %4 3
%2 = OpTypePointer Private %3
%5 = OpConstantNull %3
%1 = OpVariable %2 Private %5
)");
  EXPECT_EQ(DumpInstructions(b.functions()[0].instructions()),
            R"(%7 = OpLoad %3 %1
%6 = )" + param.op +
                " %3 %7\n");
}

TEST_P(IntrinsicIntTest, Call_UInt_Scalar) {
  auto param = GetParam();

  auto* var = Global("v", ast::StorageClass::kPrivate, ty.u32());

  auto* expr = Call(param.name, "v");
  WrapInFunction(expr);

  spirv::Builder& b = Build();

  b.push_function(Function{});
  ASSERT_TRUE(b.GenerateGlobalVariable(var)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpInstructions(b.types()), R"(%3 = OpTypeInt 32 0
%2 = OpTypePointer Private %3
%4 = OpConstantNull %3
%1 = OpVariable %2 Private %4
)");
  EXPECT_EQ(DumpInstructions(b.functions()[0].instructions()),
            R"(%6 = OpLoad %3 %1
%5 = )" + param.op +
                " %3 %6\n");
}

TEST_P(IntrinsicIntTest, Call_UInt_Vector) {
  auto param = GetParam();

  auto* var = Global("v", ast::StorageClass::kPrivate, ty.vec3<u32>());

  auto* expr = Call(param.name, "v");
  WrapInFunction(expr);

  spirv::Builder& b = Build();

  b.push_function(Function{});
  ASSERT_TRUE(b.GenerateGlobalVariable(var)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 6u) << b.error();
  EXPECT_EQ(DumpInstructions(b.types()), R"(%4 = OpTypeInt 32 0
%3 = OpTypeVector %4 3
%2 = OpTypePointer Private %3
%5 = OpConstantNull %3
%1 = OpVariable %2 Private %5
)");
  EXPECT_EQ(DumpInstructions(b.functions()[0].instructions()),
            R"(%7 = OpLoad %3 %1
%6 = )" + param.op +
                " %3 %7\n");
}
INSTANTIATE_TEST_SUITE_P(
    IntrinsicBuilderTest,
    IntrinsicIntTest,
    testing::Values(IntrinsicData{"countOneBits", "OpBitCount"},
                    IntrinsicData{"reverseBits", "OpBitReverse"}));

TEST_F(IntrinsicBuilderTest, Call_Dot) {
  auto* var = Global("v", ast::StorageClass::kPrivate, ty.vec3<f32>());

  auto* expr = Call("dot", "v", "v");
  WrapInFunction(expr);

  spirv::Builder& b = Build();

  b.push_function(Function{});
  ASSERT_TRUE(b.GenerateGlobalVariable(var)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 6u) << b.error();
  EXPECT_EQ(DumpInstructions(b.types()), R"(%4 = OpTypeFloat 32
%3 = OpTypeVector %4 3
%2 = OpTypePointer Private %3
%5 = OpConstantNull %3
%1 = OpVariable %2 Private %5
)");
  EXPECT_EQ(DumpInstructions(b.functions()[0].instructions()),
            R"(%7 = OpLoad %3 %1
%8 = OpLoad %3 %1
%6 = OpDot %4 %7 %8
)");
}

using IntrinsicDeriveTest = IntrinsicBuilderTestWithParam<IntrinsicData>;
TEST_P(IntrinsicDeriveTest, Call_Derivative_Scalar) {
  auto param = GetParam();

  auto* var = Global("v", ast::StorageClass::kPrivate, ty.f32());

  auto* expr = Call(param.name, "v");
  WrapInFunction(expr);

  spirv::Builder& b = Build();

  b.push_function(Function{});
  ASSERT_TRUE(b.GenerateGlobalVariable(var)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpInstructions(b.types()), R"(%3 = OpTypeFloat 32
%2 = OpTypePointer Private %3
%4 = OpConstantNull %3
%1 = OpVariable %2 Private %4
)");
  EXPECT_EQ(DumpInstructions(b.functions()[0].instructions()),
            R"(%6 = OpLoad %3 %1
%5 = )" + param.op +
                " %3 %6\n");
}

TEST_P(IntrinsicDeriveTest, Call_Derivative_Vector) {
  auto param = GetParam();

  auto* var = Global("v", ast::StorageClass::kPrivate, ty.vec3<f32>());

  auto* expr = Call(param.name, "v");
  WrapInFunction(expr);

  spirv::Builder& b = Build();

  b.push_function(Function{});
  ASSERT_TRUE(b.GenerateGlobalVariable(var)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 6u) << b.error();

  if (param.name != "dpdx" && param.name != "dpdy" && param.name != "fwidth") {
    EXPECT_EQ(DumpInstructions(b.capabilities()),
              R"(OpCapability DerivativeControl
)");
  }

  EXPECT_EQ(DumpInstructions(b.types()), R"(%4 = OpTypeFloat 32
%3 = OpTypeVector %4 3
%2 = OpTypePointer Private %3
%5 = OpConstantNull %3
%1 = OpVariable %2 Private %5
)");
  EXPECT_EQ(DumpInstructions(b.functions()[0].instructions()),
            R"(%7 = OpLoad %3 %1
%6 = )" + param.op +
                " %3 %7\n");
}
INSTANTIATE_TEST_SUITE_P(
    IntrinsicBuilderTest,
    IntrinsicDeriveTest,
    testing::Values(IntrinsicData{"dpdx", "OpDPdx"},
                    IntrinsicData{"dpdxFine", "OpDPdxFine"},
                    IntrinsicData{"dpdxCoarse", "OpDPdxCoarse"},
                    IntrinsicData{"dpdy", "OpDPdy"},
                    IntrinsicData{"dpdyFine", "OpDPdyFine"},
                    IntrinsicData{"dpdyCoarse", "OpDPdyCoarse"},
                    IntrinsicData{"fwidth", "OpFwidth"},
                    IntrinsicData{"fwidthFine", "OpFwidthFine"},
                    IntrinsicData{"fwidthCoarse", "OpFwidthCoarse"}));

TEST_F(IntrinsicBuilderTest, Call_Select) {
  auto* v3 = Global("v3", ast::StorageClass::kPrivate, ty.vec3<f32>());

  auto* bool_v3 =
      Global("bool_v3", ast::StorageClass::kPrivate, ty.vec3<bool>());

  auto* expr = Call("select", "v3", "v3", "bool_v3");
  WrapInFunction(expr);

  spirv::Builder& b = Build();

  b.push_function(Function{});
  ASSERT_TRUE(b.GenerateGlobalVariable(v3)) << b.error();
  ASSERT_TRUE(b.GenerateGlobalVariable(bool_v3)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 11u) << b.error();

  EXPECT_EQ(DumpInstructions(b.types()), R"(%4 = OpTypeFloat 32
%3 = OpTypeVector %4 3
%2 = OpTypePointer Private %3
%5 = OpConstantNull %3
%1 = OpVariable %2 Private %5
%9 = OpTypeBool
%8 = OpTypeVector %9 3
%7 = OpTypePointer Private %8
%10 = OpConstantNull %8
%6 = OpVariable %7 Private %10
)");
  EXPECT_EQ(DumpInstructions(b.functions()[0].instructions()),
            R"(%12 = OpLoad %3 %1
%13 = OpLoad %3 %1
%14 = OpLoad %8 %6
%11 = OpSelect %3 %12 %13 %14
)");
}

// This tests that we do not push OpTypeSampledImage and float_0 type twice.
TEST_F(IntrinsicBuilderTest, Call_TextureSampleCompare_Twice) {
  type::Sampler s(type::SamplerKind::kComparisonSampler);
  type::DepthTexture t(type::TextureDimension::k2d);

  auto* tex = Global("texture", ast::StorageClass::kNone, &t);

  auto* sampler = Global("sampler", ast::StorageClass::kNone, &s);

  auto* expr1 = Call("textureSampleCompare", "texture", "sampler",
                     vec2<f32>(1.0f, 2.0f), 2.0f);
  auto* expr2 = Call("textureSampleCompare", "texture", "sampler",
                     vec2<f32>(1.0f, 2.0f), 2.0f);

  WrapInFunction(expr1);
  WrapInFunction(expr2);

  spirv::Builder& b = Build();

  b.push_function(Function{});

  ASSERT_TRUE(b.GenerateGlobalVariable(tex)) << b.error();
  ASSERT_TRUE(b.GenerateGlobalVariable(sampler)) << b.error();

  EXPECT_EQ(b.GenerateExpression(expr1), 8u) << b.error();
  EXPECT_EQ(b.GenerateExpression(expr2), 18u) << b.error();

  EXPECT_EQ(DumpInstructions(b.types()), R"(%4 = OpTypeFloat 32
%3 = OpTypeImage %4 2D 1 0 0 1 Unknown
%2 = OpTypePointer Private %3
%1 = OpVariable %2 Private
%7 = OpTypeSampler
%6 = OpTypePointer Private %7
%5 = OpVariable %6 Private
%11 = OpTypeSampledImage %3
%13 = OpTypeVector %4 2
%14 = OpConstant %4 1
%15 = OpConstant %4 2
%16 = OpConstantComposite %13 %14 %15
%17 = OpConstant %4 0
)");

  EXPECT_EQ(DumpInstructions(b.functions()[0].instructions()),
            R"(%9 = OpLoad %7 %5
%10 = OpLoad %3 %1
%12 = OpSampledImage %11 %10 %9
%8 = OpImageSampleDrefExplicitLod %4 %12 %16 %15 Lod %17
%19 = OpLoad %7 %5
%20 = OpLoad %3 %1
%21 = OpSampledImage %11 %20 %19
%18 = OpImageSampleDrefExplicitLod %4 %21 %16 %15 Lod %17
)");
}

TEST_F(IntrinsicBuilderTest, Call_GLSLMethod_WithLoad) {
  auto* var = Global("ident", ast::StorageClass::kPrivate, ty.f32());

  auto* expr = Call("round", "ident");
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateGlobalVariable(var)) << b.error();
  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 9u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%10 = OpExtInstImport "GLSL.std.450"
OpName %1 "ident"
OpName %7 "a_func"
%3 = OpTypeFloat 32
%2 = OpTypePointer Private %3
%4 = OpConstantNull %3
%1 = OpVariable %2 Private %4
%6 = OpTypeVoid
%5 = OpTypeFunction %6
%7 = OpFunction %6 None %5
%8 = OpLabel
%11 = OpLoad %3 %1
%9 = OpExtInst %3 %10 Round %11
OpReturn
OpFunctionEnd
)");
}

using Intrinsic_Builtin_SingleParam_Float_Test =
    IntrinsicBuilderTestWithParam<IntrinsicData>;
TEST_P(Intrinsic_Builtin_SingleParam_Float_Test, Call_Scalar) {
  auto param = GetParam();

  auto* expr = Call(param.name, 1.0f);
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%7 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%6 = OpTypeFloat 32
%8 = OpConstant %6 1
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %7 )" + param.op +
                                R"( %8
OpReturn
OpFunctionEnd
)");
}

TEST_P(Intrinsic_Builtin_SingleParam_Float_Test, Call_Vector) {
  auto param = GetParam();

  auto* expr = Call(param.name, vec2<f32>(1.0f, 1.0f));
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%8 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%7 = OpTypeFloat 32
%6 = OpTypeVector %7 2
%9 = OpConstant %7 1
%10 = OpConstantComposite %6 %9 %9
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %8 )" + param.op +
                                R"( %10
OpReturn
OpFunctionEnd
)");
}
INSTANTIATE_TEST_SUITE_P(IntrinsicBuilderTest,
                         Intrinsic_Builtin_SingleParam_Float_Test,
                         testing::Values(IntrinsicData{"abs", "FAbs"},
                                         IntrinsicData{"acos", "Acos"},
                                         IntrinsicData{"asin", "Asin"},
                                         IntrinsicData{"atan", "Atan"},
                                         IntrinsicData{"ceil", "Ceil"},
                                         IntrinsicData{"cos", "Cos"},
                                         IntrinsicData{"cosh", "Cosh"},
                                         IntrinsicData{"exp", "Exp"},
                                         IntrinsicData{"exp2", "Exp2"},
                                         IntrinsicData{"floor", "Floor"},
                                         IntrinsicData{"fract", "Fract"},
                                         IntrinsicData{"inverseSqrt",
                                                       "InverseSqrt"},
                                         IntrinsicData{"log", "Log"},
                                         IntrinsicData{"log2", "Log2"},
                                         IntrinsicData{"round", "Round"},
                                         IntrinsicData{"sign", "FSign"},
                                         IntrinsicData{"sin", "Sin"},
                                         IntrinsicData{"sinh", "Sinh"},
                                         IntrinsicData{"sqrt", "Sqrt"},
                                         IntrinsicData{"tan", "Tan"},
                                         IntrinsicData{"tanh", "Tanh"},
                                         IntrinsicData{"trunc", "Trunc"}));

TEST_F(IntrinsicBuilderTest, Call_Length_Scalar) {
  auto* expr = Call("length", 1.0f);

  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%7 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%6 = OpTypeFloat 32
%8 = OpConstant %6 1
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %7 Length %8
OpReturn
OpFunctionEnd
)");
}

TEST_F(IntrinsicBuilderTest, Call_Length_Vector) {
  auto* expr = Call("length", vec2<f32>(1.0f, 1.0f));
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%7 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%6 = OpTypeFloat 32
%8 = OpTypeVector %6 2
%9 = OpConstant %6 1
%10 = OpConstantComposite %8 %9 %9
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %7 Length %10
OpReturn
OpFunctionEnd
)");
}

TEST_F(IntrinsicBuilderTest, Call_Normalize) {
  auto* expr = Call("normalize", vec2<f32>(1.0f, 1.0f));
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%8 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%7 = OpTypeFloat 32
%6 = OpTypeVector %7 2
%9 = OpConstant %7 1
%10 = OpConstantComposite %6 %9 %9
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %8 Normalize %10
OpReturn
OpFunctionEnd
)");
}

using Intrinsic_Builtin_DualParam_Float_Test =
    IntrinsicBuilderTestWithParam<IntrinsicData>;
TEST_P(Intrinsic_Builtin_DualParam_Float_Test, Call_Scalar) {
  auto param = GetParam();

  auto* expr = Call(param.name, 1.0f, 1.0f);

  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%7 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%6 = OpTypeFloat 32
%8 = OpConstant %6 1
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %7 )" + param.op +
                                R"( %8 %8
OpReturn
OpFunctionEnd
)");
}

TEST_P(Intrinsic_Builtin_DualParam_Float_Test, Call_Vector) {
  auto param = GetParam();

  auto* expr = Call(param.name, vec2<f32>(1.0f, 1.0f), vec2<f32>(1.0f, 1.0f));

  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%8 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%7 = OpTypeFloat 32
%6 = OpTypeVector %7 2
%9 = OpConstant %7 1
%10 = OpConstantComposite %6 %9 %9
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %8 )" + param.op +
                                R"( %10 %10
OpReturn
OpFunctionEnd
)");
}
INSTANTIATE_TEST_SUITE_P(IntrinsicBuilderTest,
                         Intrinsic_Builtin_DualParam_Float_Test,
                         testing::Values(IntrinsicData{"atan2", "Atan2"},
                                         IntrinsicData{"max", "NMax"},
                                         IntrinsicData{"min", "NMin"},
                                         IntrinsicData{"pow", "Pow"},
                                         IntrinsicData{"reflect", "Reflect"},
                                         IntrinsicData{"step", "Step"}));

TEST_F(IntrinsicBuilderTest, Call_Distance_Scalar) {
  auto* expr = Call("distance", 1.0f, 1.0f);

  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%7 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%6 = OpTypeFloat 32
%8 = OpConstant %6 1
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %7 Distance %8 %8
OpReturn
OpFunctionEnd
)");
}

TEST_F(IntrinsicBuilderTest, Call_Distance_Vector) {
  auto* expr = Call("distance", vec2<f32>(1.0f, 1.0f), vec2<f32>(1.0f, 1.0f));

  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%7 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%6 = OpTypeFloat 32
%8 = OpTypeVector %6 2
%9 = OpConstant %6 1
%10 = OpConstantComposite %8 %9 %9
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %7 Distance %10 %10
OpReturn
OpFunctionEnd
)");
}

TEST_F(IntrinsicBuilderTest, Call_Cross) {
  auto* expr =
      Call("cross", vec3<f32>(1.0f, 1.0f, 1.0f), vec3<f32>(1.0f, 1.0f, 1.0f));

  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%8 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%7 = OpTypeFloat 32
%6 = OpTypeVector %7 3
%9 = OpConstant %7 1
%10 = OpConstantComposite %6 %9 %9 %9
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %8 Cross %10 %10
OpReturn
OpFunctionEnd
)");
}

using Intrinsic_Builtin_ThreeParam_Float_Test =
    IntrinsicBuilderTestWithParam<IntrinsicData>;
TEST_P(Intrinsic_Builtin_ThreeParam_Float_Test, Call_Scalar) {
  auto param = GetParam();

  auto* expr = Call(param.name, 1.0f, 1.0f, 1.0f);
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%7 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%6 = OpTypeFloat 32
%8 = OpConstant %6 1
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %7 )" + param.op +
                                R"( %8 %8 %8
OpReturn
OpFunctionEnd
)");
}

TEST_P(Intrinsic_Builtin_ThreeParam_Float_Test, Call_Vector) {
  auto param = GetParam();

  auto* expr = Call(param.name, vec2<f32>(1.0f, 1.0f), vec2<f32>(1.0f, 1.0f),
                    vec2<f32>(1.0f, 1.0f));

  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%8 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%7 = OpTypeFloat 32
%6 = OpTypeVector %7 2
%9 = OpConstant %7 1
%10 = OpConstantComposite %6 %9 %9
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %8 )" + param.op +
                                R"( %10 %10 %10
OpReturn
OpFunctionEnd
)");
}
INSTANTIATE_TEST_SUITE_P(
    IntrinsicBuilderTest,
    Intrinsic_Builtin_ThreeParam_Float_Test,
    testing::Values(IntrinsicData{"clamp", "NClamp"},
                    IntrinsicData{"faceForward", "FaceForward"},
                    IntrinsicData{"fma", "Fma"},
                    IntrinsicData{"mix", "FMix"},

                    IntrinsicData{"smoothStep", "SmoothStep"}));

using Intrinsic_Builtin_SingleParam_Sint_Test =
    IntrinsicBuilderTestWithParam<IntrinsicData>;
TEST_P(Intrinsic_Builtin_SingleParam_Sint_Test, Call_Scalar) {
  auto param = GetParam();

  auto* expr = Call(param.name, 1);
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%7 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%6 = OpTypeInt 32 1
%8 = OpConstant %6 1
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %7 )" + param.op +
                                R"( %8
OpReturn
OpFunctionEnd
)");
}

TEST_P(Intrinsic_Builtin_SingleParam_Sint_Test, Call_Vector) {
  auto param = GetParam();

  auto* expr = Call(param.name, vec2<i32>(1, 1));
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%8 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%7 = OpTypeInt 32 1
%6 = OpTypeVector %7 2
%9 = OpConstant %7 1
%10 = OpConstantComposite %6 %9 %9
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %8 )" + param.op +
                                R"( %10
OpReturn
OpFunctionEnd
)");
}
INSTANTIATE_TEST_SUITE_P(IntrinsicBuilderTest,
                         Intrinsic_Builtin_SingleParam_Sint_Test,
                         testing::Values(IntrinsicData{"abs", "SAbs"}));

using Intrinsic_Builtin_SingleParam_Uint_Test =
    IntrinsicBuilderTestWithParam<IntrinsicData>;
TEST_P(Intrinsic_Builtin_SingleParam_Uint_Test, Call_Scalar) {
  auto param = GetParam();

  auto* expr = Call(param.name, 1u);
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%7 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%6 = OpTypeInt 32 0
%8 = OpConstant %6 1
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %7 )" + param.op +
                                R"( %8
OpReturn
OpFunctionEnd
)");
}

TEST_P(Intrinsic_Builtin_SingleParam_Uint_Test, Call_Vector) {
  auto param = GetParam();

  auto* expr = Call(param.name, vec2<u32>(1u, 1u));
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%8 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%7 = OpTypeInt 32 0
%6 = OpTypeVector %7 2
%9 = OpConstant %7 1
%10 = OpConstantComposite %6 %9 %9
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %8 )" + param.op +
                                R"( %10
OpReturn
OpFunctionEnd
)");
}
INSTANTIATE_TEST_SUITE_P(IntrinsicBuilderTest,
                         Intrinsic_Builtin_SingleParam_Uint_Test,
                         testing::Values(IntrinsicData{"abs", "SAbs"}));

using Intrinsic_Builtin_DualParam_SInt_Test =
    IntrinsicBuilderTestWithParam<IntrinsicData>;
TEST_P(Intrinsic_Builtin_DualParam_SInt_Test, Call_Scalar) {
  auto param = GetParam();

  auto* expr = Call(param.name, 1, 1);
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%7 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%6 = OpTypeInt 32 1
%8 = OpConstant %6 1
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %7 )" + param.op +
                                R"( %8 %8
OpReturn
OpFunctionEnd
)");
}

TEST_P(Intrinsic_Builtin_DualParam_SInt_Test, Call_Vector) {
  auto param = GetParam();

  auto* expr = Call(param.name, vec2<i32>(1, 1), vec2<i32>(1, 1));
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%8 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%7 = OpTypeInt 32 1
%6 = OpTypeVector %7 2
%9 = OpConstant %7 1
%10 = OpConstantComposite %6 %9 %9
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %8 )" + param.op +
                                R"( %10 %10
OpReturn
OpFunctionEnd
)");
}
INSTANTIATE_TEST_SUITE_P(IntrinsicBuilderTest,
                         Intrinsic_Builtin_DualParam_SInt_Test,
                         testing::Values(IntrinsicData{"max", "SMax"},
                                         IntrinsicData{"min", "SMin"}));

using Intrinsic_Builtin_DualParam_UInt_Test =
    IntrinsicBuilderTestWithParam<IntrinsicData>;
TEST_P(Intrinsic_Builtin_DualParam_UInt_Test, Call_Scalar) {
  auto param = GetParam();

  auto* expr = Call(param.name, 1u, 1u);
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%7 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%6 = OpTypeInt 32 0
%8 = OpConstant %6 1
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %7 )" + param.op +
                                R"( %8 %8
OpReturn
OpFunctionEnd
)");
}

TEST_P(Intrinsic_Builtin_DualParam_UInt_Test, Call_Vector) {
  auto param = GetParam();

  auto* expr = Call(param.name, vec2<u32>(1u, 1u), vec2<u32>(1u, 1u));
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%8 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%7 = OpTypeInt 32 0
%6 = OpTypeVector %7 2
%9 = OpConstant %7 1
%10 = OpConstantComposite %6 %9 %9
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %8 )" + param.op +
                                R"( %10 %10
OpReturn
OpFunctionEnd
)");
}
INSTANTIATE_TEST_SUITE_P(IntrinsicBuilderTest,
                         Intrinsic_Builtin_DualParam_UInt_Test,
                         testing::Values(IntrinsicData{"max", "UMax"},
                                         IntrinsicData{"min", "UMin"}));

using Intrinsic_Builtin_ThreeParam_Sint_Test =
    IntrinsicBuilderTestWithParam<IntrinsicData>;
TEST_P(Intrinsic_Builtin_ThreeParam_Sint_Test, Call_Scalar) {
  auto param = GetParam();

  auto* expr = Call(param.name, 1, 1, 1);
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%7 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%6 = OpTypeInt 32 1
%8 = OpConstant %6 1
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %7 )" + param.op +
                                R"( %8 %8 %8
OpReturn
OpFunctionEnd
)");
}

TEST_P(Intrinsic_Builtin_ThreeParam_Sint_Test, Call_Vector) {
  auto param = GetParam();

  auto* expr =
      Call(param.name, vec2<i32>(1, 1), vec2<i32>(1, 1), vec2<i32>(1, 1));

  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%8 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%7 = OpTypeInt 32 1
%6 = OpTypeVector %7 2
%9 = OpConstant %7 1
%10 = OpConstantComposite %6 %9 %9
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %8 )" + param.op +
                                R"( %10 %10 %10
OpReturn
OpFunctionEnd
)");
}
INSTANTIATE_TEST_SUITE_P(IntrinsicBuilderTest,
                         Intrinsic_Builtin_ThreeParam_Sint_Test,
                         testing::Values(IntrinsicData{"clamp", "SClamp"}));

using Intrinsic_Builtin_ThreeParam_Uint_Test =
    IntrinsicBuilderTestWithParam<IntrinsicData>;
TEST_P(Intrinsic_Builtin_ThreeParam_Uint_Test, Call_Scalar) {
  auto param = GetParam();

  auto* expr = Call(param.name, 1u, 1u, 1u);
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%7 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%6 = OpTypeInt 32 0
%8 = OpConstant %6 1
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %7 )" + param.op +
                                R"( %8 %8 %8
OpReturn
OpFunctionEnd
)");
}

TEST_P(Intrinsic_Builtin_ThreeParam_Uint_Test, Call_Vector) {
  auto param = GetParam();

  auto* expr =
      Call(param.name, vec2<u32>(1u, 1u), vec2<u32>(1u, 1u), vec2<u32>(1u, 1u));

  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  EXPECT_EQ(b.GenerateCallExpression(expr), 5u) << b.error();
  EXPECT_EQ(DumpBuilder(b), R"(%8 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%7 = OpTypeInt 32 0
%6 = OpTypeVector %7 2
%9 = OpConstant %7 1
%10 = OpConstantComposite %6 %9 %9
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpExtInst %6 %8 )" + param.op +
                                R"( %10 %10 %10
OpReturn
OpFunctionEnd
)");
}
INSTANTIATE_TEST_SUITE_P(IntrinsicBuilderTest,
                         Intrinsic_Builtin_ThreeParam_Uint_Test,
                         testing::Values(IntrinsicData{"clamp", "UClamp"}));

TEST_F(IntrinsicBuilderTest, Call_Modf) {
  auto* out = Var("out", ast::StorageClass::kFunction, ty.vec2<f32>());
  auto* expr = Call("modf", vec2<f32>(1.0f, 2.0f), "out");
  Func("a_func", ast::VariableList{}, ty.void_(),
       ast::StatementList{
           create<ast::VariableDeclStatement>(out),
           create<ast::CallStatement>(expr),
       },
       ast::FunctionDecorationList{
           create<ast::StageDecoration>(ast::PipelineStage::kVertex),
       });

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.Build()) << b.error();
  auto got = DumpBuilder(b);
  auto* expect = R"(OpCapability Shader
%11 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Vertex %3 "a_func"
OpName %3 "a_func"
OpName %5 "out"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%8 = OpTypeFloat 32
%7 = OpTypeVector %8 2
%6 = OpTypePointer Function %7
%9 = OpConstantNull %7
%12 = OpConstant %8 1
%13 = OpConstant %8 2
%14 = OpConstantComposite %7 %12 %13
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpVariable %6 Function %9
%10 = OpExtInst %7 %11 Modf %14 %5
OpReturn
OpFunctionEnd
)";
  EXPECT_EQ(expect, got);

  Validate(b);
}

TEST_F(IntrinsicBuilderTest, Call_Frexp) {
  auto* out = Var("out", ast::StorageClass::kFunction, ty.vec2<i32>());
  auto* expr = Call("frexp", vec2<f32>(1.0f, 2.0f), "out");
  Func("a_func", ast::VariableList{}, ty.void_(),
       ast::StatementList{
           create<ast::VariableDeclStatement>(out),
           create<ast::CallStatement>(expr),
       },
       ast::FunctionDecorationList{
           create<ast::StageDecoration>(ast::PipelineStage::kVertex),
       });

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.Build()) << b.error();
  auto got = DumpBuilder(b);
  auto* expect = R"(OpCapability Shader
%13 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Vertex %3 "a_func"
OpName %3 "a_func"
OpName %5 "out"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%8 = OpTypeInt 32 1
%7 = OpTypeVector %8 2
%6 = OpTypePointer Function %7
%9 = OpConstantNull %7
%12 = OpTypeFloat 32
%11 = OpTypeVector %12 2
%14 = OpConstant %12 1
%15 = OpConstant %12 2
%16 = OpConstantComposite %11 %14 %15
%3 = OpFunction %2 None %1
%4 = OpLabel
%5 = OpVariable %6 Function %9
%10 = OpExtInst %11 %13 Frexp %16 %5
OpReturn
OpFunctionEnd
)";
  EXPECT_EQ(expect, got);

  Validate(b);
}

TEST_F(IntrinsicBuilderTest, Call_Determinant) {
  auto* var = Global("var", ast::StorageClass::kPrivate, ty.mat3x3<f32>());

  auto* expr = Call("determinant", "var");
  WrapInFunction(expr);

  auto* func = Func("a_func", ast::VariableList{}, ty.void_(),
                    ast::StatementList{}, ast::FunctionDecorationList{});

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.GenerateFunction(func)) << b.error();

  ASSERT_TRUE(b.GenerateGlobalVariable(var)) << b.error();
  EXPECT_EQ(b.GenerateCallExpression(expr), 11u) << b.error();

  EXPECT_EQ(DumpBuilder(b), R"(%12 = OpExtInstImport "GLSL.std.450"
OpName %3 "a_func"
OpName %5 "var"
%2 = OpTypeVoid
%1 = OpTypeFunction %2
%9 = OpTypeFloat 32
%8 = OpTypeVector %9 3
%7 = OpTypeMatrix %8 3
%6 = OpTypePointer Private %7
%10 = OpConstantNull %7
%5 = OpVariable %6 Private %10
%3 = OpFunction %2 None %1
%4 = OpLabel
%13 = OpLoad %7 %5
%11 = OpExtInst %9 %12 Determinant %13
OpReturn
OpFunctionEnd
)");
}

TEST_F(IntrinsicBuilderTest, Call_ArrayLength) {
  auto* s = create<ast::Struct>(
      ast::StructMemberList{Member(0, "a", ty.array<f32>(4))},
      ast::StructDecorationList{
          create<ast::StructBlockDecoration>(),
      });
  auto* s_type = ty.struct_("my_struct", s);
  Global("b", ast::StorageClass::kStorage, s_type, nullptr,
         ast::VariableDecorationList{
             create<ast::BindingDecoration>(1),
             create<ast::GroupDecoration>(2),
         });

  auto* expr = Call("arrayLength", MemberAccessor("b", "a"));

  Func("a_func", ast::VariableList{}, ty.void_(),
       ast::StatementList{
           create<ast::CallStatement>(expr),
       },
       ast::FunctionDecorationList{
           create<ast::StageDecoration>(ast::PipelineStage::kVertex),
       });

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.Build()) << b.error();

  ASSERT_EQ(b.functions().size(), 1u);

  auto* expected_types = R"(%5 = OpTypeFloat 32
%4 = OpTypeRuntimeArray %5
%3 = OpTypeStruct %4
%2 = OpTypePointer StorageBuffer %3
%1 = OpVariable %2 StorageBuffer
%7 = OpTypeVoid
%6 = OpTypeFunction %7
%11 = OpTypeInt 32 0
)";
  auto got_types = DumpInstructions(b.types());
  EXPECT_EQ(expected_types, got_types);

  auto* expected_instructions = R"(%10 = OpArrayLength %11 %1 0
)";
  auto got_instructions = DumpInstructions(b.functions()[0].instructions());
  EXPECT_EQ(expected_instructions, got_instructions);

  Validate(b);
}

TEST_F(IntrinsicBuilderTest, Call_ArrayLength_OtherMembersInStruct) {
  auto* s = create<ast::Struct>(
      ast::StructMemberList{Member(0, "z", ty.f32()),
                            Member(4, "a", ty.array<f32>(4))},
      ast::StructDecorationList{
          create<ast::StructBlockDecoration>(),
      });

  auto* s_type = ty.struct_("my_struct", s);
  Global("b", ast::StorageClass::kStorage, s_type, nullptr,
         ast::VariableDecorationList{
             create<ast::BindingDecoration>(1),
             create<ast::GroupDecoration>(2),
         });

  auto* expr = Call("arrayLength", MemberAccessor("b", "a"));

  Func("a_func", ast::VariableList{}, ty.void_(),
       ast::StatementList{
           create<ast::CallStatement>(expr),
       },
       ast::FunctionDecorationList{
           create<ast::StageDecoration>(ast::PipelineStage::kVertex),
       });

  spirv::Builder& b = Build();

  ASSERT_TRUE(b.Build()) << b.error();

  ASSERT_EQ(b.functions().size(), 1u);

  auto* expected_types = R"(%4 = OpTypeFloat 32
%5 = OpTypeRuntimeArray %4
%3 = OpTypeStruct %4 %5
%2 = OpTypePointer StorageBuffer %3
%1 = OpVariable %2 StorageBuffer
%7 = OpTypeVoid
%6 = OpTypeFunction %7
%11 = OpTypeInt 32 0
)";
  auto got_types = DumpInstructions(b.types());
  EXPECT_EQ(expected_types, got_types);

  auto* expected_instructions = R"(%10 = OpArrayLength %11 %1 1
)";
  auto got_instructions = DumpInstructions(b.functions()[0].instructions());
  EXPECT_EQ(expected_instructions, got_instructions);

  Validate(b);
}

}  // namespace
}  // namespace spirv
}  // namespace writer
}  // namespace tint
