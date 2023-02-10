# Merge-Sort
This is an implementation of the Merge Sort algorithm using MPI (Message Passing Interface) for parallel processing. The implementation is written in C language.

## Usage
The implementation uses some macros that can be modified by the user to adjust its behavior. These macros are defined at the beginning of the code and are:

```bash
mpicc merge-sort.c
mpiexec -n <number of processes> ./a.out
```

## Functionality
The code follows the merge sort algorithm, sorting the elements in an array using the divide and conquer technique. The MPI is used to divide the processing of sorting the elements into multiple processes.

The code uses the recursive_merge_sort function to implement the merge sort algorithm. This function takes the following arguments:

numbers: the array of numbers to be sorted.
begin: the starting index of the portion of the array to be sorted.
end: the ending index of the portion of the array to be sorted.
tmp: a temporary array used during the sorting process.
The code also implements a merge function that merges two sorted arrays into one sorted array.

## Conclusion
This implementation of Merge Sort provides a parallel implementation of the algorithm using MPI for parallel processing. The code can be easily modified to fit the specific needs of the user by adjusting the macros defined at the beginning of the code.