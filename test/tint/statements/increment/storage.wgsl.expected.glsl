SKIP: FAILED


@group(0) @binding(0) var<storage, read_write> i : u32;

fn main() {
  i++;
}

Failed to generate: error: unknown statement type: tint::ast::IncrementDecrementStatement
