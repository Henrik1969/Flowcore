Design note: Strudel-style grouping syntax as Flowcore inspiration

Strudel's bracket/group syntax is worth examining as inspiration for Flowcore orchestration syntax.

Example pattern form:

[a b c [a b] [a,b]]

The interesting lesson is not the exact syntax, but the way compact grouping can encode structure:

[a b c]      -> sequence within a frame
[a,b,c]      -> simultaneity / coexistence / parallel layering
[a [b c] d]  -> nested subframe
a*4          -> repetition / density / expansion
a?           -> optional/probabilistic/event-gated occurrence

For Flowcore, this suggests that grouping syntax could express nested execution/orchestration frames, not merely lists.

Possible semantic mappings to examine:

Strudel sequence grouping       -> Flowcore sequence frame
Strudel comma simultaneity      -> Flowcore parallel/coexisting frame
Strudel nested brackets         -> Flowcore nested orchestration subframe
Strudel pattern modifiers       -> Flowcore policy/scheduler/context modifiers
Strudel cycle frame             -> Flowcore execution/dependency/event frame

However, Flowcore should not simply import Strudel's bracket syntax. Music-pattern syntax can be dense because the whole domain is temporal patterning. Flowcore has stricter obligations: endpoints, ports, wires, effects, resource constraints, joins, and continuation semantics must remain clear.

Any compact grouping sugar must desugar into explicit core constructs such as:

frame {
    sequence {
        a => b

        parallel {
            c => d
            e => f
        }

        join => g
    }
}

Open questions for any grouped syntax:

Is the group ordered, parallel, or merely bundled?
Does the group have one input, many inputs, or no input?
Does the group produce one output, many outputs, or no output?
Is there an implicit join before continuation?
Can the scheduler reorder branches?
What effects/resources are declared inside the group?
Can continuation after the group be proven well-defined?

Governing rule:

Sugar may remove typing, but must not remove meaning.

So Strudel's syntax is valuable as inspiration for compact nested orchestration, but Flowcore must preserve explicit semantic information.
