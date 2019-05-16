
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdint.h>

//in us
#define RECORD_STAT_QUANTUM 500000
#define GLOBAL_SCHED_QUANTUM 500000


#define SWAP_INERTIA 5
#define STARTUP_LATENCY 5

#define LEVEL2_MAP_QUANTUM 20
#define LEVEL3_MAP_QUANTUM 25

enum program_type {UNKNOWN, STREAMING, LG_MATMUL, SM_MATMUL, COMPUTE}; 
enum status {GOOD, BAD, WASTE};

int neighbours[34][4] = 
                 {  {4, 1, -1, -1},
                    {0, 5, -1, -1},
                    {8, 3, -1, -1},
                    {2, 9, -1, -1},
                    {10, 5, 0, -1},
                    {4, 11, 6, 1},
                    {5, 12, 7, -1},
                    {6, 13, 8, -1},
                    {7, 14, 9, 2},
                    {8, 15, 3, -1},
                    {16, 11, 4, -1},
                    {10, 17, 12, 5},
                    {11, 18, 13, 6},
                    {12, 19, 14, 7},
                    {13, 20, 15, 8},
                    {14, 21, 9, -1},
                    {22, 17, 10, -1},
                    {16, 23, 18, 11},
                    {17, 24, 19, 12},
                    {18, 25, 20, 13},
                    {19, 26, 21, 14},
                    {20, 27, 15, -1},
                    {28, 23, 16, -1},
                    {22, 29, 24, 17},
                    {23, 30, 25, 18},
                    {24, 31, 26, 19},
                    {25, 32, 27, 20},
                    {26, 33, 21, -1},
                    {29, 22, -1, -1},
                    {28, 30, 23, -1},
                    {29, 31, 24, -1},
                    {30, 32, 25, -1},
                    {31, 33, 26, -1},
                    {32, 27, -1, -1}    };

int num_neighbours[34] = 
                    {   2,
                        2,
                        2,
                        2,
                        3,
                        4,
                        3,
                        3,
                        4,
                        3,
                        3,
                        4,
                        4,
                        4,
                        4,
                        3,
                        3,
                        4,
                        4,
                        4,
                        4,
                        3,
                        3,
                        4,
                        4,
                        4,
                        4,
                        3,
                        2,
                        3,
                        3,
                        3,
                        3,
                        2 };


struct stats_struct
{
	int64_t l2_cache_misses;
	int64_t l2_cache_accesses;
	int64_t num_instructions;
	int64_t num_cycles;
	int64_t num_vol_ctxt_switches;


	int32_t pid;
    int32_t mapping_index;

	int32_t moved_recently;
    
    program_type type;
    status stat; 

    float past_l2_mr;

};

struct core_write_struct
{
    int64_t core_write_id;
    int64_t padding1;
    int64_t padding2;
    int64_t padding3;
    int64_t padding4;
    int64_t padding5;
    int64_t padding6;
    int64_t padding7;
};

int neighbours2[34][8] = 
{{ 10, 5, -1, -1, -1, -1, -1, -1},
	{ 4, 11, 6, -1, -1, -1, -1, -1},
	{ 7, 14, 9, -1, -1, -1, -1, -1},
	{ 8, 15, -1, -1, -1, -1, -1, -1},
	{ 16, 11, 6, -1, -1, -1, -1, -1},
	{ 10, 17, 12, 7, 0, -1, -1, -1},
	{ 4, 11, 18, 13, 8, 1, -1, -1},
	{ 5, 12, 19, 14, 9, 2, -1, -1},
	{ 6, 13, 20, 15, 3, -1, -1, -1},
	{ 7, 14, 21, 2, -1, -1, -1, -1},
	{ 22, 17, 12, 5, 0, -1, -1, -1},
	{ 16, 23, 18, 13, 6, 1, 4, -1},
	{ 10, 17, 24, 19, 14, 7, 5, -1},
	{ 11, 18, 25, 20, 15, 8, 6, -1},
	{ 12, 19, 26, 21, 9, 2, 7, -1},
	{ 13, 20, 27, 3, 8, -1, -1, -1},
	{ 28, 23, 18, 11, 4, -1, -1, -1},
	{ 22, 29, 24, 19, 12, 5, 10, -1},
	{ 16, 23, 30, 25, 20, 13, 6, 11},
	{ 17, 24, 31, 26, 21, 14, 7, 12},
	{ 18, 25, 32, 27, 15, 8, 13, -1},
	{ 19, 26, 33, 9, 14, -1, -1, -1},
	{ 29, 24, 17, 10, -1, -1, -1, -1},
	{ 28, 30, 25, 18, 11, 16, -1, -1},
	{ 22, 29, 31, 26, 19, 12, 17, -1},
	{ 23, 30, 32, 27, 20, 13, 18, -1},
	{ 24, 31, 33, 21, 14, 19, -1, -1},
	{ 25, 32, 15, 20, -1, -1, -1, -1},
	{ 30, 23, 16, -1, -1, -1, -1, -1},
	{ 31, 24, 17, 22, -1, -1, -1, -1},
	{ 28, 32, 25, 18, 23, -1, -1, -1},
	{ 29, 33, 26, 19, 24, -1, -1, -1},
	{ 30, 27, 20, 25, -1, -1, -1, -1},
	{ 31, 21, 26, -1, -1, -1, -1, -1}};

