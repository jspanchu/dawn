// Copyright 2024 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "src/tint/lang/hlsl/writer/raise/decompose_memory_access.h"

#include <gtest/gtest.h>

#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/function.h"
#include "src/tint/lang/core/ir/transform/helper_test.h"
#include "src/tint/lang/core/number.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::hlsl::writer::raise {
namespace {

using HlslWriterDecomposeMemoryAccessTest = core::ir::transform::TransformTest;

TEST_F(HlslWriterDecomposeMemoryAccessTest, NoBufferAccess) {
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] { b.Return(func); });

    auto* src = R"(
%foo = @fragment func():void {
  $B1: {
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = src;
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, AccessChainFromUnnamedAccessChain) {
    auto* Inner = ty.Struct(mod.symbols.New("Inner"), {
                                                          {mod.symbols.New("c"), ty.f32()},
                                                          {mod.symbols.New("d"), ty.u32()},
                                                      });
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), Inner},
                                                });

    auto* var = b.Var("v", storage, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* x = b.Access(ty.ptr(storage, sb, core::Access::kReadWrite), var);
        auto* y = b.Access(ty.ptr(storage, Inner, core::Access::kReadWrite), x->Result(0), 1_u);
        b.Let("b", b.Load(b.Access(ty.ptr(storage, ty.u32(), core::Access::kReadWrite),
                                   y->Result(0), 1_u)));
        b.Return(func);
    });

    auto* src = R"(
Inner = struct @align(4) {
  c:f32 @offset(0)
  d:u32 @offset(4)
}

SB = struct @align(4) {
  a:i32 @offset(0)
  b:Inner @offset(4)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, SB, read_write> = access %v
    %4:ptr<storage, Inner, read_write> = access %3, 1u
    %5:ptr<storage, u32, read_write> = access %4, 1u
    %6:u32 = load %5
    %b:u32 = let %6
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
Inner = struct @align(4) {
  c:f32 @offset(0)
  d:u32 @offset(4)
}

SB = struct @align(4) {
  a:i32 @offset(0)
  b:Inner @offset(4)
}

$B1: {  # root
  %v:hlsl.byte_address_buffer<read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:u32 = %v.Load 8u
    %4:u32 = bitcast %3
    %b:u32 = let %4
    ret
  }
}
)";

    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, AccessChainFromLetAccessChain) {
    auto* Inner = ty.Struct(mod.symbols.New("Inner"), {
                                                          {mod.symbols.New("c"), ty.f32()},
                                                      });
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), Inner},
                                                });

    auto* var = b.Var("v", storage, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* x = b.Let("x", var);
        auto* y = b.Let(
            "y", b.Access(ty.ptr(storage, Inner, core::Access::kReadWrite), x->Result(0), 1_u));
        auto* z = b.Let(
            "z", b.Access(ty.ptr(storage, ty.f32(), core::Access::kReadWrite), y->Result(0), 0_u));
        b.Let("a", b.Load(z));
        b.Return(func);
    });

    auto* src = R"(
Inner = struct @align(4) {
  c:f32 @offset(0)
}

SB = struct @align(4) {
  a:i32 @offset(0)
  b:Inner @offset(4)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %x:ptr<storage, SB, read_write> = let %v
    %4:ptr<storage, Inner, read_write> = access %x, 1u
    %y:ptr<storage, Inner, read_write> = let %4
    %6:ptr<storage, f32, read_write> = access %y, 0u
    %z:ptr<storage, f32, read_write> = let %6
    %8:f32 = load %z
    %a:f32 = let %8
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
Inner = struct @align(4) {
  c:f32 @offset(0)
}

SB = struct @align(4) {
  a:i32 @offset(0)
  b:Inner @offset(4)
}

$B1: {  # root
  %v:hlsl.byte_address_buffer<read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:u32 = %v.Load 4u
    %4:f32 = bitcast %3
    %a:f32 = let %4
    ret
  }
}
)";

    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, AccessRwByteAddressBuffer) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), ty.vec3<f32>()},
                                                });

    auto* var = b.Var("v", storage, sb, core::Access::kReadWrite);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(b.Access(ty.ptr(storage, ty.i32(), core::Access::kReadWrite), var, 0_u)));
        b.Let("b", b.Load(b.Access(ty.ptr(storage, ty.vec3<f32>(), core::Access::kReadWrite), var,
                                   1_u)));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  a:i32 @offset(0)
  b:vec3<f32> @offset(16)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, i32, read_write> = access %v, 0u
    %4:i32 = load %3
    %a:i32 = let %4
    %6:ptr<storage, vec3<f32>, read_write> = access %v, 1u
    %7:vec3<f32> = load %6
    %b:vec3<f32> = let %7
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
SB = struct @align(16) {
  a:i32 @offset(0)
  b:vec3<f32> @offset(16)
}

$B1: {  # root
  %v:hlsl.byte_address_buffer<read_write> = var
}

%foo = @fragment func():void {
  $B2: {
    %3:u32 = %v.Load 0u
    %4:i32 = bitcast %3
    %a:i32 = let %4
    %6:vec3<u32> = %v.Load3 16u
    %7:vec3<f32> = bitcast %6
    %b:vec3<f32> = let %7
    ret
  }
}
)";

    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, AccessByteAddressBuffer) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                });
    auto* var = b.Var("v", storage, sb, core::Access::kRead);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(b.Access(ty.ptr(storage, ty.i32(), core::Access::kRead), var, 0_u)));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(4) {
  a:i32 @offset(0)
}

