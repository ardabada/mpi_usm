#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define SPAWN_COUNT 1

int main( int argc, char *argv[] )
{
	int world_size, rank, namelen, flag, universe_size, *universe_sizep;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    char worker_program[100] = "./L10Ex2.exe";

   	MPI_Comm parent, everyone;
	MPI_Info hostinfo;
    char* host = (char*)"host";
	MPI_Init(&argc, &argv);
    MPI_Get_processor_name(processor_name, &namelen);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_get_parent(&parent); //получаем коммуникатор процесса-родителя
	if (parent == MPI_COMM_NULL) //если коммуникатор процесса-родителя = null, значит процесс не был сгенерирован родительским
		printf("=== Intercomunicatorul parinte nu a fost creat!\n");
	else {
		printf("\n == CHILD PROCESS RANK %d (%s) ==", rank, processor_name);
		MPI_Finalize();
		return 0;
	}

	MPI_Attr_get(MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, &universe_sizep, &flag);
    if (!flag) {
        printf("This MPI does not support UNIVERSE_SIZE. How many n processes total?\n> ");
        scanf("%d", &universe_size);
    }
    else
        universe_size = *universe_sizep;
		
	printf("\nUniverse size: %d", universe_size);
    if (universe_size == 1) {
        printf("\nNo room to start workers");
		MPI_Finalize();
		return 0;
	}
	MPI_Info_create(&hostinfo);
    MPI_Info_set(hostinfo, host, "compute-0-0,compute-0-3");
	MPI_Comm_spawn(worker_program, MPI_ARGV_NULL, SPAWN_COUNT, hostinfo, 0, MPI_COMM_WORLD, &everyone, MPI_ERRCODES_IGNORE);

	printf("\nParent process rank %d (%s)\n", rank, processor_name);
	
    MPI_Finalize();
    return 0;
}