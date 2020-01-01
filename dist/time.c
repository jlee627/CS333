#ifdef CS333_P2
#include "types.h"
#include "user.h"

int
main(int argc, char * argv[])
{
  /*
  if(argc < 2)
  {
    printf(1,"Time took 1110.00 seconds.\n");
    exit();
  }
  */

  int start_time = uptime();
  int pid = fork();

  if(pid < 0)
    exit();

  else if(pid == 0){
    exec(argv[1], argv+1);
    exit();
  }

  else if(pid > 0){
    wait();
    
    int end_time = uptime();
    int total_time = end_time - start_time;

    int s  = total_time / 1000;
    int ms = total_time % 1000;

    if(ms >= 100)
      printf(1, "%s ran in %d.%d seconds.\n", argv[1],s,ms);
    else if(ms < 10)
      printf(1, "%s ran in %d.00%d seconds.\n", argv[1],s,ms);
    else
      printf(1, "%s ran in %d.0%d seconds.\n", argv[1],s,ms);
  }
  else
    exit();
  
  exit();
}
#endif // CS333_P2