$B1: {  # root
  %v:ptr<storage, SB, read> = var
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, i32, read> = access %v, 0u
    %4:i32 = load %3
    %a:i32 = let %4
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
SB = struct @align(4) {
  a:i32 @offset(0)
}

$B1: {  # root
  %v:hlsl.byte_address_buffer<read> = var
}

%foo = @fragment func():void {
  $B2: {
    %3:u32 = %v.Load 0u
    %4:i32 = bitcast %3
    %a:i32 = let %4
    ret
  }
}
)";

    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, AccessStorageVector) {
    auto* var = b.Var<storage, vec4<f32>, core::Access::kRead>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        b.Let("b", b.LoadVectorElement(var, 0_u));
        b.Let("c", b.LoadVectorElement(var, 1_u));
        b.Let("d", b.LoadVectorElement(var, 2_u));
        b.Let("e", b.LoadVectorElement(var, 3_u));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, vec4<f32>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:vec4<f32> = load %v
    %a:vec4<f32> = let %3
    %5:f32 = load_vector_element %v, 0u
    %b:f32 = let %5
    %7:f32 = load_vector_element %v, 1u
    %c:f32 = let %7
    %9:f32 = load_vector_element %v, 2u
    %d:f32 = let %9
    %11:f32 = load_vector_element %v, 3u
    %e:f32 = let %11
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:hlsl.byte_address_buffer<read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:vec4<u32> = %v.Load4 0u
    %4:vec4<f32> = bitcast %3
    %a:vec4<f32> = let %4
    %6:u32 = %v.Load 0u
    %7:f32 = bitcast %6
    %b:f32 = let %7
    %9:u32 = %v.Load 4u
    %10:f32 = bitcast %9
    %c:f32 = let %10
    %12:u32 = %v.Load 8u
    %13:f32 = bitcast %12
    %d:f32 = let %13
    %15:u32 = %v.Load 12u
    %16:f32 = bitcast %15
    %e:f32 = let %16
    ret
  }
}
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, AccessStorageVectorF16) {
    auto* var = b.Var<storage, vec4<f16>, core::Access::kRead>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        b.Let("b", b.LoadVectorElement(var, 0_u));
        b.Let("c", b.LoadVectorElement(var, 1_u));
        b.Let("d", b.LoadVectorElement(var, 2_u));
        b.Let("e", b.LoadVectorElement(var, 3_u));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, vec4<f16>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:vec4<f16> = load %v
    %a:vec4<f16> = let %3
    %5:f16 = load_vector_element %v, 0u
    %b:f16 = let %5
    %7:f16 = load_vector_element %v, 1u
    %c:f16 = let %7
    %9:f16 = load_vector_element %v, 2u
    %d:f16 = let %9
    %11:f16 = load_vector_element %v, 3u
    %e:f16 = let %11
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:hlsl.byte_address_buffer<read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:vec4<f16> = %v.Load4F16 0u
    %a:vec4<f16> = let %3
    %5:f16 = %v.LoadF16 0u
    %b:f16 = let %5
    %7:f16 = %v.LoadF16 2u
    %c:f16 = let %7
    %9:f16 = %v.LoadF16 4u
    %d:f16 = let %9
    %11:f16 = %v.LoadF16 6u
    %e:f16 = let %11
    ret
  }
}
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, AccessStorageMatrix) {
    auto* var = b.Var<storage, mat4x4<f32>, core::Access::kRead>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        b.Let("b", b.Load(b.Access(ty.ptr<storage, vec4<f32>, core::Access::kRead>(), var, 3_u)));
        b.Let("c", b.LoadVectorElement(
                       b.Access(ty.ptr<storage, vec4<f32>, core::Access::kRead>(), var, 1_u), 2_u));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, mat4x4<f32>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:mat4x4<f32> = load %v
    %a:mat4x4<f32> = let %3
    %5:ptr<storage, vec4<f32>, read> = access %v, 3u
    %6:vec4<f32> = load %5
    %b:vec4<f32> = let %6
    %8:ptr<storage, vec4<f32>, read> = access %v, 1u
    %9:f32 = load_vector_element %8, 2u
    %c:f32 = let %9
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:hlsl.byte_address_buffer<read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:mat4x4<f32> = call %4, 0u
    %a:mat4x4<f32> = let %3
    %6:vec4<u32> = %v.Load4 48u
    %7:vec4<f32> = bitcast %6
    %b:vec4<f32> = let %7
    %9:u32 = %v.Load 24u
    %10:f32 = bitcast %9
    %c:f32 = let %10
    ret
  }
}
%4 = func(%offset:u32):mat4x4<f32> {
  $B3: {
    %13:u32 = add %offset, 0u
    %14:vec4<u32> = %v.Load4 %13
    %15:vec4<f32> = bitcast %14
    %16:u32 = add %offset, 16u
    %17:vec4<u32> = %v.Load4 %16
    %18:vec4<f32> = bitcast %17
    %19:u32 = add %offset, 32u
    %20:vec4<u32> = %v.Load4 %19
    %21:vec4<f32> = bitcast %20
    %22:u32 = add %offset, 48u
    %23:vec4<u32> = %v.Load4 %22
    %24:vec4<f32> = bitcast %23
    %25:mat4x4<f32> = construct %15, %18, %21, %24
    ret %25
  }
}
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, AccessStorageArray) {
    auto* var = b.Var<storage, array<vec3<f32>, 5>, core::Access::kRead>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        b.Let("b", b.Load(b.Access(ty.ptr<storage, vec3<f32>, core::Access::kRead>(), var, 3_u)));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, array<vec3<f32>, 5>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:array<vec3<f32>, 5> = load %v
    %a:array<vec3<f32>, 5> = let %3
    %5:ptr<storage, vec3<f32>, read> = access %v, 3u
    %6:vec3<f32> = load %5
    %b:vec3<f32> = let %6
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:hlsl.byte_address_buffer<read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:array<vec3<f32>, 5> = call %4, 0u
    %a:array<vec3<f32>, 5> = let %3
    %6:vec3<u32> = %v.Load3 48u
    %7:vec3<f32> = bitcast %6
    %b:vec3<f32> = let %7
    ret
  }
}
%4 = func(%offset:u32):array<vec3<f32>, 5> {
  $B3: {
    %a_1:ptr<function, array<vec3<f32>, 5>, read_write> = var, array<vec3<f32>, 5>(vec3<f32>(0.0f))  # %a_1: 'a'
    loop [i: $B4, b: $B5, c: $B6] {  # loop_1
      $B4: {  # initializer
        next_iteration 0u  # -> $B5
      }
      $B5 (%idx:u32): {  # body
        %12:bool = gte %idx, 5u
        if %12 [t: $B7] {  # if_1
          $B7: {  # true
            exit_loop  # loop_1
          }
        }
        %13:ptr<function, vec3<f32>, read_write> = access %a_1, %idx
        %14:u32 = mul %idx, 16u
        %15:u32 = add %offset, %14
        %16:vec3<u32> = %v.Load3 %15
        %17:vec3<f32> = bitcast %16
        store %13, %17
        continue  # -> $B6
      }
      $B6: {  # continuing
        %18:u32 = add %idx, 1u
        next_iteration %18  # -> $B5
      }
    }
    %19:array<vec3<f32>, 5> = load %a_1
    ret %19
  }
}
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, AccessStorageArrayWhichCanHaveSizesOtherThenFive) {
    auto* var = b.Var<storage, array<vec3<f32>, 42>, core::Access::kRead>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        b.Let("b", b.Load(b.Access(ty.ptr<storage, vec3<f32>, core::Access::kRead>(), var, 3_u)));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, array<vec3<f32>, 42>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:array<vec3<f32>, 42> = load %v
    %a:array<vec3<f32>, 42> = let %3
    %5:ptr<storage, vec3<f32>, read> = access %v, 3u
    %6:vec3<f32> = load %5
    %b:vec3<f32> = let %6
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:hlsl.byte_address_buffer<read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:array<vec3<f32>, 42> = call %4, 0u
    %a:array<vec3<f32>, 42> = let %3
    %6:vec3<u32> = %v.Load3 48u
    %7:vec3<f32> = bitcast %6
    %b:vec3<f32> = let %7
    ret
  }
}
%4 = func(%offset:u32):array<vec3<f32>, 42> {
  $B3: {
    %a_1:ptr<function, array<vec3<f32>, 42>, read_write> = var, array<vec3<f32>, 42>(vec3<f32>(0.0f))  # %a_1: 'a'
    loop [i: $B4, b: $B5, c: $B6] {  # loop_1
      $B4: {  # initializer
        next_iteration 0u  # -> $B5
      }
      $B5 (%idx:u32): {  # body
        %12:bool = gte %idx, 42u
        if %12 [t: $B7] {  # if_1
          $B7: {  # true
            exit_loop  # loop_1
          }
        }
        %13:ptr<function, vec3<f32>, read_write> = access %a_1, %idx
        %14:u32 = mul %idx, 16u
        %15:u32 = add %offset, %14
        %16:vec3<u32> = %v.Load3 %15
        %17:vec3<f32> = bitcast %16
        store %13, %17
        continue  # -> $B6
      }
      $B6: {  # continuing
        %18:u32 = add %idx, 1u
        next_iteration %18  # -> $B5
      }
    }
    %19:array<vec3<f32>, 42> = load %a_1
    ret %19
  }
}
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, AccessStorageStruct) {
    auto* SB = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), ty.f32()},
                                                });

    auto* var = b.Var("v", storage, SB, core::Access::kRead);
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        b.Let("b", b.Load(b.Access(ty.ptr<storage, f32, core::Access::kRead>(), var, 1_u)));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(4) {
  a:i32 @offset(0)
  b:f32 @offset(4)
}

