#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

//тут возникает deadlock почему-то
int main(int argc, char** argv) {
	int rank, number, namelen, size;
	MPI_Status status; 
	MPI_Init(&argc,&argv); 
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	char processor_name[MPI_MAX_PROCESSOR_NAME];
	MPI_Get_processor_name(processor_name,&namelen);
	
	if (rank == 0) {
		for (int i = 1; i < size; i++) {
			MPI_Sendrecv(&rank, 1, MPI_INT, i, 10, &number, 1, MPI_INT, 0, 10, MPI_COMM_WORLD, &status);
		}
	}
	printf("Procesu cu rancul %d al nodului '%s' a primit valoarea %d de la procesul cu rancul %d\n",rank, processor_name, number, number);
	
	MPI_Finalize();
	return 0;
}