int num_neighbours2[34] = {2, 3,  3, 2, 4, 5, 6, 6, 5, 4, 5, 7, 7, 7, 7, 5, 5, 7, 8, 8, 7, 5, 4, 6, 7, 7, 6, 4, 3, 4, 5, 5, 4, 3};



int neighbours3[34][12] = 
{{ 16, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 10, 17, 12, 7, 2, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 6, 13, 20, 15, -1, -1, -1, -1, -1, -1, -1 },
{ 7, 14, 21, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 22, 17, 12, 7, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 16, 23, 18, 13, 8, -1, -1, -1, -1, -1, -1, -1 },
{ 10, 17, 24, 19, 14, 9, 2, 0, -1, -1, -1, -1 },
{ 4, 11, 18, 25, 20, 15, 3, 1, -1, -1, -1, -1 },
{ 5, 12, 19, 26, 21, -1, -1, -1, -1, -1, -1, -1 },
{ 6, 13, 20, 27, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 28, 23, 18, 13, 6, 1, -1, -1, -1, -1, -1, -1 },
{ 22, 29, 24, 19, 14, 7, 0, -1, -1, -1, -1, -1 },
{ 16, 23, 30, 25, 20, 15, 8, 1, 4, -1, -1, -1 },
{ 10, 17, 24, 31, 26, 21, 9, 2, 5, -1, -1, -1 },
{ 11, 18, 25, 32, 27, 3, 6, -1, -1, -1, -1, -1 },
{ 12, 19, 26, 33, 2, 7, -1, -1, -1, -1, -1, -1 },
{ 29, 24, 19, 12, 5, 0, -1, -1, -1, -1, -1, -1 },
{ 28, 30, 25, 20, 13, 6, 1, 4, -1, -1, -1, -1 },
{ 22, 29, 31, 26, 21, 14, 7, 5, 10, -1, -1, -1 },
{ 16, 23, 30, 32, 27, 15, 8, 6, 11, -1, -1, -1 },
{ 17, 24, 31, 33, 9, 2, 7, 12, -1, -1, -1, -1 },
{ 18, 25, 32, 3, 8, 13, -1, -1, -1, -1, -1, -1 },
{ 30, 25, 18, 11, 4, -1, -1, -1, -1, -1, -1, -1 },
{ 31, 26, 19, 12, 5, 10, -1, -1, -1, -1, -1, -1 },
{ 28, 32, 27, 20, 13, 6, 11, 16, -1, -1, -1, -1 },
{ 22, 29, 33, 21, 14, 7, 12, 17, -1, -1, -1, -1 },
{ 23, 30, 15, 8, 13, 18, -1, -1, -1, -1, -1, -1 },
{ 24, 31, 9, 14, 19, -1, -1, -1, -1, -1, -1, -1 },
{ 31, 24, 17, 10, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 32, 25, 18, 11, 16, -1, -1, -1, -1, -1, -1, -1 },
{ 33, 26, 19, 12, 17, 22, -1, -1, -1, -1, -1, -1 },
{ 28, 27, 20, 13, 18, 23, -1, -1, -1, -1, -1, -1 },
{ 29, 21, 14, 19, 24, -1, -1, -1, -1, -1, -1, -1 },
{ 30, 15, 20, 25, -1, -1, -1, -1, -1, -1, -1, -1 }};

int num_neighbours3[34] = { 3,  5,   5,  3,  4,  5,  8,  8,  5,  4,  6,  7,  9,  9,  7,  6,  6,  8,  9,  9,  8,  6,  5,  6,  8,  8,  6,  5,  4,  5,  6,  6,  5,  4  };

