# Structured examples

These examples use the v6 human-facing structured syntax:

- `main { ... }`
- initialized declarations only
- `while` blocks
- expression placement: `expr -> target`
- `print expr`
- no implicit shadowing
- no undeclared assignment

## Quick checks

```bash
echo 5   | ./build/flowmini examples/factorial_structured.flow       # 120
echo 100 | ./build/flowmini examples/sum_1_to_n_structured.flow      # 5050
echo 10  | ./build/flowmini examples/power2_structured.flow          # 1024
echo 10  | ./build/flowmini examples/fibonacci_structured.flow       # 55
echo 462 | ./build/flowmini examples/gcd_structured.flow             # 21
```
