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
 void merge(int* numbers1, int* numbers2, int begin1, int end1, int begin2, int end2, int beginSorted, int endSorted, int * tmp, int * sorted) {
	int i, j;
	i = begin1; j = begin2;
	//debug("Merging. Begin: %d, Middle: %d, End: %d\n", begin, middle, end);
	for (int k = beginSorted; k < endSorted; ++k) {
		debug("LHS[%d]: %d, RHS[%d]: %d\n", i, numbers1[i], j, numbers2[j]);
		if (i < end1 && (j >= end2 || numbers1[i] < numbers2[j])) {
			tmp[k] = numbers1[i];
			//printf("Merge2 Rank %d\n", rank);
			i++;
		} else {
			tmp[k] = numbers2[j];
			j++;
		}
	}

	for(int i = beginSorted; i < endSorted; i++) {
		sorted[i] = tmp[i];
	}
}

void print_array(int* array, int size) {
	printf("Printing Array:\n");
	for (int i = 0; i < size; ++i) {
		printf("%d. ", array[i]);
	}
	printf("\n");
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
		memcpy(copy, numbers + begin, arraySize * sizeof(int)); // Faz uma copia do array para copy

		int to = rank + pow(2, power++);
		int receive = 0;
		if(divisions < maxDivisions) { // Se puder se dividir em mais processos

			MPI_Send(&arraySize, 1, MPI_INT, to, 0, MPI_COMM_WORLD);
			MPI_Send(copy, arraySize, MPI_INT, to, 0, MPI_COMM_WORLD); // Envia a copia do array
			
			receive = 1;
			divisions++;
		}
		// Deixando rodando metade
		recursive_merge_sort(numbers, middle, end, tmp);
		if(receive) {
			MPI_Status st;
			MPI_Recv(copy, arraySize, MPI_INT, to, MPI_ANY_TAG, MPI_COMM_WORLD, &st); // Recebe o array ordenado do outro processo
		} else {
			recursive_merge_sort(copy, 0, arraySize, tmp); // Tem que usar o mesmo processo pra calcular
		}
		merge(numbers, copy, middle, end, 0, arraySize, begin, end, tmp, numbers); // Ordena o numbers e copy em um array só e o destino é numbers
		free(copy);
	}
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
		if (lo - loi > 0) {
			if (rank >= size + (value >> 1) - value)
				maxDivisions -= 1;
		}
	} else {
		if (size > 1)
			maxDivisions = bitSize + 1;
	}

	MPI_Barrier(MPI_COMM_WORLD);

	if (rank == 0) {
		int seed, max_val;
		int * sortable;
		int * tmp;
		size_t arr_size;

		switch (argc) {
			case 1:
				seed = time(NULL);
				arr_size = NELEMENTS;
				max_val = MAXVAL;
				break;
			case 2:
				seed = atoi(argv[1]);
				arr_size = NELEMENTS;
				max_val = MAXVAL;
				break;
			case 3:
				seed = atoi(argv[1]);
				arr_size = atoi(argv[2]);
				max_val = MAXVAL;
				break;
			case 4:
				seed = atoi(argv[1]);
				arr_size = atoi(argv[2]);
				max_val = atoi(argv[3]);
				break;
			default:
				printf("Too many arguments\n");
				break;
		}

		srand(seed);
		sortable = malloc(arr_size*sizeof(int));
		tmp 	 = malloc(arr_size*sizeof(int));

		populate_array(sortable, arr_size, max_val);
		memcpy(tmp, sortable, arr_size*sizeof(int));

		print_array(sortable, arr_size);
		merge_sort(sortable, arr_size, tmp);
		print_array(sortable, arr_size);

		free(sortable);
		free(tmp);
	} else {
		// Não é rank 0 e ta esperando sua mensagem
		int bit = 31 - __builtin_clz(rank);
		double d = pow(2, bit);
		int waitingFrom = rank - d;
		int arraySize;
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
