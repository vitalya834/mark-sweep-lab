# Collector design

## Heap model

Every managed object belongs to one `GcVm`. Objects form an intrusive linked
list so the collector can visit the complete heap without a separate index.
Pairs contain two managed edges; integers and strings contain no managed
edges.

The VM root stack is the collector's trust boundary. A pointer held only in a
local C variable is not a root. `GcRootScope` records the current stack size
and restores it when a temporary allocation scope ends.

## Collection cycle

Collection has three phases:

1. Clear stale mark bits on every heap object.
2. Mark roots and repeatedly propagate marks through pair edges until no new
   object becomes reachable.
3. Sweep the heap list, freeing unmarked objects and clearing weak references
   that point to them.

Mark propagation is iterative. Deep graphs therefore do not overflow the C
call stack. The implementation favors clarity over asymptotic speed: repeated
heap scans make the worst case quadratic. A future implementation can replace
this phase with an explicit gray worklist while preserving the public API.

## Automatic collection

Allocation triggers collection when the live object count reaches the current
threshold. After a collection, the next threshold becomes twice the live heap
size, but never less than the configured minimum.

`GcStats` exposes allocation, collection, and reclamation totals for demos,
tests, and embedding diagnostics.

## Weak references

A weak reference is registered with its VM but is not marked as a root. During
sweep, references to an object are cleared immediately before that object is
freed. Weak-reference handles themselves must be released with
`gc_weak_ref_free`; any handles still registered are released with the VM.

## Deliberate limitations

- Single-threaded VM and API.
- Stop-the-world collection.
- No compaction, generations, or concurrent marking.
- No interior pointers.
- No custom allocator hooks.

These constraints keep the code useful as a readable base for experiments.
