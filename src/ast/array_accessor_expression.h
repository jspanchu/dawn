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

#ifndef SRC_AST_ARRAY_ACCESSOR_EXPRESSION_H_
#define SRC_AST_ARRAY_ACCESSOR_EXPRESSION_H_

#include <memory>
#include <utility>

#include "src/ast/expression.h"
#include "src/ast/literal.h"

namespace tint {
namespace ast {

/// An array accessor expression
class ArrayAccessorExpression
    : public Castable<ArrayAccessorExpression, Expression> {
 public:
  /// Constructor
  ArrayAccessorExpression();
  /// Constructor
  /// @param array the array
  /// @param idx_expr the index expression
  ArrayAccessorExpression(Expression* array, Expression* idx_expr);
  /// Constructor
  /// @param source the array accessor source
  /// @param array the array
  /// @param idx_expr the index expression
  ArrayAccessorExpression(const Source& source,
                          Expression* array,
                          Expression* idx_expr);
  /// Move constructor
  ArrayAccessorExpression(ArrayAccessorExpression&&);
  ~ArrayAccessorExpression() override;

  /// Sets the array
  /// @param array the array
  void set_array(Expression* array) { array_ = array; }
  /// @returns the array
  Expression* array() const { return array_; }

  /// Sets the index expression
  /// @param idx_expr the index expression
  void set_idx_expr(Expression* idx_expr) { idx_expr_ = idx_expr; }
  /// @returns the index expression
  Expression* idx_expr() const { return idx_expr_; }

  /// @returns true if the node is valid
  bool IsValid() const override;

  /// Writes a representation of the node to the output stream
  /// @param out the stream to write to
  /// @param indent number of spaces to indent the node when writing
  void to_str(std::ostream& out, size_t indent) const override;

 private:
  ArrayAccessorExpression(const ArrayAccessorExpression&) = delete;

  Expression* array_ = nullptr;
  Expression* idx_expr_ = nullptr;
};

}  // namespace ast
}  // namespace tint

#endif  // SRC_AST_ARRAY_ACCESSOR_EXPRESSION_H_
