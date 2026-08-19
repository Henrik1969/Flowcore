# Chapter 1 — A Program Is a Flow

FlowMini programs are built from named values, expressions, and destinations.
The language makes the movement of a computed value visible:

```flow
expression -> destination
```

That small arrow is a good place to begin.

## Your first program

```flow
program hello

main {
    answer : int(40)
    answer + 2 -> answer
    print answer
}
```

Read the program from top to bottom.

`program hello` names the executable source unit. `main` is the entrypoint of
the simple, single-product form. Inside the block, `answer : int(40)` creates a
typed value and initializes it. The next line evaluates `answer + 2` and places
the result back into `answer`. The final line uses the current output spelling
to display the value.

The result is `42`.

Notice what the example does not hide. There is no implicit declaration, no
untyped assignment, and no mysterious destination chosen by the expression.
The value and its destination are both visible.

## Programs and units

A FlowMini source unit has one of two roles:

```flow
program application
unit math_helpers
```

A `program` is an executable root and may contain `main`. A `unit` is a
defining, importable source unit and must not contain `main`.

A unit can provide reusable functions:

```flow
unit math_helpers

fn square(n : int): int {
    n * n -> return
}
```

A program imports a unit by path:

```flow
import "../../std/math.flow"

program calculate

main {
    value : int(7)
    result : int(0)
    square(value) -> result
    print result
}
```

The program is the thing that owns an executable entrypoint. The unit is the
thing that supplies reusable definitions.

## One program, several products

The simple form has one root `main`. FlowMini also has a named-target model for
describing several products from one shared source universe:

```flow
program toolset

fn banner(value : string): string {
    value -> return
}

target cli {
    main {
        message : string(banner("command line"))
        print message
    }
}

target daemon {
    main {
        message : string(banner("service"))
        print message
    }
}
```

The meaning is structural:

```text
program = shared semantic universe
target  = named buildable or runnable projection
main    = entrypoint owned by a target
```

The names matter. `cli` and `daemon` are not two anonymous guesses at what a
program should execute; they are two named products. Each target owns one
entrypoint. Shared declarations, such as `banner`, can participate in more
than one target projection. Target-local declarations belong to that target.

This model lets one FlowMini source base describe related tools, services, or
other products without copying their shared language and domain definitions.

## Following a value

Suppose a program reads two values and chooses the larger one:

```flow
program larger_demo

fn larger(a : int, b : int): int {
    if a > b {
        a -> return
    } else {
        b -> return
    }
}

main {
    first : int(4)
    second : int(9)
    result : int(0)

    larger(first, second) -> result
    print result
}
```

The function call produces a value. The arrow places that value into `result`.
Inside `larger`, each branch sends its selected value to `return`. The same
idea appears at every scale: an expression produces something, and a named
destination gives that result a role.

## A small exercise

Write a program that:

1. declares a list containing `4`, `9`, and `2`;
2. reads the first two elements into typed integer values;
3. calls a function that returns the larger value; and
4. prints the result.

The essential shapes are:

```flow
values : list<int>([4, 9, 2])
first : int(0)
second : int(0)
result : int(0)

values[0] -> first
values[1] -> second
larger(first, second) -> result
print result
```

Once this reads naturally, you have learned the first FlowMini habit: follow
the value, then identify its destination.
