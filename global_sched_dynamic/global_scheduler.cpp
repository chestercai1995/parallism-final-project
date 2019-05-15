#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sys/wait.h>

#include "shm.h"
#include "stats.h"

using namespace std;

/* =========================================== */
//Globals

int num_programs;
stats_struct *stats_ptrs;
core_write_struct **core_mapping;


//Global stats
stats_struct *core_stats;

//For shm
int shmid;
int *shmids;

/* =========================================== */
void swap_processes(int core_src, int core_dest)
{
    printf("SSSSSSSSSSSSSSSSSSSSSSSSSs%lu\n", sizeof(stats_struct));
    int i = core_src;
    int j = core_dest;
    //swap
    //swap pids
    stats_struct * src =  &stats_ptrs[i];
    stats_struct * dest =  &stats_ptrs[j];

    int32_t pid1 = src->pid;
    int32_t pid2 = dest->pid;

    int32_t map1 = src->mapping_index;
    int32_t map2 = dest->mapping_index;

    src->pid = pid2;
    dest->pid = pid1;
    
    src->mapping_index = map2;
    dest->mapping_index = map1;

    int32_t core1 = core_mapping[map1]->core_write_id;
    int32_t core2 = core_mapping[map2]->core_write_id;
    
    core_mapping[map1]->core_write_id = core2;
    core_mapping[map2]->core_write_id = core1;

    cpu_set_t set1;
    cpu_set_t set2;

    CPU_ZERO(&set1);
    CPU_ZERO(&set2);

    CPU_SET(core2, &set1);
    CPU_SET(core1, &set2);

    int set_val1 = sched_setaffinity(pid1, sizeof(set1), &set1);
    int set_val2 = sched_setaffinity(pid2, sizeof(set2), &set2);

    if(set_val1==-1 || set_val2==-1)
    {
        printf("Unable to swap threads\n");
    }
         

}
int findNeighborIndex(int i){
    if(i%2){
        return i - 1;
    }
    else{
        return i + 1;
    }
    return -1;
}