$B1: {  # root
  %v:ptr<storage, SB, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:SB = load %v
    %a:SB = let %3
    %5:ptr<storage, f32, read> = access %v, 1u
    %6:f32 = load %5
    %b:f32 = let %6
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
SB = struct @align(4) {
  a:i32 @offset(0)
  b:f32 @offset(4)
}

$B1: {  # root
  %v:hlsl.byte_address_buffer<read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:SB = call %4, 0u
    %a:SB = let %3
    %6:u32 = %v.Load 4u
    %7:f32 = bitcast %6
    %b:f32 = let %7
    ret
  }
}
%4 = func(%offset:u32):SB {
  $B3: {
    %10:u32 = add %offset, 0u
    %11:u32 = %v.Load %10
    %12:i32 = bitcast %11
    %13:u32 = add %offset, 4u
    %14:u32 = %v.Load %13
    %15:f32 = bitcast %14
    %16:SB = construct %12, %15
    ret %16
  }
}
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, AccessStorageNested) {
    auto* Inner =
        ty.Struct(mod.symbols.New("Inner"), {
                                                {mod.symbols.New("s"), ty.mat3x3<f32>()},
                                                {mod.symbols.New("t"), ty.array<vec3<f32>, 5>()},
                                            });
    auto* Outer = ty.Struct(mod.symbols.New("Outer"), {
                                                          {mod.symbols.New("x"), ty.f32()},
                                                          {mod.symbols.New("y"), Inner},
                                                      });

    auto* SB = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), Outer},
                                                });

    auto* var = b.Var("v", storage, SB, core::Access::kRead);
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        b.Let("b", b.LoadVectorElement(b.Access(ty.ptr<storage, vec3<f32>, core::Access::kRead>(),
                                                var, 1_u, 1_u, 1_u, 3_u),
                                       2_u));
        b.Return(func);
    });

    auto* src = R"(
Inner = struct @align(16) {
  s:mat3x3<f32> @offset(0)
  t:array<vec3<f32>, 5> @offset(48)
}

Outer = struct @align(16) {
  x:f32 @offset(0)
  y:Inner @offset(16)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Outer @offset(16)
}

$B1: {  # root
  %v:ptr<storage, SB, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:SB = load %v
    %a:SB = let %3
    %5:ptr<storage, vec3<f32>, read> = access %v, 1u, 1u, 1u, 3u
    %6:f32 = load_vector_element %5, 2u
    %b:f32 = let %6
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
Inner = struct @align(16) {
  s:mat3x3<f32> @offset(0)
  t:array<vec3<f32>, 5> @offset(48)
}

Outer = struct @align(16) {
  x:f32 @offset(0)
  y:Inner @offset(16)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Outer @offset(16)
}

$B1: {  # root
  %v:hlsl.byte_address_buffer<read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:SB = call %4, 0u
    %a:SB = let %3
    %6:u32 = %v.Load 136u
    %7:f32 = bitcast %6
    %b:f32 = let %7
    ret
  }
}
%4 = func(%offset:u32):SB {
  $B3: {
    %10:u32 = add %offset, 0u
    %11:u32 = %v.Load %10
    %12:i32 = bitcast %11
    %13:u32 = add %offset, 16u
    %14:Outer = call %15, %13
    %16:SB = construct %12, %14
    ret %16
  }
}
%15 = func(%offset_1:u32):Outer {  # %offset_1: 'offset'
  $B4: {
    %18:u32 = add %offset_1, 0u
    %19:u32 = %v.Load %18
    %20:f32 = bitcast %19
    %21:u32 = add %offset_1, 16u
    %22:Inner = call %23, %21
    %24:Outer = construct %20, %22
    ret %24
  }
}
%23 = func(%offset_2:u32):Inner {  # %offset_2: 'offset'
  $B5: {
    %26:u32 = add %offset_2, 0u
    %27:mat3x3<f32> = call %28, %26
    %29:u32 = add %offset_2, 48u
    %30:array<vec3<f32>, 5> = call %31, %29
    %32:Inner = construct %27, %30
    ret %32
  }
}
%28 = func(%offset_3:u32):mat3x3<f32> {  # %offset_3: 'offset'
  $B6: {
    %34:u32 = add %offset_3, 0u
    %35:vec3<u32> = %v.Load3 %34
    %36:vec3<f32> = bitcast %35
    %37:u32 = add %offset_3, 16u
    %38:vec3<u32> = %v.Load3 %37
    %39:vec3<f32> = bitcast %38
    %40:u32 = add %offset_3, 32u
    %41:vec3<u32> = %v.Load3 %40
    %42:vec3<f32> = bitcast %41
    %43:mat3x3<f32> = construct %36, %39, %42
    ret %43
  }
}
%31 = func(%offset_4:u32):array<vec3<f32>, 5> {  # %offset_4: 'offset'
  $B7: {
    %a_1:ptr<function, array<vec3<f32>, 5>, read_write> = var, array<vec3<f32>, 5>(vec3<f32>(0.0f))  # %a_1: 'a'
    loop [i: $B8, b: $B9, c: $B10] {  # loop_1
      $B8: {  # initializer
        next_iteration 0u  # -> $B9
      }
      $B9 (%idx:u32): {  # body
        %47:bool = gte %idx, 5u
        if %47 [t: $B11] {  # if_1
          $B11: {  # true
            exit_loop  # loop_1
          }
        }
        %48:ptr<function, vec3<f32>, read_write> = access %a_1, %idx
        %49:u32 = mul %idx, 16u
        %50:u32 = add %offset_4, %49
        %51:vec3<u32> = %v.Load3 %50
        %52:vec3<f32> = bitcast %51
        store %48, %52
        continue  # -> $B10
      }
      $B10: {  # continuing
        %53:u32 = add %idx, 1u
        next_iteration %53  # -> $B9
      }
    }
    %54:array<vec3<f32>, 5> = load %a_1
    ret %54
  }
}
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, ComplexStaticAccessChain) {
    auto* S1 = ty.Struct(mod.symbols.New("S1"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), ty.vec3<f32>()},
                                                    {mod.symbols.New("c"), ty.i32()},
                                                });
    auto* S2 = ty.Struct(mod.symbols.New("S2"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), ty.array(S1, 3)},
                                                    {mod.symbols.New("c"), ty.i32()},
                                                });

    auto* SB = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), ty.runtime_array(S2)},
                                                });

    auto* var = b.Var("sb", storage, SB, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.LoadVectorElement(b.Access(ty.ptr<storage, vec3<f32>, read_write>(),
                                                // var.b[4].b[1].b.z
                                                //   .b   [4]  .b   [1] .b
                                                var, 1_u, 4_u, 1_u, 1_u, 1_u),
                                       2_u));
        b.Return(func);
    });

    auto* src = R"(
S1 = struct @align(16) {
  a:i32 @offset(0)
  b:vec3<f32> @offset(16)
  c:i32 @offset(28)
}

S2 = struct @align(16) {
  a_1:i32 @offset(0)
  b_1:array<S1, 3> @offset(16)
  c_1:i32 @offset(112)
}

SB = struct @align(16) {
  a_2:i32 @offset(0)
  b_2:array<S2> @offset(16)
}

