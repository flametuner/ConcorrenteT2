// V. Freitas [2018] @ ECL-UFSC
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <mpi.h>

/***
 * Todas as Macros pré-definidas devem ser recebidas como parâmetros de
 * execução da sua implementação paralela!!
 ***/
#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef NELEMENTS
#define NELEMENTS 100
#endif

#ifndef MAXVAL
#define MAXVAL 255
#endif // MAX_VAL


int rank;
int power = 0;
int maxDivisions = 0;
int divisions = 0;
int *ordenado;
/*
 * More info on: http://en.cppreference.com/w/c/language/variadic
 */
void debug(const char* msg, ...) {
	if (DEBUG > 2) {
		va_list args;
		va_start(args, msg);
		vprintf(msg, args);
		va_end(args);
	}
}

/*
 * Orderly merges two int arrays (numbers[begin..middle] and numbers[middle..end]) into one (sorted).
 * \retval: merged array -> sorted
 */
 void merge(int* numbers, int begin, int middle, int end, int * tmp, int * sorted) {
	int i, j;
	i = begin; j = middle;
	debug("Merging. Begin: %d, Middle: %d, End: %d\n", begin, middle, end);
	for (int k = begin; k < end; ++k) {
		debug("LHS[%d]: %d, RHS[%d]: %d\n", i, numbers[i], j, numbers[j]);
		if (i < middle && (j >= end || numbers[i] < numbers[j])) {
			tmp[k] = numbers[i];
			//printf("Merge Rank %d\n", rank);
			i++;
		} else {
			tmp[k] = numbers[j];
			j++;
		}
	}

	for(int i = begin; i < end; i++) {
		sorted[i] = tmp[i];
	}

}

/*
 * Merge sort recursive step
 */
void recursive_merge_sort(int* numbers, int begin, int end, int* tmp) {
	if (end - begin < 2)
		return;
	else {
		int middle = (begin + end)/2;

		int arraySize = middle - begin;

		int* copy = malloc(sizeof(int) * arraySize);

		int to = rank + pow(2, power++);
	  int receive = 0;
	  if(divisions < maxDivisions) {

			for(int i = 0; i < arraySize; i++) {
				copy[i] = numbers[begin + i];
			}

	    MPI_Send(&arraySize, 1, MPI_INT, to, 0, MPI_COMM_WORLD);
			MPI_Send(copy, arraySize, MPI_INT, to, 0, MPI_COMM_WORLD);
	    receive = 1;
	    divisions++;
	  }
		// Deixando rodando metade
		recursive_merge_sort(numbers, middle, end, tmp);
		if(receive) {
			MPI_Status st;
			MPI_Recv(copy, arraySize, MPI_INT, to, MPI_ANY_TAG, MPI_COMM_WORLD, &st); // Recebe o array ordenado
			for(int i = 0; i < arraySize; i++) {
				 numbers[begin + i] = copy[i];
			}
		} else {
			recursive_merge_sort(numbers, begin, middle, tmp);
		}
		merge(numbers, begin, middle, end, tmp, numbers);
		free(copy);
	}
}

void print_array(int* array, int size) {
	printf("Printing Array:\n");
	for (int i = 0; i < size; ++i) {
		printf("%d. ", array[i]);
	}
	printf("\n");
}

// First Merge Sort call
void merge_sort(int * numbers, int size, int * tmp) {
	recursive_merge_sort(numbers, 0, size, tmp);
}

void populate_array(int* array, int size, int max) {
	int m = max+1;
	for (int i = 0; i < size; ++i) {
		array[i] = rand()%m;
	}
}

