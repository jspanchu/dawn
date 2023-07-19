// Copyright 2023 The Tint Authors.
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

#include "src/tint/type/builtin_structs.h"
#include "src/tint/writer/spirv/ir/test_helper_ir.h"

#include "src/tint/builtin/function.h"

using namespace tint::builtin::fluent_types;  // NOLINT
using namespace tint::number_suffixes;        // NOLINT

namespace tint::writer::spirv {
namespace {

TEST_F(SpvGeneratorImplTest, AtomicAdd_Storage) {
    auto* var = b.Var("var", ty.ptr(storage, ty.atomic(ty.i32())));
    var->SetBindingPoint(0, 0);
    b.RootBlock()->Append(var);

    auto* arg1 = b.FunctionParam("arg1", ty.i32());
    auto* func = b.Function("foo", ty.i32());
    func->SetParams({arg1});

    b.With(func->Block(), [&] {
        auto* ptr = b.Let("ptr", var);
        auto* result = b.Call(ty.i32(), builtin::Function::kAtomicAdd, ptr, arg1);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpAtomicIAdd %int %ptr %uint_1 %uint_0 %arg1");
}

TEST_F(SpvGeneratorImplTest, AtomicAdd_Workgroup) {
    auto* var = b.RootBlock()->Append(b.Var("var", ty.ptr(workgroup, ty.atomic(ty.i32()))));

    auto* arg1 = b.FunctionParam("arg1", ty.i32());
    auto* func = b.Function("foo", ty.i32());
    func->SetParams({arg1});

    b.With(func->Block(), [&] {
        auto* result = b.Call(ty.i32(), builtin::Function::kAtomicAdd, var, arg1);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpAtomicIAdd %int %var %uint_2 %uint_0 %arg1");
}

TEST_F(SpvGeneratorImplTest, AtomicAnd) {
    auto* var = b.RootBlock()->Append(b.Var("var", ty.ptr(workgroup, ty.atomic(ty.i32()))));

    auto* arg1 = b.FunctionParam("arg1", ty.i32());
    auto* func = b.Function("foo", ty.i32());
    func->SetParams({arg1});

    b.With(func->Block(), [&] {
        auto* result = b.Call(ty.i32(), builtin::Function::kAtomicAnd, var, arg1);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpAtomicAnd %int %var %uint_2 %uint_0 %arg1");
}

TEST_F(SpvGeneratorImplTest, AtomicCompareExchangeWeak) {
    auto* var = b.RootBlock()->Append(b.Var("var", ty.ptr(workgroup, ty.atomic(ty.i32()))));

    auto* cmp = b.FunctionParam("cmp", ty.i32());
    auto* val = b.FunctionParam("val", ty.i32());
    auto* func = b.Function("foo", ty.i32());
    func->SetParams({cmp, val});

    b.With(func->Block(), [&] {
        auto* result_ty = type::CreateAtomicCompareExchangeResult(ty, mod.symbols, ty.i32());
        auto* result =
            b.Call(result_ty, builtin::Function::kAtomicCompareExchangeWeak, var, cmp, val);
        auto* original = b.Access(ty.i32(), result, 0_u);
        b.Return(func, original);
        mod.SetName(result, "result");
        mod.SetName(original, "original");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%9 = OpAtomicCompareExchange %int %var %uint_2 %uint_0 %uint_0 %val %cmp");
    EXPECT_INST("%13 = OpIEqual %bool %9 %cmp");
    EXPECT_INST("%result = OpCompositeConstruct %__atomic_compare_exchange_result_i32 %9 %13");
    EXPECT_INST("%original = OpCompositeExtract %int %result 0");
}

TEST_F(SpvGeneratorImplTest, AtomicExchange) {
    auto* var = b.RootBlock()->Append(b.Var("var", ty.ptr(workgroup, ty.atomic(ty.i32()))));

    auto* arg1 = b.FunctionParam("arg1", ty.i32());
    auto* func = b.Function("foo", ty.i32());
    func->SetParams({arg1});

    b.With(func->Block(), [&] {
        auto* result = b.Call(ty.i32(), builtin::Function::kAtomicExchange, var, arg1);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpAtomicExchange %int %var %uint_2 %uint_0 %arg1");
}

TEST_F(SpvGeneratorImplTest, AtomicLoad) {
    auto* var = b.RootBlock()->Append(b.Var("var", ty.ptr(workgroup, ty.atomic(ty.i32()))));

    auto* func = b.Function("foo", ty.i32());

    b.With(func->Block(), [&] {
        auto* result = b.Call(ty.i32(), builtin::Function::kAtomicLoad, var);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpAtomicLoad %int %var %uint_2 %uint_0");
}

TEST_F(SpvGeneratorImplTest, AtomicMax_I32) {
    auto* var = b.RootBlock()->Append(b.Var("var", ty.ptr(workgroup, ty.atomic(ty.i32()))));

    auto* arg1 = b.FunctionParam("arg1", ty.i32());
    auto* func = b.Function("foo", ty.i32());
    func->SetParams({arg1});

    b.With(func->Block(), [&] {
        auto* result = b.Call(ty.i32(), builtin::Function::kAtomicMax, var, arg1);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpAtomicSMax %int %var %uint_2 %uint_0 %arg1");
}

TEST_F(SpvGeneratorImplTest, AtomicMax_U32) {
    auto* var = b.RootBlock()->Append(b.Var("var", ty.ptr(workgroup, ty.atomic(ty.u32()))));

    auto* arg1 = b.FunctionParam("arg1", ty.u32());
    auto* func = b.Function("foo", ty.u32());
    func->SetParams({arg1});

    b.With(func->Block(), [&] {
        auto* result = b.Call(ty.u32(), builtin::Function::kAtomicMax, var, arg1);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpAtomicUMax %uint %var %uint_2 %uint_0 %arg1");
}

TEST_F(SpvGeneratorImplTest, AtomicMin_I32) {
    auto* var = b.RootBlock()->Append(b.Var("var", ty.ptr(workgroup, ty.atomic(ty.i32()))));

    auto* arg1 = b.FunctionParam("arg1", ty.i32());
    auto* func = b.Function("foo", ty.i32());
    func->SetParams({arg1});

    b.With(func->Block(), [&] {
        auto* result = b.Call(ty.i32(), builtin::Function::kAtomicMin, var, arg1);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpAtomicSMin %int %var %uint_2 %uint_0 %arg1");
}

TEST_F(SpvGeneratorImplTest, AtomicMin_U32) {
    auto* var = b.RootBlock()->Append(b.Var("var", ty.ptr(workgroup, ty.atomic(ty.u32()))));

    auto* arg1 = b.FunctionParam("arg1", ty.u32());
    auto* func = b.Function("foo", ty.u32());
    func->SetParams({arg1});

    b.With(func->Block(), [&] {
        auto* result = b.Call(ty.u32(), builtin::Function::kAtomicMin, var, arg1);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpAtomicUMin %uint %var %uint_2 %uint_0 %arg1");
}

TEST_F(SpvGeneratorImplTest, AtomicOr) {
    auto* var = b.RootBlock()->Append(b.Var("var", ty.ptr(workgroup, ty.atomic(ty.i32()))));

    auto* arg1 = b.FunctionParam("arg1", ty.i32());
    auto* func = b.Function("foo", ty.i32());
    func->SetParams({arg1});

    b.With(func->Block(), [&] {
        auto* result = b.Call(ty.i32(), builtin::Function::kAtomicOr, var, arg1);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpAtomicOr %int %var %uint_2 %uint_0 %arg1");
}

TEST_F(SpvGeneratorImplTest, AtomicStore) {
    auto* var = b.RootBlock()->Append(b.Var("var", ty.ptr(workgroup, ty.atomic(ty.i32()))));

    auto* arg1 = b.FunctionParam("arg1", ty.i32());
    auto* func = b.Function("foo", ty.void_());
    func->SetParams({arg1});

    b.With(func->Block(), [&] {
        b.Call(ty.void_(), builtin::Function::kAtomicStore, var, arg1);
        b.Return(func);
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("OpAtomicStore %var %uint_2 %uint_0 %arg1");
}

TEST_F(SpvGeneratorImplTest, AtomicSub) {
    auto* var = b.RootBlock()->Append(b.Var("var", ty.ptr(workgroup, ty.atomic(ty.i32()))));

    auto* arg1 = b.FunctionParam("arg1", ty.i32());
    auto* func = b.Function("foo", ty.i32());
    func->SetParams({arg1});

    b.With(func->Block(), [&] {
        auto* result = b.Call(ty.i32(), builtin::Function::kAtomicSub, var, arg1);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpAtomicISub %int %var %uint_2 %uint_0 %arg1");
}

TEST_F(SpvGeneratorImplTest, AtomicXor) {
    auto* var = b.RootBlock()->Append(b.Var("var", ty.ptr(workgroup, ty.atomic(ty.i32()))));

    auto* arg1 = b.FunctionParam("arg1", ty.i32());
    auto* func = b.Function("foo", ty.i32());
    func->SetParams({arg1});

    b.With(func->Block(), [&] {
        auto* result = b.Call(ty.i32(), builtin::Function::kAtomicXor, var, arg1);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpAtomicXor %int %var %uint_2 %uint_0 %arg1");
}

}  // namespace
}  // namespace tint::writer::spirv
