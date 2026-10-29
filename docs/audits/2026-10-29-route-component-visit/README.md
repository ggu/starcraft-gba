# Route component hints with visit tracking

## Change

`SC64_REACHABLE_FORMATION` may use completed route fields for component-based
reachability even when `SC64_ROUTE_VISIT_GRID` is enabled. Partial fields remain
excluded because they do not describe the whole map. Product feature defaults
remain unchanged; the visit-tracking flag is still off unless a product opts
into it.

The public header change only removes the `VISIT_GRID` exclusion from this
guard. The measured private header had the same executable change; its only
difference from the final public header was a comment correction.

## Paired 55k measurement

The bounded canonical Zerg Z7 replay used source-release commit
`79ef4054991b5a7d8c734361c15786adad8417e3`, the same captured input tape and
runtime feature vector in both builds, with `SC64_ROUTE_VISIT_GRID` as the
candidate's sole feature define. The canonical direct-firing helper was on and
range-work was off in both builds. This was an asset-prepared replay, not a
literal Make-built product ROM.

| Build | ROM SHA-256 | Route flags |
| --- | --- | --- |
| Baseline | `7d985d15c5415a140a70858ef33272ef47fe57ed60b8fdaadcfa3b71247ef491` | Captured canonical Z7 flags; visit grid off |
| Candidate | `a6961444065d52c927eb718be9f9835e70050f8cea43f170cb331e88e08d1302` | Same flags plus `SC64_ROUTE_VISIT_GRID` |

Both runs matched the same 55,000-tick prefix: 137 trace rows, 3 world rows,
3 pixel rows, and 5,627 ordinary input events. Across the same 53,819 sampled
calls with an advancing frame counter, mean cycles changed by −0.35%, maximum
update cycles by −6.40%, and maximum combined update/audio/render cycles by
−6.14%. Calls following telemetry output were excluded from these samples.

`make test-sc64-route-component-visit` checks the component queries against an
independent graph oracle and checks exact walkability-grid restoration in byte
and packed-grid configurations under ASan and UBSan.

## Limits

This evidence covers only the measured 55k prefix. It does not establish a
complete-mission result, SRAM-load behavior, hardware performance, a frame-rate
target, or product activation. No product feature defaults were promoted. A
separate partial-flood candidate measured −1.58% mean but regressed its worst
combined call by 60.8%; a recency replacement policy increased visited-cell work by
32.92%. Those alternatives were rejected and are not part of this change.
