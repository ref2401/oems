# oems
Odd Even Merge Sort on GPU.

https://bekbolatov.github.io/sorting/
http://www.iti.fh-flensburg.de/lang/algorithmen/sortieren/networks/oemen.htm

The algorithm designed with assumption that thread sync points are worse then extra computations.
That means that each pass's params such as ... are computed over and over again by each thread.


Find out how many rows are in the current column and how many comparisons have to be made for each row.
Now we can calculate the total number of comparisons have to be made for

pass, rows, columns, inner_pass.
first & third passes have 1 column, the second pass has varying number of columns
comparison indices inside a column

to achieve best performance one should poke compute shader's c_thread_count constant 