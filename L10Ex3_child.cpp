#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main( int argc, char *argv[] )
{
	int rank, size, psize, namelen;
	int parent_rank;
	int globalrank,sumrank;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	MPI_Comm parent,allcom;
	MPI_Status status; 
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Get_processor_name(processor_name, &namelen);
	MPI_Comm_get_parent(&parent);
	//проверка, что процесс запущен родителем
	if (parent == MPI_COMM_NULL) { 
		printf("Error: no parent process found!\n");
		exit(1);
	}

	MPI_Comm_remote_size(parent,&psize);
	if (psize!=1)
	{
		printf("Error: number of parents (%d) should be 1.\n", psize);
		exit(2);
	}
	MPI_Intercomm_merge(parent, 1, &allcom);
	MPI_Comm_rank(allcom, &globalrank);

	printf("[CHILD %d/%d on %s]\n", globalrank, rank, processor_name);
	
	MPI_Comm_disconnect(&parent);
    MPI_Finalize();
    return 0;
}