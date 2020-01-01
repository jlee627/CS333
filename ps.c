#ifdef CS333_P2
#include "types.h"
#include "user.h"
#include "uproc.h"

int
main(int argc, char * argv[])
{
  int max = 16;

  if(argc > 1)
    max = atoi(argv[1]);

  struct uproc * table = malloc(sizeof(*table)*max);

  if(table == 0){
    printf(1,"unable to make table \n");
    exit();
  }

  int num_procs = getprocs(max, table);
  uint s;
  uint ms;
#ifdef CS333_P4
  printf(1, "PID\tName\tUID\tGID\tPPID\tPrio\tElapsed\tCPU\tState\tSize\n");
#elif CS333_P2
  printf(1, "PID\tName\tUID\tGID\tPPID\tElapsed\tCPU\tState\tSize\n");
#endif

  for(int i =0; i < num_procs; i++){
#ifdef CS333_P4
    // Print PID, Name, UID, GID, PPID, Priority  
    printf(1, "%d\t%s\t%d\t%d\t%d\t%d\t", table[i].pid, table[i].name, table[i].uid, table[i].gid, table[i].ppid, table[i].priority);
#elif CS333_P2
    // Print PID, Name, UID, GID, PPID
    printf(1, "%d\t%s\t%d\t%d\t%d\t", table[i].pid, table[i].name, table[i].uid, table[i].gid, table[i].ppid);
#endif // End of if/else for output

    // Calculate Elapsed time
    s  = table[i].elapsed_ticks / 1000;
    ms = table[i].elapsed_ticks % 1000;

    if(ms >= 100)
      printf(1, "%d.%d\t",s,ms);
    else if(ms < 10)
      printf(1, "%d.00%d\t",s,ms);
    else
      printf(1, "%d.0%d\t",s,ms);

    // Calculate CPU time
    s  = table[i].CPU_total_ticks / 1000;
    ms = table[i].CPU_total_ticks % 1000;
    
    if(ms >= 100)
      printf(1, "%d.%d\t",s,ms);
    else if(ms < 10)
      printf(1, "%d.00%d\t",s,ms);
    else
      printf(1, "%d.0%d\t",s,ms);

    // Print state, size
    printf(1, "%s\t%d\n", table[i].state,table[i].size);
  }

  free(table);
  exit();
}
#endif
