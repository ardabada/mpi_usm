#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
int main(int argc, char* argv[])
{
    int i, k, p, size, rank, incep = 0, t, master_data;
    int rank_gr, rank_gr2, gr2_master_rank = 1;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Status status;
	MPI_Comm com_new, com_new2;
    MPI_Group MPI_GROUP_WORLD, newgr, newgr2;
    int* ranks;
    int namelen;
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
				if (rN == gr2_master_rank) {
					repeat = 1;
					continue;
				}
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

    MPI_Group_excl(MPI_GROUP_WORLD, k, ranks, &newgr2);
    MPI_Group_rank(newgr2, &rank_gr2);
    MPI_Comm_create(MPI_COMM_WORLD, newgr2, &com_new2);

    if (rank_gr != MPI_UNDEFINED) {
		if (rank_gr == incep) {
			MPI_Send(&rank_gr,1,MPI_INT, (rank_gr + 1) % k, 10, com_new);
      		MPI_Recv(&t,1,MPI_INT, (rank_gr+k-1) % k,10,com_new,&status);
			// MPI_Send(&rank_gr,1,MPI_INT, ranks[(rank_gr + 1) % k], 10, MPI_COMM_WORLD);
      		// MPI_Recv(&t,1,MPI_INT, ranks[(rank_gr+k-1) % k],10,MPI_COMM_WORLD,&status);
		}
		else {
			MPI_Recv(&t,1,MPI_INT, (rank_gr+k-1)%k,10,com_new,&status);
			MPI_Send(&rank_gr,1,MPI_INT,(rank_gr+1)%k,10,com_new);
			// MPI_Recv(&t,1,MPI_INT, ranks[(rank_gr+k-1)%k],10,MPI_COMM_WORLD,&status);
			// MPI_Send(&rank_gr,1,MPI_INT,ranks[(rank_gr+1)%k],10,MPI_COMM_WORLD);
		}

		printf("Proces rank %d (%d) node %s a primit valoadea %d de la proces cu rank %d\n", rank_gr, rank, processor_name, t, t);
	}
	if (rank_gr2 != MPI_UNDEFINED) {
		master_data = 10;
		MPI_Bcast(&master_data, 1, MPI_INT, gr2_master_rank, com_new2);
		printf("Proces rank %d (%d node %s) master_data = %d\n", rank_gr2, rank, processor_name, master_data);
	}
    MPI_Group_free(&newgr);
    MPI_Finalize();
    return 0;
}
