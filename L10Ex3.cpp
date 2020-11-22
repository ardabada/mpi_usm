#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main( int argc, char *argv[] )
{
	int world_size, rank, globalrank, namelen, flag, universe_size, *universe_sizep;
    char processor_name[MPI_MAX_PROCESSOR_NAME];


	MPI_Comm family_comm, allcomm;
	MPI_Info hostinfo;
    char* host = (char*)"host";
	MPI_Init(&argc, &argv);
    MPI_Get_processor_name(processor_name, &namelen);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	MPI_Info_create(&hostinfo);
    MPI_Info_set(hostinfo, host, "compute-0-0,compute-0-3");
	
	int num_to_spawn = 4;
	printf("Spawning %d processes\n", num_to_spawn);
	MPI_Comm_spawn("./L10Ex3_child.exe", MPI_ARGV_NULL, num_to_spawn, hostinfo, 0, MPI_COMM_SELF, &family_comm, MPI_ERRCODES_IGNORE);
	MPI_Intercomm_merge(family_comm, 1, &allcomm);
	MPI_Comm_rank(allcomm, &globalrank);

	printf("[PARENT %d/%d on %s]", rank, globalrank, processor_name);
	
    MPI_Finalize();
    return 0;
}