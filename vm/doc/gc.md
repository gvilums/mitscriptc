# Garbage Collection

## Note
Due to custom requirements, the allocator implemented in gc/gc.h is completely separate from the allocator acutally used in the vm. The documentation below is focused on the implementation in the vm, but the same principles apply to both designs.

## Design
The general design of the garbage collector follows the principle of simple mark-and-sweep collectors. Allocations are handled by the VirtualMachine object executing the code.

### HeapObject
The HeapObject class represents objects which can be allocated on the heap. Each HeapObject has a marked bit and a pointer to the next heap object. Additionally, it implements a `trace` function, which recursively traces objects that an object references. HeapObject is implemented as a tagged union, as all allocated types are known in advance and there were some very weird issues when trying to implement `Collectable` for the value structure.

### TrackingAlloc
To enable the use of STL containers such as vectors, strings, and maps, a custom allocator is implemented in allocator.h. Whenever it receives a request for allocation, it increments the global variable total_alloc by the size of the allocation. This information is then used by the virtual machine to decide when to run garbage collection.

### Memory Usage Monitoring
After each executed instruction, the vm estimates the amount of memory that is currently used. The exact amount of allocated usable memory is always available, as both the vm and TrackingAllocator keep track of the exact amount of memory used. There is however an additional overhead associated with every heap allocation which cannot be tracked exactly. Therefore, the garbage collector is conservative and executes a collection cycle even when only half the available memory is thought to be used.

### Collection
The collection algorithm is completely straightforward: First, all objects which are on the stack are traced. Then, starting at the head element of the allocation list, all allocations are traversed and unmarked objects are deallocated.