$B1: {  # root
  %sb:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, vec3<f32>, read_write> = access %sb, 1u, 4u, 1u, 1u, 1u
    %4:f32 = load_vector_element %3, 2u
    %x:f32 = let %4
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    // sb.b[4].b[1].b.z
    //    ^  ^ ^  ^ ^ ^
    //    |  | |  | | |
    //   16  | |576 | 600
    //       | |    |
    //     528 544  592

    auto* expect = R"(
S1 = struct @align(16) {
  a:i32 @offset(0)
  b:vec3<f32> @offset(16)
  c:i32 @offset(28)
}

S2 = struct @align(16) {
  a_1:i32 @offset(0)
  b_1:array<S1, 3> @offset(16)
  c_1:i32 @offset(112)
}

SB = struct @align(16) {
  a_2:i32 @offset(0)
  b_2:array<S2> @offset(16)
}

$B1: {  # root
  %sb:hlsl.byte_address_buffer<read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:u32 = %sb.Load 600u
    %4:f32 = bitcast %3
    %x:f32 = let %4
    ret
  }
}
)";

    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_ComplexDynamicAccessChain) {
    auto* S1 = ty.Struct(mod.symbols.New("S1"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), ty.vec3<f32>()},
                                                    {mod.symbols.New("c"), ty.i32()},
                                                });
    auto* S2 = ty.Struct(mod.symbols.New("S2"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), ty.array(S1, 3)},
                                                    {mod.symbols.New("c"), ty.i32()},
                                                });

    auto* SB = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), ty.runtime_array(S2)},
                                                });

    auto* var = b.Var("sb", storage, SB, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* i = b.Load(b.Var("i", 4_i));
        auto* j = b.Load(b.Var("j", 1_u));
        auto* k = b.Load(b.Var("k", 2_i));
        // let x : f32 = sb.b[i].b[j].b[k];
        b.Let("x",
              b.LoadVectorElement(
                  b.Access(ty.ptr<storage, vec3<f32>, read_write>(), var, 1_u, i, 1_u, j, 1_u), k));
        b.Return(func);
    });

    auto* src = R"(
S1 = struct @align(16) {
  a:i32 @offset(0)
  b:vec3<f32> @offset(16)
  c:i32 @offset(28)
}

S2 = struct @align(16) {
  a_1:i32 @offset(0)
  b_1:array<S1, 3> @offset(16)
  c_1:i32 @offset(112)
}

SB = struct @align(16) {
  a_2:i32 @offset(0)
  b_2:array<S2> @offset(16)
}

$B1: {  # root
  %sb:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %i:ptr<function, i32, read_write> = var, 4i
    %4:i32 = load %i
    %j:ptr<function, u32, read_write> = var, 1u
    %6:u32 = load %j
    %k:ptr<function, i32, read_write> = var, 2i
    %8:i32 = load %k
    %9:ptr<storage, vec3<f32>, read_write> = access %sb, 1u, %4, 1u, %6, 1u
    %10:f32 = load_vector_element %9, %8
    %x:f32 = let %10
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
S1 = struct @align(16) {
  a:i32 @offset(0)
  b:vec3<f32> @offset(16)
  c:i32 @offset(28)
}

S2 = struct @align(16) {
  a_1:i32 @offset(0)
  b_1:array<S1, 3> @offset(16)
  c_1:i32 @offset(112)
}

SB = struct @align(16) {
  a_2:i32 @offset(0)
  b_2:array<S2> @offset(16)
}

$B1: {  # root
  %sb:hlsl.byte_address_buffer<read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %i:ptr<function, i32, read_write> = var, 4i
    %4:i32 = load %i
    %j:ptr<function, u32, read_write> = var, 1u
    %6:u32 = load %j
    %k:ptr<function, i32, read_write> = var, 2i
    %8:i32 = load %k
    %10:f32 = bitcast %9
    %x:f32 = let %10
    ret
  }
}
)";

    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_UniformAccessChainFromUnnamedAccessChain) {
    auto* Inner = ty.Struct(mod.symbols.New("Inner"), {
                                                          {mod.symbols.New("c"), ty.f32()},
                                                          {mod.symbols.New("d"), ty.u32()},
                                                      });

    tint::Vector<const core::type::StructMember*, 2> members;
    members.Push(ty.Get<core::type::StructMember>(mod.symbols.New("a"), ty.i32(), 0u, 0u, 4u,
                                                  ty.i32()->Size(), core::IOAttributes{}));
    members.Push(ty.Get<core::type::StructMember>(mod.symbols.New("b"), Inner, 1u, 16u, 16u,
                                                  Inner->Size(), core::IOAttributes{}));
    auto* sb = ty.Struct(mod.symbols.New("SB"), members);

    auto* var = b.Var("v", uniform, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* x = b.Access(ty.ptr(storage, sb, core::Access::kReadWrite), var);
        auto* y = b.Access(ty.ptr(storage, Inner, core::Access::kReadWrite), x->Result(0), 1_u);
        b.Let("b", b.Load(b.Access(ty.ptr(storage, ty.u32(), core::Access::kReadWrite),
                                   y->Result(0), 1_u)));
        b.Return(func);
    });

    auto* src = R"(
Inner = struct @align(4) {
  c:f32 @offset(0)
  d:u32 @offset(4)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Inner @offset(16)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, SB, read_write> = access %v
    %4:ptr<storage, Inner, read_write> = access %3, 1u
    %5:ptr<storage, u32, read_write> = access %4, 1u
    %6:u32 = load %5
    %b:u32 = let %6
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
Inner = struct @align(4) {
  c:f32 @offset(0)
  d:u32 @offset(4)
}

SB = struct @align(4) {
  a:i32 @offset(0)
  b:Inner @offset(4)
}

$B1: {  # root
  %v:hlsl.byte_address_buffer<read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:u32 = %v.Load 8u
    %4:u32 = bitcast %3
    %b:u32 = let %4
    ret
  }
}
)";

    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_UniformAccessChainFromLetAccessChain) {
    auto* Inner = ty.Struct(mod.symbols.New("Inner"), {
                                                          {mod.symbols.New("c"), ty.f32()},
                                                      });

    tint::Vector<const core::type::StructMember*, 2> members;
    members.Push(ty.Get<core::type::StructMember>(mod.symbols.New("a"), ty.i32(), 0u, 0u, 4u,
                                                  ty.i32()->Size(), core::IOAttributes{}));
    members.Push(ty.Get<core::type::StructMember>(mod.symbols.New("b"), Inner, 1u, 16u, 16u,
                                                  Inner->Size(), core::IOAttributes{}));
    auto* sb = ty.Struct(mod.symbols.New("SB"), members);

    auto* var = b.Var("v", uniform, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* x = b.Let("x", var);
        auto* y = b.Let(
            "y", b.Access(ty.ptr(storage, Inner, core::Access::kReadWrite), x->Result(0), 1_u));
        auto* z = b.Let(
            "z", b.Access(ty.ptr(storage, ty.f32(), core::Access::kReadWrite), y->Result(0), 0_u));
        b.Let("a", b.Load(z));
        b.Return(func);
    });

    auto* src = R"(
