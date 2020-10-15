#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
int main(int argc, char* argv[])
{
    int k, p, size, rank;
    int rank_gr_cet, rank_gr_necet;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Status status;
	MPI_Comm com_cet, com_necet;
    MPI_Group MPI_GROUP_WORLD, gr_cet, gr_necet;
    int *ranks, *matrix;
    int namelen;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(processor_name, &namelen);

	int m_in[size], m_out[size];
	struct 
	{
		double val;
		int rank;
	} in[size], out_max[size], out_min[size];

    srand(time(0));
    k = size / 2;
    ranks = (int*)malloc(k * sizeof(int));

    if (rank == 0) {
        printf("\n=====REZULTATUL PROGRAMULUI '%s' \n", argv[0]);
		matrix = (int*)malloc(size * size * sizeof(int));
		for (int i = 0; i < size * size; i++)
			matrix[i] = rand() % 20 + 1;

		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++)
				printf("%5d", matrix[i * size + j]);
			printf("\n");
		}
		printf("\n");
		
        int j = 0;
        for (int i = 0; i < size; i++) {
			if (i % 2 == 0)
				ranks[j++] = i;
        }
    	MPI_Barrier(MPI_COMM_WORLD);
	}
	else 
    	MPI_Barrier(MPI_COMM_WORLD);

    MPI_Comm_group(MPI_COMM_WORLD, &MPI_GROUP_WORLD);
    MPI_Bcast(ranks, k, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Group_incl(MPI_GROUP_WORLD, k, ranks, &gr_cet);
    MPI_Group_rank(gr_cet, &rank_gr_cet);

    MPI_Group_excl(MPI_GROUP_WORLD, k, ranks, &gr_necet);
    MPI_Group_rank(gr_necet, &rank_gr_necet);

	MPI_Comm_create(MPI_COMM_WORLD, gr_cet, &com_cet);
	MPI_Comm_create(MPI_COMM_WORLD, gr_necet, &com_necet);

	MPI_Scatter(matrix, size, MPI_INT, m_in, size, MPI_INT, 0, MPI_COMM_WORLD);
	for (int i = 0; i < size; ++i) 
	{
		in[i].val = m_in[i];
		in[i].rank = rank;
	}
	// MPI_Reduce(in, out_max, size, MPI_DOUBLE_INT, MPI_MAXLOC, 0, MPI_COMM_WORLD);
    if (rank_gr_cet != MPI_UNDEFINED) {
		MPI_Reduce(in, out_max, size, MPI_DOUBLE_INT, MPI_MAXLOC, 0, com_cet);
		MPI_Barrier(com_cet);
	}
	// else if (rank_gr_necet != MPI_UNDEFINED) {
	// 	MPI_Reduce(in, out_min, size, MPI_DOUBLE_INT, MPI_MINLOC, 0, com_necet);
	// 	// MPI_Barrier(com_necet);
	// }

	if (rank == 0) {
		for (int i = 0; i < size; ++i) {
			int max = out_max[i].val;
			int max_line = out_max[i].rank;
			int min = 0; //out_max[i].val;
			int min_line = 0; // out_min[i].rank;
			printf("Coloana %d, max = %d, max linia = %d, min = %d, min linia = %d\n", i, max, max_line, min, min_line); 
		}
	}

    MPI_Group_free(&gr_cet);
    MPI_Group_free(&gr_necet);
    MPI_Finalize();
    return 0;
}
