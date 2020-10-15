/*
передача сообщения по часовой стрелке по четным процессам, начиная от процесса incep
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int size, incep = 0;

//расчет следующего процесса для отправки сообщения от процесса current
int getSendRank(int current) {
	int result = current;
	if (current % 2 == 0)
		result += 2;
	else result += 1;
	if (current < incep && result > incep)
		result = incep;
	result %= size;
	return result;
}
//расчет процесса от которого придет сообщение для процесса current
int getRecvRank(int current) {
	int result = current + size;
	if (current % 2 == 0)
		result -= 2;
	else result -= 1;
	if ((current > incep && result - size < incep) || (result < incep && current < incep))
		result = incep;
	result %= size;
	return result;
}
int main(int argc, char** argv) {
	int rank, number, namelen;
	MPI_Status status; 
	MPI_Init(&argc,&argv); 
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	MPI_Get_processor_name(processor_name,&namelen);
	
	if (rank == incep) {
		MPI_Send(&rank, 1, MPI_INT, getSendRank(rank), 10, MPI_COMM_WORLD);
		MPI_Recv(&number, 1, MPI_INT, getRecvRank(rank), 10, MPI_COMM_WORLD, &status);
	}
	else if (rank % 2 == 0) {
		MPI_Recv(&number,1,MPI_INT, getRecvRank(rank),10,MPI_COMM_WORLD,&status);
		MPI_Send(&rank,1,MPI_INT,getSendRank(rank),10,MPI_COMM_WORLD);
	}
	
	//работают только четные процессы и процесс incep, поэтому вывод только у них
	if (rank % 2 == 0 || rank == incep)
		printf("Procesu cu rancul %d al nodului '%s' a primit valoarea %d de la procesul cu rancul %d\n",rank, processor_name, number, number);
	MPI_Finalize();
	return 0;
}