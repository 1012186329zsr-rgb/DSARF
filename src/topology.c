#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include "utils.h"
#include "topo_gen.h"
#include "floyd_warshall.h"
#include "sa.h"
#include "heap.h"
#include "dfs.h"
#include "route_sim_anytopo.h"
#include "graph_analysis.h"
#include "tree_turn.h"

int main(int argc, char *argv[])
{
    // /********************* SA GEN *********************/
    // GraphAdjList *G = NULL;
    // if (atoi(argv[1]) == 0)
    // {
    //     G = sa_jellyfish_gen(atoi(argv[2]), atoi(argv[3]), atoi(argv[2]), 0); // num_nodes, degree, dst-threshold（=num_nodes -> non-local）
    // }
    // else
    // {
    //     int num_nodes = 1;
    //     int ary = atoi(argv[2]);
    //     int cube = atoi(argv[3]);
    //     for (int i = 0; i < cube; ++i)
    //         num_nodes *= ary;
    //     G = sa_swdc_torus_gen(ary, cube, atoi(argv[4]), num_nodes, 0); // ary cube rand_degrees
    // }

    /*********************  *********************/
    // char file_name[100];

    // GraphAdjList *G = NULL;
    // if (atoi(argv[1]) == 0)
    // {
    //     G = swdc_torus_gen(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), 0); // ary cube rand_degrees
    //     if (atoi(argv[3]) == 1)
    //         sprintf(file_name, "temp/221110_topo/swdc_ring_%d_%d.txt", atoi(argv[2]), atoi(argv[4]) + 2);
    //     else if (atoi(argv[3]) == 2)
    //         if (atoi(argv[4]) == 0)
    //             sprintf(file_name, "temp/221110_topo/torus_%d_%d.txt", atoi(argv[2]) * atoi(argv[2]), atoi(argv[4]) + 4);
    //         else
    //             sprintf(file_name, "temp/221110_topo/swdc_torus_%d_%d.txt", atoi(argv[2]) * atoi(argv[2]), atoi(argv[4]) + 4);
    //     else
    //         assert(0);
    // }
    // else if (atoi(argv[1]) == 1)
    // {
    //     G = jellyfish_gen(atoi(argv[2]), atoi(argv[3]), 0); // num_nodes degrees
    //     sprintf(file_name, "temp/221110_topo/jellyfish_%d_%d.txt", atoi(argv[2]), atoi(argv[3]));
    // }
    // else if (atoi(argv[1]) == 2)
    // {
    //     G = spinakker_gen(atoi(argv[2])); // ary
    //     sprintf(file_name, "temp/221110_topo/spinnaker_%d_%d.txt", atoi(argv[2]) * atoi(argv[2]), 6);
    // }
    // else if (atoi(argv[1]) == 3)
    // {
    //     G = sa_jellyfish_gen(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), 0);
    //     if (atoi(argv[2]) == atoi(argv[4]))
    //         sprintf(file_name, "temp/221110_topo/sa_jellyfish_%d_%d.txt", atoi(argv[2]), atoi(argv[3]));
    //     else
    //         sprintf(file_name, "temp/221110_topo/sa_jellyfish_%d_%d_local%d.txt", atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
    // }
    // else if (atoi(argv[1]) == 4)
    // {
    //     G = sa_swdc_torus_gen(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), 0); // ary cube rand_degrees
    //     if (atoi(argv[3]) == 1)
    //         if (atoi(argv[2]) == atoi(argv[5]))
    //             sprintf(file_name, "temp/221110_topo/sa_swdc_ring_%d_%d.txt", atoi(argv[2]), atoi(argv[4]) + 2);
    //         else
    //             sprintf(file_name, "temp/221110_topo/sa_swdc_ring_%d_%d_local%d.txt", atoi(argv[2]), atoi(argv[4]) + 2, atoi(argv[5]));
    //     else if (atoi(argv[3]) == 2)
    //         if (atoi(argv[2]) * atoi(argv[2]) == atoi(argv[5]))
    //             sprintf(file_name, "temp/221110_topo/sa_swdc_torus_%d_%d.txt", atoi(argv[2]) * atoi(argv[2]), atoi(argv[4]) + 4);
    //         else
    //             sprintf(file_name, "temp/221110_topo/sa_swdc_torus_%d_%d_local%d.txt", atoi(argv[2]) * atoi(argv[2]), atoi(argv[4]) + 4, atoi(argv[5]));
    //     else
    //         assert(0);
    // }
    // else
    //     assert(0);

    // double a1, a2, a3;
    // a1 = 0;
    // a2 = 0;
    // a3 = 0;
    // unsigned int **dist = NULL;
    // int **path = NULL;
    // bfs_initial_path_dist(G, &dist, &path);
    // bfs_shortest_path_algorithm_parallel(G, dist, path);
    // transpose(G->num_nodes, path);
    // floyd_check_path(G, path);

    // a1 = floyd_average_shortest_path_length(G->num_nodes, dist);
    // a2 = floyd_shortest_path_variance(G->num_nodes, dist);
    // unsigned int *array = floyd_initial_path_load_balance(G, path);
    // a3 = floyd_path_load_balance(G->num_nodes * G->degree, array);

    // bfs_delete_path_dist(dist, path);

    // write_graph_adjlist_to_file(G, file_name);
    // delete_graph(G);
    // printf("\n$$$$$\t%s\t$$$$$\n", file_name);
    // printf("result is %lf %lf %lf\n\n", a1, a2, a3);

    /********************* Jellyfish Gen *********************/
    // char file_name[100];
    // GraphAdjList *G = jellyfish_gen(atoi(argv[1]), atoi(argv[2]), (unsigned int)time(NULL)); // num_nodes degrees
    // sprintf(file_name, "temp/deadlock_free/topo/jellyfish_%d_%d.txt", atoi(argv[1]), atoi(argv[2]));
    // write_graph_adjlist_to_file(G, file_name);
    // delete_graph(G);

    /********************* SIM *********************/
    char file_name[100];
    char path_log_file_name[300];
    char traffic_name[300];
    /* parse optional runtime VC flag: --vc N (place anywhere; recommended at end) */
    for (int _i = 1; _i < argc; _i++) {
        if (strcmp(argv[_i], "--vc") == 0 && _i + 1 < argc) {
            int vc = atoi(argv[_i + 1]);
            if (vc >= 1 && vc <= 64) {
                ROUTE_SIM_ANYTOPO_VC_RUNTIME = vc;
                printf("Using runtime VC count = %d\n", ROUTE_SIM_ANYTOPO_VC_RUNTIME);
            } else {
                printf("Invalid VC count '%s' (must be 1..64), using default %d\n", argv[_i + 1], ROUTE_SIM_ANYTOPO_VC_RUNTIME);
            }
        }
    }
    // char *file_name = "temp/deadlock_free/topo/jellyfish_800_6.txt";
    sprintf(file_name, "%s", argv[1]);
    sprintf(path_log_file_name, "%s", argv[10]);
    sprintf(traffic_name, "%s", argv[11]);
    // route_sim_anytopo(char *file_name, int traffic_mode, int route_lut_mode, unsigned int seed, int ppp, int packets_num, int root_select)
    route_sim_anytopo(file_name, atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), atoi(argv[9]), path_log_file_name, traffic_name, atoi(argv[12]));
    printf("%s\n", file_name);

    /********************* statistic path *********************/
    // char file_name[100];
    // // char *file_name = "temp/deadlock_free/topo/jellyfish_800_6.txt";
    // sprintf(file_name, "%s", argv[1]);
    // // route_sim_anytopo(char *file_name, int traffic_mode, int route_lut_mode, unsigned int seed, int ppp, int packets_num, int root_select)
    // route_sim_anytopo(file_name, 0, 1, 0, 100, 100, atoi(argv[2]));
    // printf("%s\n", file_name);

    /*********************  *********************/
    // GraphAdjList *G = construct_graph_adjlist_from_file(argv[1]);
    // unsigned int **dist = NULL;
    // int **path = NULL;
    // bfs_initial_path_dist(G, &dist, &path);
    // bfs_shortest_path_algorithm_parallel(G, dist, path);
    // transpose(G->num_nodes, path);
    // floyd_check_path(G, path);
    // unsigned int dia = diameter(dist, G->num_nodes);
    // double avg_cluster_coefficient = average_cluster_coefficient(G);
    // int *betweeen_cen = NULL;
    // between_centrality(path, G->num_nodes, &betweeen_cen);
    // double centra_var = variance(G->num_nodes, betweeen_cen);
    // printf("%s, diameter = %u, avg_cluster_coefficient = %lf, var = %lf\n", argv[1], dia, avg_cluster_coefficient, centra_var);
    // bfs_delete_path_dist(dist, path);
    // free(betweeen_cen);
    // delete_graph(G);

    /********************* Tree Turn *********************/
    // GraphAdjList *G = jellyfish_gen(2000, 6, 0);
    // print_graph_adjlist(G);
    // TreeCoordinate *tc = gen_tree_coordinate(G, 0, 1);
    // unsigned int **dist = NULL;
    // int **path = NULL;
    // floyd_initial_path_dist(G, &dist, &path);
    // tree_turn_floyd_algorithm_parallel(G->num_nodes, tc, dist, path);
    // floyd_check_path(G, path);

    // // print_matrix(path, 50, "PATH");
    // // print_matrix(dist, 50, "DIST");

    // delete_graph(G);
    // delete_tc(tc);
    // floyd_delete_path_dist(dist, path);
    return 0;
}