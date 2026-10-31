# Z8 engine and trigger clocks

The Z8 trigger interpreter takes one native scheduler frame per call. Its
pass counter, 16-frame elapsed-seconds counter, and 42 ms Fastest speed-table
wait decrement remain in that domain. The runtime maps the project's nominal
30 Hz engine policy to its 24 Hz source-clock policy by calling the interpreter
on four of every five active engine updates. This is a pacing policy, not a
claim that physical hardware or the original game runs at an exact rate.
Scripted AI timers and cloak updates continue on every engine update.

The existing saved engine frame determines the clock phase: frames with residue one modulo five skip the interpreter. Z8 starts at frame zero, increments the frame
once before each active mission update, and does not increment while paused.
Loading an older save uses its existing frame residue for the next step and
preserves its saved scheduler state; it does not rewind prior timing. No save
bytes or tag are reinterpreted. Camera completion remains latched until the
next scheduler step; the existing save preflight refuses a save while that
transient latch is set.
