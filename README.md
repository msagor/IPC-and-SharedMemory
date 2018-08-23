# IPC-and-SharedMemory
A parent creates a child. The child takes two matrices as input and puts them in a shared memory. Parent computes the multiplication for the matices in a multi-threaded fashion and puts the resultant matrix in the shared memory. Child prints the resultant matrix. Later, child dies, parent takes over, destroyes the shared memory and dies too!
