Design note: Strudel as inspiration for Flowcore orchestration

Strudel is worth examining later as inspiration, not as something to copy directly.

The interesting part is its model of declarative orchestration: the user declares patterns, transformations, layers, routing, and timing relationships, while the runtime owns synchronization through a shared clock.

For Flowcore, the analogous idea is not musical time, but flow/dependency/event time:

Strudel:
  patterns + transformations + global clock -> synchronized music

Flowcore:
  nodes + wires + contracts + scheduler -> synchronized computation

Concepts worth revisiting:

Strudel stack(...)        -> parallel composition / grouped concurrent flow
cycle/repetition frame    -> execution frame / recurring flow window
pattern transformation    -> node/pipeline transformation
parameter pattern         -> policy/context/envelope modulation
orbit/channel routing     -> lane/sink/output routing

Open naming question:

stack is a good name in Strudel because musical layers are literally stacked. For Flowcore, the name may be misleading unless the construct really means “layer these flows together.”

Possible names to compare:

stack       = layered coexistence, good if visual/audio/data layers matter
parallel    = very explicit, but may overpromise actual simultaneous execution
par         = terse, familiar from parallel/dataflow languages
group       = neutral, but semantically weak
bundle      = suggests several flows carried together
fanout      = good for one-to-many, not general parallel composition
join        = good for convergence, not general parallel composition
fork        = good for splitting, not coexistence

Tentative conclusion:

Use parallel or par when the construct means “these branches are independent and may execute concurrently.”

Use stack only if the intended meaning is closer to “these branches coexist in the same orchestration frame and their outputs are layered/combined.”

Important distinction:

parallel = scheduling/execution possibility
stack    = structural/orchestration grouping

Flowcore should avoid choosing a name that smuggles in stronger semantics than the construct actually guarantees.
