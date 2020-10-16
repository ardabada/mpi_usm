#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
int main(int argc, char* argv[])
{
    int i, k, p, size, rank, size_new, rank_new, sour, dest;
    int rank_gr;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Status status;
    MPI_Comm com_new, ring1;
    MPI_Group MPI_GROUP_WORLD, newgr;
    int* ranks;
    int namelen;
    int D1 = 123, D2;
    int dims[1], period[1], reord;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(processor_name, &namelen);
    if (rank == 0)
        printf("\n=====REZULTATUL PROGRAMULUI '%s' \n", argv[0]);
    MPI_Barrier(MPI_COMM_WORLD);
    srand(time(0));
    k = size / 2;
    ranks = (int*)malloc(k * sizeof(int));
    if (rank == 0) {
        int rN = 0;
        int repeat;
        for (i = 0; i < k; i++) {
            do {
                repeat = 0;
                rN = rand() % size;
                for (int j = 0; j < i; ++j) {
                    if (rN == ranks[j]) {
                        repeat = 1;
                        break;
                    }
                }
            } while (repeat == 1);

            ranks[i] = rN;
        }

        printf("Au fost extrase aleator %d numere dupa cum urmeaza:\n", k);
        for (i = 0; i < k; i++)
            printf(" %d ", ranks[i]);
        printf(" \n");
        MPI_Barrier(MPI_COMM_WORLD);
    }
    else
        MPI_Barrier(MPI_COMM_WORLD);

    MPI_Comm_group(MPI_COMM_WORLD, &MPI_GROUP_WORLD);
    MPI_Bcast(ranks, k, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Group_incl(MPI_GROUP_WORLD, k, ranks, &newgr);
    MPI_Group_rank(newgr, &rank_gr);
    MPI_Comm_create(MPI_COMM_WORLD, newgr, &com_new);
    if (rank_gr != MPI_UNDEFINED) {
		MPI_Comm_size(com_new, &size_new);
        MPI_Comm_rank(com_new, &rank_new);

        dims[0] = size_new;
        period[0] = 1;
        reord = 1;
      
        MPI_Barrier(com_new);

        //realizara topologiei de tip cerc
        MPI_Cart_create(com_new, 1, dims, period, reord, &ring1);
        MPI_Cart_shift(ring1, 1, 2, &sour, &dest);

        D1 = D1 + rank;
        MPI_Sendrecv(&D1, 1, MPI_INT, dest, 12, &D2, 1, MPI_INT, sour, 12, ring1, &status);

        MPI_Barrier(com_new);

        if (rank_new == 0) {
       		printf("===Rezultatul MPI_Sendrecv:\n");
            MPI_Barrier(com_new);
        }
        else MPI_Barrier(com_new);
		printf ("Process %d (%d from %s), received from proces %d the value %d and send to proces %d the value %d\n",rank_new,rank,processor_name,sour,D2,dest,D1);
        MPI_Barrier(com_new);
        MPI_Group_free(&newgr);
        MPI_Comm_free(&ring1);
        MPI_Comm_free(&com_new);
	}
        // printf("Sunt procesul cu rankul %d (%d) de pe nodul %s. \n", rank_gr, rank, processor_name);
    // MPI_Group_free(&newgr);
    MPI_Finalize();
    return 0;
}