Inner = struct @align(4) {
  c:f32 @offset(0)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Inner @offset(16)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %x:ptr<storage, SB, read_write> = let %v
    %4:ptr<storage, Inner, read_write> = access %x, 1u
    %y:ptr<storage, Inner, read_write> = let %4
    %6:ptr<storage, f32, read_write> = access %y, 0u
    %z:ptr<storage, f32, read_write> = let %6
    %8:f32 = load %z
    %a:f32 = let %8
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
Inner = struct @align(4) {
  c:f32 @offset(0)
}

SB = struct @align(4) {
  a:i32 @offset(0)
  b:Inner @offset(4)
}

$B1: {  # root
  %v:hlsl.byte_address_buffer<read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:u32 = %v.Load 4u
    %4:f32 = bitcast %3
    %a:f32 = let %4
    ret
  }
}
)";

    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_UniformAccessVector) {
    auto* var = b.Var<uniform, vec4<f32>, core::Access::kRead>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        b.Let("b", b.LoadVectorElement(var, 0_u));
        b.Let("c", b.LoadVectorElement(var, 1_u));
        b.Let("d", b.LoadVectorElement(var, 2_u));
        b.Let("e", b.LoadVectorElement(var, 3_u));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, vec4<f32>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:vec4<f32> = load %v
    %a:vec4<f32> = let %3
    %5:f32 = load_vector_element %v, 0u
    %b:f32 = let %5
    %7:f32 = load_vector_element %v, 1u
    %c:f32 = let %7
    %9:f32 = load_vector_element %v, 2u
    %d:f32 = let %9
    %11:f32 = load_vector_element %v, 3u
    %e:f32 = let %11
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:hlsl.byte_address_buffer<read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:vec4<u32> = %v.Load4 0u
    %4:vec4<f32> = bitcast %3
    %a:vec4<f32> = let %4
    %6:u32 = %v.Load 0u
    %7:f32 = bitcast %6
    %b:f32 = let %7
    %9:u32 = %v.Load 4u
    %10:f32 = bitcast %9
    %c:f32 = let %10
    %12:u32 = %v.Load 8u
    %13:f32 = bitcast %12
    %d:f32 = let %13
    %15:u32 = %v.Load 12u
    %16:f32 = bitcast %15
    %e:f32 = let %16
    ret
  }
}
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_UniformAccessVectorF16) {
    auto* var = b.Var<uniform, vec4<f16>, core::Access::kRead>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        b.Let("b", b.LoadVectorElement(var, 0_u));
        b.Let("c", b.LoadVectorElement(var, 1_u));
        b.Let("d", b.LoadVectorElement(var, 2_u));
        b.Let("e", b.LoadVectorElement(var, 3_u));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, vec4<f16>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:vec4<f16> = load %v
    %a:vec4<f16> = let %3
    %5:f16 = load_vector_element %v, 0u
    %b:f16 = let %5
    %7:f16 = load_vector_element %v, 1u
    %c:f16 = let %7
    %9:f16 = load_vector_element %v, 2u
    %d:f16 = let %9
    %11:f16 = load_vector_element %v, 3u
    %e:f16 = let %11
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:hlsl.byte_address_buffer<read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:vec4<f16> = %v.Load4F16 0u
    %a:vec4<f16> = let %3
    %5:f16 = %v.LoadF16 0u
    %b:f16 = let %5
    %7:f16 = %v.LoadF16 2u
    %c:f16 = let %7
    %9:f16 = %v.LoadF16 4u
    %d:f16 = let %9
    %11:f16 = %v.LoadF16 6u
    %e:f16 = let %11
    ret
  }
}
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_UniformAccessMatrix) {
    auto* var = b.Var<uniform, mat4x4<f32>, core::Access::kRead>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        b.Let("b", b.Load(b.Access(ty.ptr<storage, vec4<f32>, core::Access::kRead>(), var, 3_u)));
        b.Let("c", b.LoadVectorElement(
                       b.Access(ty.ptr<storage, vec4<f32>, core::Access::kRead>(), var, 1_u), 2_u));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, mat4x4<f32>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:mat4x4<f32> = load %v
    %a:mat4x4<f32> = let %3
    %5:ptr<storage, vec4<f32>, read> = access %v, 3u
    %6:vec4<f32> = load %5
    %b:vec4<f32> = let %6
    %8:ptr<storage, vec4<f32>, read> = access %v, 1u
    %9:f32 = load_vector_element %8, 2u
    %c:f32 = let %9
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:hlsl.byte_address_buffer<read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:mat4x4<f32> = call %4, 0u
    %a:mat4x4<f32> = let %3
    %6:vec4<u32> = %v.Load4 48u
    %7:vec4<f32> = bitcast %6
    %b:vec4<f32> = let %7
    %9:u32 = %v.Load 24u
    %10:f32 = bitcast %9
    %c:f32 = let %10
    ret
  }
}
%4 = func(%offset:u32):mat4x4<f32> {
  $B3: {
    %13:u32 = add %offset, 0u
    %14:vec4<u32> = %v.Load4 %13
    %15:vec4<f32> = bitcast %14
    %16:u32 = add %offset, 16u
    %17:vec4<u32> = %v.Load4 %16
    %18:vec4<f32> = bitcast %17
    %19:u32 = add %offset, 32u
    %20:vec4<u32> = %v.Load4 %19
    %21:vec4<f32> = bitcast %20
    %22:u32 = add %offset, 48u
    %23:vec4<u32> = %v.Load4 %22
    %24:vec4<f32> = bitcast %23
    %25:mat4x4<f32> = construct %15, %18, %21, %24
    ret %25
  }
}
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_UniformAccessArray) {
    auto* var = b.Var<uniform, array<vec3<f32>, 5>, core::Access::kRead>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        b.Let("b", b.Load(b.Access(ty.ptr<storage, vec3<f32>, core::Access::kRead>(), var, 3_u)));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, array<vec3<f32>, 5>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:array<vec3<f32>, 5> = load %v
    %a:array<vec3<f32>, 5> = let %3
    %5:ptr<storage, vec3<f32>, read> = access %v, 3u
    %6:vec3<f32> = load %5
    %b:vec3<f32> = let %6
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:hlsl.byte_address_buffer<read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:array<vec3<f32>, 5> = call %4, 0u
    %a:array<vec3<f32>, 5> = let %3
    %6:vec3<u32> = %v.Load3 48u
    %7:vec3<f32> = bitcast %6
    %b:vec3<f32> = let %7
    ret
  }
}
%4 = func(%offset:u32):array<vec3<f32>, 5> {
  $B3: {
    %a_1:ptr<function, array<vec3<f32>, 5>, read_write> = var, array<vec3<f32>, 5>(vec3<f32>(0.0f))  # %a_1: 'a'
    loop [i: $B4, b: $B5, c: $B6] {  # loop_1
      $B4: {  # initializer
        next_iteration 0u  # -> $B5
      }
      $B5 (%idx:u32): {  # body
        %12:bool = gte %idx, 5u
        if %12 [t: $B7] {  # if_1
          $B7: {  # true
            exit_loop  # loop_1
          }
        }
        %13:ptr<function, vec3<f32>, read_write> = access %a_1, %idx
        %14:u32 = mul %idx, 16u
        %15:u32 = add %offset, %14
        %16:vec3<u32> = %v.Load3 %15
        %17:vec3<f32> = bitcast %16
        store %13, %17
        continue  # -> $B6
      }
      $B6: {  # continuing
        %18:u32 = add %idx, 1u
        next_iteration %18  # -> $B5
      }
    }
    %19:array<vec3<f32>, 5> = load %a_1
    ret %19
  }
}
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest,
       DISABLED_UniformAccessArrayWhichCanHaveSizesOtherThenFive) {
    auto* var = b.Var<uniform, array<vec3<f32>, 42>, core::Access::kRead>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        b.Let("b", b.Load(b.Access(ty.ptr<storage, vec3<f32>, core::Access::kRead>(), var, 3_u)));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, array<vec3<f32>, 42>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:array<vec3<f32>, 42> = load %v
    %a:array<vec3<f32>, 42> = let %3
    %5:ptr<storage, vec3<f32>, read> = access %v, 3u
    %6:vec3<f32> = load %5
    %b:vec3<f32> = let %6
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:hlsl.byte_address_buffer<read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:array<vec3<f32>, 42> = call %4, 0u
    %a:array<vec3<f32>, 42> = let %3
    %6:vec3<u32> = %v.Load3 48u
    %7:vec3<f32> = bitcast %6
    %b:vec3<f32> = let %7
    ret
  }
}
%4 = func(%offset:u32):array<vec3<f32>, 42> {
  $B3: {
    %a_1:ptr<function, array<vec3<f32>, 42>, read_write> = var, array<vec3<f32>, 42>(vec3<f32>(0.0f))  # %a_1: 'a'
    loop [i: $B4, b: $B5, c: $B6] {  # loop_1
      $B4: {  # initializer
        next_iteration 0u  # -> $B5
      }
      $B5 (%idx:u32): {  # body
        %12:bool = gte %idx, 42u
        if %12 [t: $B7] {  # if_1
          $B7: {  # true
            exit_loop  # loop_1
          }
        }
        %13:ptr<function, vec3<f32>, read_write> = access %a_1, %idx
        %14:u32 = mul %idx, 16u
        %15:u32 = add %offset, %14
        %16:vec3<u32> = %v.Load3 %15
        %17:vec3<f32> = bitcast %16
        store %13, %17
        continue  # -> $B6
      }
      $B6: {  # continuing
        %18:u32 = add %idx, 1u
        next_iteration %18  # -> $B5
      }
    }
    %19:array<vec3<f32>, 42> = load %a_1
    ret %19
  }
}
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_UniformAccessStruct) {
    auto* SB = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), ty.f32()},
                                                });

    auto* var = b.Var("v", uniform, SB, core::Access::kRead);
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        b.Let("b", b.Load(b.Access(ty.ptr<storage, f32, core::Access::kRead>(), var, 1_u)));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(4) {
  a:i32 @offset(0)
  b:f32 @offset(4)
}

