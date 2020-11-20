#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
int main(int argc, char** argv)
{
int rank, size, namelen, version, subversion, psize;
int parent_rank;
int globalrank,sumrank;
MPI_Comm parent,allcom;
char processor_name[MPI_MAX_PROCESSOR_NAME];
MPI_Init(&argc, &argv);
MPI_Comm_rank(MPI_COMM_WORLD,&rank);
MPI_Comm_size(MPI_COMM_WORLD,&size);
MPI_Get_processor_name(processor_name,&namelen);
MPI_Get_version(&version,&subversion);
printf("I'm worker %d of %d on %s running MPI %d.%d\n", rank, size, processor_name, version, subversion);
MPI_Comm_get_parent(&parent);
if (parent == MPI_COMM_NULL) { printf("Error: no parent process found!\n");
exit(1);
}
MPI_Comm_remote_size(parent,&psize);
if
(psize!=1)
{
printf("Error: number of parents (%d) should be 1.\n", psize);
exit(2);
}
// MPI_Intercomm_merge	-­‐ creates  an	 intracommunicator  from  an  intercommunicator
MPI_Intercomm_merge(parent,1,&allcom);
  MPI_Comm_rank(allcom,	 &globalrank);
 printf("worker: globalrank is %d,rank is %d \n",globalrank, rank);
	
	MPI_Reduce(&globalrank,&sumrank, 1, MPI_INT, MPI_SUM, 0, allcom);
	MPI_Bcast(&sumrank, 1, MPI_INT, 0, allcom);

 	
  printf("sumrank (com) after allreduce on  process  %d  is  %d  \n",	 rank,sumrank);
MPI_Comm_disconnect(&parent);
MPI_Finalize();
return 0;
}
