#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

//функция подсчета нормы
double calcNorm(double *array, int length) {
	if (length <= 0)
		return 0;
	double result = array[0] * array[0];
	for (int i = 1; i < length; i++)
		result += array[i] * array[i];
	return sqrt(result);
}

int main(int argc, char** argv)
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

	//получение значение L, введенного в родителе
	int L;
	MPI_Recv(&L, 1, MPI_INT, 0, 111, parent, &status);

	//получение вектора X, сгенерированного в родителе
	double *X = (double*)malloc(L * sizeof(double));
	MPI_Recv(X, L, MPI_DOUBLE, 0, 111, parent, &status);
	
	//подсчет нормы для полученного вектора
	double norma = calcNorm(X, L);
	printf("[CHILD %d/%d]: recv vector X: ", globalrank, rank);
	for (int j = 0; j < L; j++)
		printf("%.2f ", X[j]);
	printf(" | %.2f\n", norma);

	//отправка родительскому процессу подсчитанной нормы для полученного вектора
	MPI_Send(&norma, 1, MPI_DOUBLE, 0, 111, parent);
		
	MPI_Comm_disconnect(&parent);
	MPI_Finalize();
	return 0;
}