$B1: {  # root
  %v:ptr<storage, SB, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:SB = load %v
    %a:SB = let %3
    %5:ptr<storage, f32, read> = access %v, 1u
    %6:f32 = load %5
    %b:f32 = let %6
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
SB = struct @align(4) {
  a:i32 @offset(0)
  b:f32 @offset(4)
}

$B1: {  # root
  %v:hlsl.byte_address_buffer<read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:SB = call %4, 0u
    %a:SB = let %3
    %6:u32 = %v.Load 4u
    %7:f32 = bitcast %6
    %b:f32 = let %7
    ret
  }
}
%4 = func(%offset:u32):SB {
  $B3: {
    %10:u32 = add %offset, 0u
    %11:u32 = %v.Load %10
    %12:i32 = bitcast %11
    %13:u32 = add %offset, 4u
    %14:u32 = %v.Load %13
    %15:f32 = bitcast %14
    %16:SB = construct %12, %15
    ret %16
  }
}
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_UniformAccessStructNested) {
    auto* Inner =
        ty.Struct(mod.symbols.New("Inner"), {
                                                {mod.symbols.New("s"), ty.mat3x3<f32>()},
                                                {mod.symbols.New("t"), ty.array<vec3<f32>, 5>()},
                                            });
    auto* Outer = ty.Struct(mod.symbols.New("Outer"), {
                                                          {mod.symbols.New("x"), ty.f32()},
                                                          {mod.symbols.New("y"), Inner},
                                                      });

    auto* SB = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), Outer},
                                                });

    auto* var = b.Var("v", uniform, SB, core::Access::kRead);
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        b.Let("b", b.LoadVectorElement(b.Access(ty.ptr<storage, vec3<f32>, core::Access::kRead>(),
                                                var, 1_u, 1_u, 1_u, 3_u),
                                       2_u));
        b.Return(func);
    });

    auto* src = R"(
Inner = struct @align(16) {
  s:mat3x3<f32> @offset(0)
  t:array<vec3<f32>, 5> @offset(48)
}

Outer = struct @align(16) {
  x:f32 @offset(0)
  y:Inner @offset(16)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Outer @offset(16)
}