int main (int argc, char ** argv) {
	MPI_Init(&argc, &argv);

	MPI_Status st;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int bitSize = 31 - __builtin_clz(size - 1); // Ex: 5 : 0100 (size - 1) = 2
  int value = ((pow(2, bitSize) * 2) - 1); // Ex 5: 0111 Se cria a mascara

  if(rank > 0) {
    int bitMax = 31 - __builtin_clz(rank & value);
    power = (bitMax + 1);

    maxDivisions = bitSize - bitMax;
    double lo = log2(size);
    int loi = lo;
    if(lo - loi > 0) {
      if(rank >= size + (value >> 1) - value)
        maxDivisions -= 1;
    }
  } else {
		if(size > 1)
    	maxDivisions = bitSize + 1;
  }

	MPI_Barrier(MPI_COMM_WORLD);

	if (rank == 0) {
		int seed, max_val;
		int * sortableA;
		int * tmpA;
		int * sortableB;
		int * tmpB;
		int * sortableC;
		int * tmpC;
		size_t arr_sizeA;
		size_t arr_sizeB;
		size_t arr_sizeC = arr_sizeA+arr_sizeB;

		switch (argc) {
			case 1:
				seed = time(NULL);
				arr_sizeA = NELEMENTS;
				arr_sizeB = NELEMENTS;
				arr_sizeC = 2*NELEMENTS;
				max_val = MAXVAL;
				break;
			case 2:
				seed = atoi(argv[1]);
				arr_sizeB = NELEMENTS;
				arr_sizeA = NELEMENTS;
				arr_sizeC = 2*NELEMENTS;
				max_val = MAXVAL;
				break;
			case 3:
				seed = atoi(argv[1]);
				arr_sizeA = atoi(argv[2]);
				arr_sizeB = atoi(argv[2]);
				arr_sizeC = 2*atoi(argv[2]);
				max_val = MAXVAL;
				break;
			case 4:
				seed = atoi(argv[1]);
				arr_sizeA = atoi(argv[2]);
				arr_sizeB = atoi(argv[2]);
				arr_sizeC = 2*atoi(argv[2]);
				max_val = atoi(argv[3]);
				break;
			default:
				printf("Too many arguments\n");
				break;
		}

		srand(seed);
		sortableA = malloc(arr_sizeA*sizeof(int));
		tmpA 	 = malloc(arr_sizeA*sizeof(int));

		sortableB = malloc(arr_sizeB*sizeof(int));
		tmpB 	 = malloc(arr_sizeB*sizeof(int));

		sortableC = malloc(arr_sizeC*sizeof(int));
		tmpC 	 = malloc(arr_sizeC*sizeof(int));

		populate_array(sortableA, arr_sizeA, max_val);
		populate_array(sortableB, arr_sizeB, max_val);
		populate_array(sortableC, arr_sizeC, max_val);

		tmpA = memcpy(tmpA, sortableA, arr_sizeA*sizeof(int));
		tmpB = memcpy(tmpB, sortableB, arr_sizeB*sizeof(int));
	  tmpC = memcpy(tmpC, sortableC, arr_sizeC*sizeof(int));

		for (int i = 0; i < arr_sizeA; i++) {
			sortableC[i] = sortableA[i];
		}
		for (int j = arr_sizeA; j < arr_sizeC; j++) {
			sortableC[j] = sortableB[j-arr_sizeB];
		}
		printf("Array A: ");
		print_array(sortableA, arr_sizeA);
		printf("Array B: ");
		print_array(sortableB, arr_sizeB);
		printf("Uniao dos array: ");
		print_array(sortableC, arr_sizeC);

		merge_sort(sortableC, arr_sizeC, tmpC);
		printf("Sorted Array: ");
		print_array(sortableC, arr_sizeC);

		free(sortableA);
		free(tmpA);
		free(sortableB);
		free(tmpB);
		free(sortableC);
		free(tmpC);
	} else {
		// Não é rank 0 e ta esperando sua mensagem
		int bit = 31 - __builtin_clz(rank);
		double d = pow(2, bit);
		int waitingFrom = rank - d;
		int arraySize;
		//int arrayBSize;
		MPI_Recv(&arraySize, 1, MPI_INT, waitingFrom, MPI_ANY_TAG, MPI_COMM_WORLD, &st); // Recebe primeiro o size
		if(size > 0) { // Se o size == 0 significa que o programa já acabou
			int* arrayCopy = malloc(arraySize * sizeof(int));
			int* tmp = malloc(arraySize * sizeof(int));
			MPI_Recv(arrayCopy, arraySize, MPI_INT, waitingFrom, MPI_ANY_TAG, MPI_COMM_WORLD, &st); // Recebe o array
			recursive_merge_sort(arrayCopy, 0, arraySize, tmp); // Ordena o array
			MPI_Send(arrayCopy, arraySize, MPI_INT, waitingFrom, 0, MPI_COMM_WORLD); // Envia o array ordenado
			// Libera da memoria
			free(tmp);
			free(arrayCopy);
		}
	}
	while(divisions < maxDivisions) { // Finaliza mandando mensagem para todos que não foram usados para acabar com tarefa
		int size = 0;
		int to = rank + pow(2, power++);
		//printf("Enviando mensagem de %d para %d\n", rank, to);
		MPI_Send(&size, 1, MPI_INT, to, 0, MPI_COMM_WORLD);
		divisions++;
	}
	MPI_Finalize();
	return 0;
}
