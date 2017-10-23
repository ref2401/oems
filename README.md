# Odd Even Merge Sort
```Odd even merge sort``` sorting network for 32 elements is represented on the picture below.

![Algorithm Layout](https://user-images.githubusercontent.com/10673999/31888388-8553d25e-b804-11e7-8a48-20635e028d04.png)

Such network has ```C``` columns each of which has ```Nc``` number of comparisons. Columns must be processed sequentially but comparisons inside a column can be processed in any order. 

The algorithm designed with assumption that thread sync points are worse than extra computations. That means that each thread generates a sorting network column descriptor, decides which part of the column it should process and processes the part. After the thread has completed processing it's part it's waiting until all other threads finish the current column.

To achieve best performance for the specified number of elements one should poke compute shader's ```c_thread_count``` constant. Possible values lay inside the range [1, 1024]. Number of elements and number of threads must be 2 to the power of ```K```.
I did not bother with dx11 timeout so if your device has been removed just lower the number of elements. On my PC the maximum number is 4 * 1024 * 1024 before timeout kicks in.

# References
- https://bekbolatov.github.io/sorting/
- http://www.iti.fh-flensburg.de/lang/algorithmen/sortieren/networks/oemen.htm

