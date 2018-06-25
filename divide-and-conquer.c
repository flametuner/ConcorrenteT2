#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

int rank = 0;
int power = 0;
int maxDivisions = 0;
int divisions = 0;
// TODO Saber o size e o máximo de vezes que pode ser divido
// TODO Saber oq fazer quando size não esta em log2
int sum_numbers(int start, int end) {
  if (end - start < 1)
		return start;

  int midle = (start + end)/2;
  int f;
  int to = rank + pow(2, power++);
  int receive = 0;
  if(divisions < maxDivisions) {
    int buffer[2] = {start, midle};
    //printf("Enviando mensagem de %d para %d\n", rank, to);
    MPI_Send(&buffer, 2, MPI_INT, to, 0, MPI_COMM_WORLD);
    receive = 1;
    divisions++;
  }
  int s = sum_numbers(midle + 1, end);
  if(receive) {
    MPI_Status st;
    MPI_Recv(&f, 1, MPI_INT, to, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
    //divisions++;
    //power++;
  } else {
    f = sum_numbers(start, midle);
  }
  return f + s;
}

int main(int argc, char **argv)  {
  MPI_Init(&argc, &argv);
  // Inicio do ambiente com MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int bitSize = 31 - __builtin_clz(size - 1); // Ex: 5 : 0100 (size - 1) = 2
  int value = ((pow(2, bitSize) * 2) - 1); // Ex 5: 0111 Se cria a mascara

  if(rank > 0) {
    // Onde começa a potencia ta certo
    int bitMax = 31 - __builtin_clz(rank & value);
    power = (bitMax + 1);
    // Maximo de divisão para rank > 0 ta errado :c
    maxDivisions = bitSize - bitMax;
    double lo = log2(size);
    int loi = lo;
    if(lo - loi > 0) {
      if(rank >= size + (value >> 1) - value && maxDivisions > 0)
        maxDivisions -= 1;
    }
  } else {
    if(size > 1)
      maxDivisions = bitSize + 1;
  }
  //printf("Rank %d, maxDivisions %d\n", rank, maxDivisions);
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Status st;
  int inicial = 0;
  int final = 300;
  if(rank == 0) {// Programa é raiz
    int final2 = sum_numbers(inicial, final);
    printf("%d\n", final2);
  } else { // Programa ainda vai receber informações
      int bit = 31 - __builtin_clz(rank);
      double d = pow(2, bit);
      int waitingFrom = rank - d;
      int buffer[2];
      MPI_Recv(&buffer, 2, MPI_INT, waitingFrom, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
      int ret = sum_numbers(buffer[0], buffer[1]);
      MPI_Send(&ret, 1, MPI_INT, waitingFrom, 0, MPI_COMM_WORLD);

  }
  while(divisions < maxDivisions) { // Finaliza mandando mensagem para todos que não foram usados para acabar com tarefa
    int buffer[2] = {0, 0};
    int to = rank + pow(2, power++);
    //printf("Enviando mensagem de %d para %d\n", rank, to);
    MPI_Send(&buffer, 2, MPI_INT, to, 0, MPI_COMM_WORLD);
    divisions++;
  }
  MPI_Finalize();
  return 0;
}