void *global_scheduler(int intr)
{

	int i = 0;
	for(; i < num_programs; i++){//see if there are any misidentified types
        if(core_stats[i].l2_cache_accesses > 1500000){//larger than 1.5M, mark it as ST
            core_stats[i].type = STREAMING;
        }
        else if(core_stats[i].l2_cache_accesses > 130000){//larger than 130k marking it as LG
           core_stats[i].type = LG_MATMUL;
        }
        else if(core_stats[i].l2_cache_accesses > 10000){
            core_stats[i].type = SM_MATMUL;    
        }
        else{
            core_stats[i].type = COMPUTE;    
        }
	}
	for(i = 0; i < 34; i ++){//loot at each tile to see if each match makes sense
		if(core_stats[2 * i].type == STREAMING){
			if(core_stats[2*i + 1].type == STREAMING){
				core_stats[2 * i].stat = GOOD;
				core_stats[2 * i + 1].stat = GOOD;
			}
			else if(core_stats[2*i + 1].type == SM_MATMUL){
				core_stats[2 * i].stat = GOOD;
				core_stats[2 * i + 1].stat = GOOD;
			}
			else if(core_stats[2*i + 1].type == LG_MATMUL){
				core_stats[2 * i].stat = BAD;
				core_stats[2 * i + 1].stat = GOOD;
			}
			else if(core_stats[2*i + 1].type == COMPUTE){
				core_stats[2 * i].stat = WASTE;
				core_stats[2 * i + 1].stat = GOOD;
			}
			else{//unknown
			}
		}
		else if(core_stats[2 * i].type == SM_MATMUL){
			if(core_stats[2*i + 1].type == STREAMING){
				core_stats[2 * i].stat = GOOD;
				core_stats[2 * i + 1].stat = GOOD;
			}
			else if(core_stats[2*i + 1].type == SM_MATMUL){
				core_stats[2 * i].stat = GOOD;
				core_stats[2 * i + 1].stat = GOOD;
			}
			else if(core_stats[2*i + 1].type == LG_MATMUL){
				core_stats[2 * i].stat = WASTE;
				core_stats[2 * i + 1].stat = GOOD;
			}
			else if(core_stats[2*i + 1].type == COMPUTE){
				core_stats[2 * i].stat = GOOD;
				core_stats[2 * i + 1].stat = WASTE;
			}
			else{//unknown
			}
		}
		else if(core_stats[2 * i].type == LG_MATMUL){
			if(core_stats[2*i + 1].type == STREAMING){
				core_stats[2 * i].stat = GOOD;
				core_stats[2 * i + 1].stat = BAD;
			}
			else if(core_stats[2*i + 1].type == SM_MATMUL){
				core_stats[2 * i].stat = GOOD;
				core_stats[2 * i + 1].stat = WASTE;
			}
			else if(core_stats[2*i + 1].type == LG_MATMUL){
				core_stats[2 * i].stat = BAD;
				core_stats[2 * i + 1].stat = GOOD;
			}
			else if(core_stats[2*i + 1].type == COMPUTE){
				core_stats[2 * i].stat = GOOD;
				core_stats[2 * i + 1].stat = GOOD;
			}
			else{//unknown
			}
		}
		else if(core_stats[2 * i].type == COMPUTE){
			if(core_stats[2*i + 1].type == STREAMING){
				core_stats[2 * i].stat = GOOD;
				core_stats[2 * i + 1].stat = WASTE;
			}
			else if(core_stats[2*i + 1].type == SM_MATMUL){
				core_stats[2 * i].stat = WASTE;
				core_stats[2 * i + 1].stat = GOOD;
			}
			else if(core_stats[2*i + 1].type == LG_MATMUL){
				core_stats[2 * i].stat = GOOD;
				core_stats[2 * i + 1].stat = GOOD;
			}
			else if(core_stats[2*i + 1].type == COMPUTE){
				core_stats[2 * i].stat = BAD;
				core_stats[2 * i + 1].stat = GOOD;
			}
			else{//unknown
			}
		}
	}
	for(i = 0; i < num_programs; i++){//find a swap for each bad
		if(core_stats[i].stat == BAD){
			int j;
            int found = -1;
			if(core_stats[i].type == STREAMING){//trying to find another streaming or small
				for(j = 0; j < num_programs; j++){
				    if(core_stats[findNeighborIndex(j)].type == STREAMING 
                        || core_stats[findNeighborIndex(j)].type == SM_MATMUL){
                        if(core_stats[j].stat == GOOD)
                            continue;
                        found = j;
                        if(core_stats[j].stat == BAD){
                            break;
                        }
                    }	
				}
                if(found == -1){//did not find a suitable condidate to switch
                
                }
            }
            else if(core_stats[i].type == LG_MATMUL){
				for(j = 0; j < num_programs; j++){//try finding a compute first
				    if(core_stats[findNeighborIndex(j)].type == COMPUTE){
                        if(core_stats[j].stat == BAD){
                            found = j;
                            break;
                        }
                        else if(core_stats[j].stat == WASTE){
                            found = j;
                        }
                    }	
				}
                if(found == -1){
				    for(j = 0; j < num_programs; j++){//try finding a compute first
				        if(core_stats[findNeighborIndex(j)].type == SM_MATMUL){
                            if(core_stats[j].stat == BAD){
                                found = j;
                                break;
                            }
                            else if(core_stats[j].stat == WASTE){
                                found = j;
                            }
                        }	
				    }
                }
                if(found == -1){
                }
            }
            else if(core_stats[i].type == COMPUTE){//if it's a bad computem, try finding a lg, if not, streaming, then small
                int found = -1;
				for(j = 0; j < num_programs; j++){//try finding a compute first
				    if(core_stats[findNeighborIndex(j)].type == LG_MATMUL){
                        if(core_stats[j].stat == BAD){
                            found = j;
                            break;
                        }
                        else if(core_stats[j].stat == WASTE){
                            found = j;
                        }
                    }	
				}
                if(found == -1){
				    for(j = 0; j < num_programs; j++){//try finding a compute first
				        if(core_stats[findNeighborIndex(j)].type == SM_MATMUL){
                            if(core_stats[j].stat == BAD){
                                found = j;
                                break;
                            }
                            else if(core_stats[j].stat == WASTE){
                                found = j;
                            }
                        }	
				    }
                }
                if(found == -1){
				    for(j = 0; j < num_programs; j++){//try finding a compute first
				        if(core_stats[findNeighborIndex(j)].type == STREAMING){
                            if(core_stats[j].stat == BAD){
                                found = j;
                                break;
                            }
                            else if(core_stats[j].stat == WASTE){
                                found = j;
                            }
                        }	
				    }
                }
                if(found == -1){
                }
            }//end of searching for a swap for a bad
            //swap i with j
            //TODO:call swap function
            if(found != -1){
                printf("*****************************\n");
                printf("trying to swap %d with %d\n", i, found);
                printf("*****************************\n");
                swap_processes(i, found);
            }
		}
	}
	

	for(i = 0; i < num_programs; i++)
    {
		//if(shm_ptrs[i]!=NULL)
		//{
			//printf("Reading from Proc %d :", i);
			memcpy(&core_stats[i], &stats_ptrs[i], sizeof(stats_struct));
			//printf("%ld, %ld, %ld, %ld, %ld\n", core_stats[i].l2_cache_misses, core_stats[i].l2_cache_accesses, core_stats[i].num_instructions, core_stats[i].num_cycles, core_stats[i].num_ref_cycles);
		//}
	}

	return NULL;

}

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("global_scheduler [file with list of programs] \n");
		exit(1);
	}

	ifstream in(argv[1]);
	
	if(!in)
	{	
		printf("Input file not found\n");
		exit(1);
	}

	string str;
	num_programs = 0;
	int pos = 0;
	vector<string> programs;
	vector<string> args1;
	vector<int> initial_affinity;

	
	while(getline(in, str))
	{
		string delimiter = " "; 
		pos = str.find(delimiter);
		string token = str.substr(0, pos);
		programs.push_back(token);
		str.erase(0, pos + delimiter.length());
		
		pos = str.find(" ");
		token = str.substr(0, pos);
		args1.push_back(token);
		str.erase(0, pos + delimiter.length());
		
		pos = str.find(" ");
		token = str.substr(0, pos);
		initial_affinity.push_back(std::stoi(token));
		str.erase(0, pos + delimiter.length());
	}
	
	num_programs = programs.size();

	core_stats = new stats_struct[68];
	shmids = new int[68];
	core_mapping = new core_write_struct *[68];
	

	stats_ptrs = (stats_struct *) get_shared_ptr( "stats_pt", sizeof(stats_struct)*68, SHM_W, &shmid);

    stats_struct tryi = stats_ptrs[3];

	for(int i=0; i<68; i++)
	{
        bool created = false;
        for(int k=0; k<num_programs; k++) 
        {
            if(initial_affinity[k] == i)
            {
                core_mapping[k] = (core_write_struct *) get_shared_ptr( (char *) programs[k].c_str(), sizeof(core_write_struct), SHM_W, &shmids[k]);
                core_mapping[k]->core_write_id = i;
                //Add pid to stats_struct
                created= true;
                break;
            }
        }
    
	}

	//Launch all programs
	int pid = 0;
	int i = 0;
	cpu_set_t set;
	CPU_ZERO (&set);
	
	for(i=0; i<num_programs; i++)
	{
		pid = fork();
		if(pid==0) break;
        (&stats_ptrs[initial_affinity[i]])->pid = pid;
        (&stats_ptrs[initial_affinity[i]])->mapping_index = i;
	}
	
	if(pid == 0) 
	{
		CPU_SET((initial_affinity[i]), &set);
		if (sched_setaffinity(getpid(), sizeof(set), &set) == -1)
		{
			printf("Uable to set affinity to %d\n", initial_affinity[i]);
			exit(2);
		}

		printf("Child %d\n", i);
		char *child_argv[] = {(char *) programs[i].c_str(), (char *) args1[i].c_str(), NULL};
		printf("Child_argv %s %s\n", programs[i].c_str(), args1[i].c_str());
		int ret = execv(child_argv[0], child_argv);
		if(ret == -1) printf(" %s\n", strerror(errno));
	}
	else
	{
        struct timeval value = {1, 0};
        struct timeval interval = {0, GLOBAL_SCHED_QUANTUM};
        struct itimerval timer = {interval, value};
        
        signal(SIGALRM, (__sighandler_t) global_scheduler);
        setitimer(ITIMER_REAL, &timer, 0);
        
            
        
        for(int i=0; i<num_programs; i++)
        {
            int status;
            pid_t done = waitpid(-1, &status, 0);
            if(done!=0 || done!=-1)
            {
                printf("Completed app %d\n", done);
            }
        }
        
        //while (!child_scheds.empty())
        //{
        //    printf("Kill status: %d\n", kill(child_scheds.back(), SIGKILL));
        //    child_scheds.pop_back();
        //}
            
        detach_shared_mem(stats_ptrs);
        destroy_shared_mem(&shmid);

        for(int i=0; i<num_programs; i++)
        {
            detach_shared_mem(core_mapping[i]);
            destroy_shared_mem(&shmids[i]);
        }
	
	}



}
