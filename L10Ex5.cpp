#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

const int ROOT_RANK = 0;

int main(int argc, char **argv) {
	srand(time(NULL));

	int rank, size, globalrank;
	int L;

	MPI_Comm family_comm, allcomm;
	MPI_Status status;
	MPI_Info hostinfo;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if (rank == ROOT_RANK) {
		printf("Input L:\n");
		scanf("%d", &L);
	}

	MPI_Barrier(MPI_COMM_WORLD);

	//передача введенного L всем процессам
	MPI_Bcast(&L, 1, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);

	//запуск L процессов
	MPI_Info_create(&hostinfo);
	MPI_Info_set(hostinfo, "host", "compute-0-0,compute-0-3");
	printf("Spawning %d processes\n", L);
	MPI_Comm_spawn("./L10Ex5_child.exe", MPI_ARGV_NULL, L, hostinfo, 0, MPI_COMM_SELF, &family_comm, MPI_ERRCODES_IGNORE);
	MPI_Intercomm_merge(family_comm, 1, &allcomm);
	MPI_Comm_rank(allcomm, &globalrank);

	//передача введенного L всем созданным процессам
	for (int i = 0; i < L; i++) {
		MPI_Send(&L, 1, MPI_INT, i, 111, family_comm);
	}

	double *normas = (double*)malloc(L * sizeof(double)); //вектор посчитанных норм
	double **Xs = (double**)malloc(L * sizeof(double*)); //массив указателей на исходные вектора X
	for (int i = 0; i < L; i++) {
		double *X = (double*)malloc(L * sizeof(double)); //инициализация вектора X
		Xs[i] = X;
		for (int j = 0; j < L; j++)
			X[j] = rand() / 1000000000.0;
		
		printf("[PARENT %d]: generated vector X%d: ", rank, i);
		for (int j = 0; j < L; j++)
			printf("%.2f ", X[j]);
		printf("\n");

		MPI_Send(X, L, MPI_DOUBLE, i, 111, family_comm); //передача вектора X созданному процессу
		MPI_Recv(normas + i, 1, MPI_DOUBLE, i, 111, family_comm, &status); //получение значение посчитанной нормы от созданного процесса и запись его в вектор посчитанных норм
	}

	//вывод всех норм и поиск индекса минимальной нормы
	int min_index = 0;
	printf("\nALL NORMAS (rank %d): ", rank);
	for (int i = 0; i < L; i++) {
		printf("%.2f ", normas[i]);
		if (normas[i] < normas[min_index])
			min_index = i;
	}
	printf("\n");
	printf("Min norma value: %.2f (index %d, rank %d)\n", normas[min_index], min_index, rank);
	//вывод вектора с минимальной нормой
	double* min_norma_index = Xs[min_index];
	printf("\nMIN NORMA (%.2f) VECTOR X%d (rank %d): ", min_index, rank);
	for (int i = 0; i < L; i++) {
		printf("%.2f ", min_norma_index[i]);
	}
	printf("\n");

	//очищение памяти
	free(normas);
	for (int i = 0; i < L; i++)
		free(Xs[i]);
	free(Xs);

	MPI_Comm_disconnect(&family_comm);
	MPI_Finalize();
	return 0;
}