$B1: {  # root
  %v:ptr<storage, SB, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:SB = load %v
    %a:SB = let %3
    %5:ptr<storage, vec3<f32>, read> = access %v, 1u, 1u, 1u, 3u
    %6:f32 = load_vector_element %5, 2u
    %b:f32 = let %6
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
Inner = struct @align(16) {
  s:mat3x3<f32> @offset(0)
  t:array<vec3<f32>, 5> @offset(48)
}

Outer = struct @align(16) {
  x:f32 @offset(0)
  y:Inner @offset(16)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Outer @offset(16)
}

$B1: {  # root
  %v:hlsl.byte_address_buffer<read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:SB = call %4, 0u
    %a:SB = let %3
    %6:u32 = %v.Load 136u
    %7:f32 = bitcast %6
    %b:f32 = let %7
    ret
  }
}
%4 = func(%offset:u32):SB {
  $B3: {
    %10:u32 = add %offset, 0u
    %11:u32 = %v.Load %10
    %12:i32 = bitcast %11
    %13:u32 = add %offset, 16u
    %14:Outer = call %15, %13
    %16:SB = construct %12, %14
    ret %16
  }
}
%15 = func(%offset_1:u32):Outer {  # %offset_1: 'offset'
  $B4: {
    %18:u32 = add %offset_1, 0u
    %19:u32 = %v.Load %18
    %20:f32 = bitcast %19
    %21:u32 = add %offset_1, 16u
    %22:Inner = call %23, %21
    %24:Outer = construct %20, %22
    ret %24
  }
}
%23 = func(%offset_2:u32):Inner {  # %offset_2: 'offset'
  $B5: {
    %26:u32 = add %offset_2, 0u
    %27:mat3x3<f32> = call %28, %26
    %29:u32 = add %offset_2, 48u
    %30:array<vec3<f32>, 5> = call %31, %29
    %32:Inner = construct %27, %30
    ret %32
  }
}
%28 = func(%offset_3:u32):mat3x3<f32> {  # %offset_3: 'offset'
  $B6: {
    %34:u32 = add %offset_3, 0u
    %35:vec3<u32> = %v.Load3 %34
    %36:vec3<f32> = bitcast %35
    %37:u32 = add %offset_3, 16u
    %38:vec3<u32> = %v.Load3 %37
    %39:vec3<f32> = bitcast %38
    %40:u32 = add %offset_3, 32u
    %41:vec3<u32> = %v.Load3 %40
    %42:vec3<f32> = bitcast %41
    %43:mat3x3<f32> = construct %36, %39, %42
    ret %43
  }
}
%31 = func(%offset_4:u32):array<vec3<f32>, 5> {  # %offset_4: 'offset'
  $B7: {
    %a_1:ptr<function, array<vec3<f32>, 5>, read_write> = var, array<vec3<f32>, 5>(vec3<f32>(0.0f))  # %a_1: 'a'
    loop [i: $B8, b: $B9, c: $B10] {  # loop_1
      $B8: {  # initializer
        next_iteration 0u  # -> $B9
      }
      $B9 (%idx:u32): {  # body
        %47:bool = gte %idx, 5u
        if %47 [t: $B11] {  # if_1
          $B11: {  # true
            exit_loop  # loop_1
          }
        }
        %48:ptr<function, vec3<f32>, read_write> = access %a_1, %idx
        %49:u32 = mul %idx, 16u
        %50:u32 = add %offset_4, %49
        %51:vec3<u32> = %v.Load3 %50
        %52:vec3<f32> = bitcast %51
        store %48, %52
        continue  # -> $B10
      }
      $B10: {  # continuing
        %53:u32 = add %idx, 1u
        next_iteration %53  # -> $B9
      }
    }
    %54:array<vec3<f32>, 5> = load %a_1
    ret %54
  }
}
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StorageAtomicStore) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", storage, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Call(ty.void_(), core::BuiltinFn::kAtomicStore,
               b.Access(ty.ptr<storage, atomic<i32>, read_write>(), var, 1_u), 123_i);
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, atomic<i32>, read_write> = access %v, 1u
    %4:void = atomicStore %3, 123i
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StorageAtomicLoad) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", storage, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicLoad,
                          b.Access(ty.ptr<storage, atomic<i32>, read_write>(), var, 1_u)));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, atomic<i32>, read_write> = access %v, 1u
    %4:i32 = atomicLoad %3
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StorageAtomicAdd) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", storage, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicAdd,
                          b.Access(ty.ptr<storage, atomic<i32>, read_write>(), var, 1_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, atomic<i32>, read_write> = access %v, 1u
    %4:i32 = atomicAdd %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StorageAtomicSub) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", storage, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicSub,
                          b.Access(ty.ptr<storage, atomic<i32>, read_write>(), var, 1_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, atomic<i32>, read_write> = access %v, 1u
    %4:i32 = atomicSub %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StorageAtomicMax) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", storage, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicMax,
                          b.Access(ty.ptr<storage, atomic<i32>, read_write>(), var, 1_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, atomic<i32>, read_write> = access %v, 1u
    %4:i32 = atomicMax %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StorageAtomicMin) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", storage, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicMin,
                          b.Access(ty.ptr<storage, atomic<i32>, read_write>(), var, 1_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, atomic<i32>, read_write> = access %v, 1u
    %4:i32 = atomicMin %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StorageAtomicAnd) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", storage, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicAnd,
                          b.Access(ty.ptr<storage, atomic<i32>, read_write>(), var, 1_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, atomic<i32>, read_write> = access %v, 1u
    %4:i32 = atomicAnd %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StorageAtomicOr) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", storage, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicOr,
                          b.Access(ty.ptr<storage, atomic<i32>, read_write>(), var, 1_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, atomic<i32>, read_write> = access %v, 1u
    %4:i32 = atomicOr %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StorageAtomicXor) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", storage, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicXor,
                          b.Access(ty.ptr<storage, atomic<i32>, read_write>(), var, 2_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, atomic<i32>, read_write> = access %v, 2u
    %4:i32 = atomicXor %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StorageAtomicExchange) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", storage, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicExchange,
                          b.Access(ty.ptr<storage, atomic<i32>, read_write>(), var, 2_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, atomic<i32>, read_write> = access %v, 2u
    %4:i32 = atomicExchange %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StorageAtomicCompareExchangeWeak) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* out = ty.Struct(
        mod.symbols.New("__atomic_compare_exchange_result"),
        {{mod.symbols.New("old_value"), ty.i32()}, {mod.symbols.New("exchanged"), ty.bool_()}});

    auto* var = b.Var("v", storage, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x",
              b.Call(out, core::BuiltinFn::kAtomicCompareExchangeWeak,
                     b.Access(ty.ptr<storage, atomic<i32>, read_write>(), var, 1_u), 123_i, 345_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}
__atomic_compare_exchange_result = struct @align(4) {
  old_value:i32 @offset(0)
  exchanged:bool @offset(4)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, atomic<i32>, read_write> = access %v, 1u
    %4:__atomic_compare_exchange_result = atomicCompareExchangeWeak %3, 123i, 345i
    %x:__atomic_compare_exchange_result = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_WorkgroupAtomicStore) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", workgroup, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Call(ty.void_(), core::BuiltinFn::kAtomicStore,
               b.Access(ty.ptr<workgroup, atomic<i32>, read_write>(), var, 1_u), 123_i);
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<workgroup, atomic<i32>, read_write> = access %v, 1u
    %4:void = atomicStore %3, 123i
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_WorkgroupAtomicLoad) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", workgroup, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicLoad,
                          b.Access(ty.ptr<workgroup, atomic<i32>, read_write>(), var, 1_u)));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<workgroup, atomic<i32>, read_write> = access %v, 1u
    %4:i32 = atomicLoad %3
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_WorkgroupAtomicAdd) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", workgroup, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicAdd,
                          b.Access(ty.ptr<workgroup, atomic<i32>, read_write>(), var, 1_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<workgroup, atomic<i32>, read_write> = access %v, 1u
    %4:i32 = atomicAdd %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_WorkgroupAtomicSub) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", workgroup, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicSub,
                          b.Access(ty.ptr<workgroup, atomic<i32>, read_write>(), var, 1_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<workgroup, atomic<i32>, read_write> = access %v, 1u
    %4:i32 = atomicSub %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_WorkgroupAtomicMax) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", workgroup, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicMax,
                          b.Access(ty.ptr<workgroup, atomic<i32>, read_write>(), var, 1_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<workgroup, atomic<i32>, read_write> = access %v, 1u
    %4:i32 = atomicMax %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_WorkgroupAtomicMin) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", workgroup, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicMin,
                          b.Access(ty.ptr<workgroup, atomic<i32>, read_write>(), var, 1_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<workgroup, atomic<i32>, read_write> = access %v, 1u
    %4:i32 = atomicMin %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_WorkgroupAtomicAnd) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", workgroup, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicAnd,
                          b.Access(ty.ptr<workgroup, atomic<i32>, read_write>(), var, 1_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<workgroup, atomic<i32>, read_write> = access %v, 1u
    %4:i32 = atomicAnd %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_WorkgroupAtomicOr) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", workgroup, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicOr,
                          b.Access(ty.ptr<workgroup, atomic<i32>, read_write>(), var, 1_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<workgroup, atomic<i32>, read_write> = access %v, 1u
    %4:i32 = atomicOr %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_WorkgroupAtomicXor) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", workgroup, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicXor,
                          b.Access(ty.ptr<workgroup, atomic<i32>, read_write>(), var, 2_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<workgroup, atomic<i32>, read_write> = access %v, 2u
    %4:i32 = atomicXor %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_WorkgroupAtomicExchange) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* var = b.Var("v", workgroup, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.i32(), core::BuiltinFn::kAtomicExchange,
                          b.Access(ty.ptr<workgroup, atomic<i32>, read_write>(), var, 2_u), 123_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<workgroup, atomic<i32>, read_write> = access %v, 2u
    %4:i32 = atomicExchange %3, 123i
    %x:i32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_WorkgroupAtomicCompareExchangeWeak) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("padding"), ty.vec4<f32>()},
                                                    {mod.symbols.New("a"), ty.atomic<i32>()},
                                                    {mod.symbols.New("b"), ty.atomic<u32>()},
                                                });

    auto* out = ty.Struct(
        mod.symbols.New("__atomic_compare_exchange_result"),
        {{mod.symbols.New("old_value"), ty.i32()}, {mod.symbols.New("exchanged"), ty.bool_()}});

    auto* var = b.Var("v", workgroup, sb, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(out, core::BuiltinFn::kAtomicCompareExchangeWeak,
                          b.Access(ty.ptr<workgroup, atomic<i32>, read_write>(), var, 1_u), 123_i,
                          345_i));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  padding:vec4<f32> @offset(0)
  a:atomic<i32> @offset(16)
  b:atomic<u32> @offset(20)
}
__atomic_compare_exchange_result = struct @align(4) {
  old_value:i32 @offset(0)
  exchanged:bool @offset(4)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<workgroup, atomic<i32>, read_write> = access %v, 1u
    %4:__atomic_compare_exchange_result = atomicCompareExchangeWeak %3, 123i, 345i
    %x:__atomic_compare_exchange_result = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, StoreVecF32) {
    auto* var = b.Var<storage, vec4<f32>, core::Access::kReadWrite>("v");
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.StoreVectorElement(var, 0_u, 2_f);
        b.StoreVectorElement(var, 1_u, 4_f);
        b.StoreVectorElement(var, 2_u, 8_f);
        b.StoreVectorElement(var, 3_u, 16_f);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, vec4<f32>, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    store_vector_element %v, 0u, 2.0f
    store_vector_element %v, 1u, 4.0f
    store_vector_element %v, 2u, 8.0f
    store_vector_element %v, 3u, 16.0f
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:hlsl.byte_address_buffer<read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:u32 = bitcast 2.0f
    %4:void = %v.Store 0u, %3
    %5:u32 = bitcast 4.0f
    %6:void = %v.Store 4u, %5
    %7:u32 = bitcast 8.0f
    %8:void = %v.Store 8u, %7
    %9:u32 = bitcast 16.0f
    %10:void = %v.Store 12u, %9
    ret
  }
}
)";

    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, StoreVecF16) {
    auto* var = b.Var<storage, vec4<f16>, core::Access::kReadWrite>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.StoreVectorElement(var, 0_u, 1_h);
        b.StoreVectorElement(var, 1_u, 2_h);
        b.StoreVectorElement(var, 2_u, 3_h);
        b.StoreVectorElement(var, 3_u, 4_h);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, vec4<f16>, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    store_vector_element %v, 0u, 1.0h
    store_vector_element %v, 1u, 2.0h
    store_vector_element %v, 2u, 3.0h
    store_vector_element %v, 3u, 4.0h
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:hlsl.byte_address_buffer<read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:u32 = bitcast 1.0h
    %4:void = %v.Store 0u, %3
    %5:u32 = bitcast 2.0h
    %6:void = %v.Store 2u, %5
    %7:u32 = bitcast 3.0h
    %8:void = %v.Store 4u, %7
    %9:u32 = bitcast 4.0h
    %10:void = %v.Store 6u, %9
    ret
  }
}
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StoreMatrixElement) {
    auto* var = b.Var<storage, mat4x4<f32>, core::Access::kReadWrite>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.StoreVectorElement(
            b.Access(ty.ptr<storage, vec4<f32>, core::Access::kReadWrite>(), var, 1_u), 2_u, 5_f);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, mat4x4<f32>, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, vec4<f32>, read_write> = access %v, 1u
    store_vector_element %3, 2u, 5.0f
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StoreMatrixColumn) {
    auto* var = b.Var<storage, mat4x4<f32>, core::Access::kReadWrite>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Store(b.Access(ty.ptr<storage, vec4<f32>, core::Access::kReadWrite>(), var, 1_u),
                b.Splat<vec4<f32>>(5_f));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, mat4x4<f32>, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, vec4<f32>, read_write> = access %v, 1u
    store %3, vec4<f32>(5.0f)
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StoreMatrix) {
    auto* var = b.Var<storage, mat4x4<f32>, core::Access::kReadWrite>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Store(var, b.Zero<mat4x4<f32>>());
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, mat4x4<f32>, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    store %v, mat4x4<f32>(vec4<f32>(0.0f))
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StoreArrayElement) {
    auto* var = b.Var<storage, array<f32, 5>, core::Access::kReadWrite>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Store(b.Access(ty.ptr<storage, f32, core::Access::kReadWrite>(), var, 3_u), 1_f);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, array<f32, 5>, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, f32, read_write> = access %v, 3u
    store %3, 1.0f
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StoreArray) {
    auto* var = b.Var<storage, array<vec3<f32>, 5>, core::Access::kRead>("v");
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* ary = b.Let("ary", b.Zero<array<vec3<f32>, 5>>());
        b.Store(var, ary);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<storage, array<vec3<f32>, 5>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %ary:array<vec3<f32>, 5> = let array<vec3<f32>, 5>(vec3<f32>(0.0f))
    store %v, %ary
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StoreStructMember) {
    auto* SB = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), ty.f32()},
                                                });

    auto* var = b.Var("v", storage, SB, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Store(b.Access(ty.ptr<storage, f32, core::Access::kReadWrite>(), var, 1_u), 3_f);
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(4) {
  a:i32 @offset(0)
  b:f32 @offset(4)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, f32, read_write> = access %v, 1u
    store %3, 3.0f
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StoreStructNested) {
    auto* Inner =
        ty.Struct(mod.symbols.New("Inner"), {
                                                {mod.symbols.New("s"), ty.mat3x3<f32>()},
                                                {mod.symbols.New("t"), ty.array<vec3<f32>, 5>()},
                                            });
    auto* Outer = ty.Struct(mod.symbols.New("Outer"), {
                                                          {mod.symbols.New("x"), ty.f32()},
                                                          {mod.symbols.New("y"), Inner},
                                                      });

    auto* SB = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), Outer},
                                                });

    auto* var = b.Var("v", storage, SB, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Store(b.Access(ty.ptr<storage, f32, core::Access::kReadWrite>(), var, 1_u, 0_u), 2_f);
        b.Return(func);
    });

    auto* src = R"(
