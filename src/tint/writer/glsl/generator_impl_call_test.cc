// Copyright 2021 The Tint Authors.
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

#include "src/tint/ast/call_statement.h"
#include "src/tint/utils/string_stream.h"
#include "src/tint/writer/glsl/test_helper.h"

using namespace tint::number_suffixes;  // NOLINT

namespace tint::writer::glsl {
namespace {

using GlslGeneratorImplTest_Call = TestHelper;

TEST_F(GlslGeneratorImplTest_Call, EmitExpression_Call_WithoutParams) {
    Func("my_func", utils::Empty, ty.f32(), utils::Vector{Return(1.23_f)});

    auto* call = Call("my_func");
    WrapInFunction(call);

    GeneratorImpl& gen = Build();

    utils::StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, call)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "my_func()");
}

TEST_F(GlslGeneratorImplTest_Call, EmitExpression_Call_WithParams) {
    Func("my_func",
         utils::Vector{
             Param(Sym(), ty.f32()),
             Param(Sym(), ty.f32()),
         },
         ty.f32(), utils::Vector{Return(1.23_f)});
    GlobalVar("param1", ty.f32(), builtin::AddressSpace::kPrivate);
    GlobalVar("param2", ty.f32(), builtin::AddressSpace::kPrivate);

    auto* call = Call("my_func", "param1", "param2");
    WrapInFunction(call);

    GeneratorImpl& gen = Build();

    utils::StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, call)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "my_func(param1, param2)");
}

TEST_F(GlslGeneratorImplTest_Call, EmitStatement_Call) {
    Func("my_func",
         utils::Vector{
             Param(Sym(), ty.f32()),
             Param(Sym(), ty.f32()),
         },
         ty.void_(), utils::Empty, utils::Empty);
    GlobalVar("param1", ty.f32(), builtin::AddressSpace::kPrivate);
    GlobalVar("param2", ty.f32(), builtin::AddressSpace::kPrivate);

    auto* call = CallStmt(Call("my_func", "param1", "param2"));
    WrapInFunction(call);

    GeneratorImpl& gen = Build();

    gen.increment_indent();
    ASSERT_TRUE(gen.EmitStatement(call)) << gen.Diagnostics();
    EXPECT_EQ(gen.result(), "  my_func(param1, param2);\n");
}

}  // namespace
}  // namespace tint::writer::glsl
