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

#include "src/ast/expression.h"

namespace tint {
namespace ast {

Expression::Expression() = default;

Expression::Expression(const Source& source) : Base(source) {}

Expression::Expression(Expression&&) = default;

Expression::~Expression() = default;

void Expression::set_result_type(type::Type* type) {
  // The expression result should never be an alias or access-controlled type
  result_type_ = type->UnwrapIfNeeded();
}

}  // namespace ast
}  // namespace tint