Inner = struct @align(16) {
  s:mat3x3<f32> @offset(0)
  t:array<vec3<f32>, 5> @offset(48)
}

Outer = struct @align(16) {
  x:f32 @offset(0)
  y:Inner @offset(16)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Outer @offset(16)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:ptr<storage, f32, read_write> = access %v, 1u, 0u
    store %3, 2.0f
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterDecomposeMemoryAccessTest, DISABLED_StoreStruct) {
    auto* Inner =
        ty.Struct(mod.symbols.New("Inner"), {
                                                {mod.symbols.New("s"), ty.mat3x3<f32>()},
                                                {mod.symbols.New("t"), ty.array<vec3<f32>, 5>()},
                                            });
    auto* Outer = ty.Struct(mod.symbols.New("Outer"), {
                                                          {mod.symbols.New("x"), ty.f32()},
                                                          {mod.symbols.New("y"), Inner},
                                                      });

    auto* SB = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), Outer},
                                                });

    auto* var = b.Var("v", storage, SB, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);

    b.ir.root_block->Append(var);
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* s = b.Let("s", b.Zero(SB));
        b.Store(var, s);
        b.Return(func);
    });

    auto* src = R"(
Inner = struct @align(16) {
  s:mat3x3<f32> @offset(0)
  t:array<vec3<f32>, 5> @offset(48)
}

Outer = struct @align(16) {
  x:f32 @offset(0)
  y:Inner @offset(16)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Outer @offset(16)
}

$B1: {  # root
  %v:ptr<storage, SB, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %s:SB = let SB(0i, Outer(0.0f, Inner(mat3x3<f32>(vec3<f32>(0.0f)), array<vec3<f32>, 5>(vec3<f32>(0.0f)))))
    store %v, %s
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
)";
    Run(DecomposeMemoryAccess);
    EXPECT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::hlsl::writer::raise
