/*=========================== 
| Test Program for CS333_P3 |
| Created: 2/15/2019        |
| Written by Cole Christian |
| Shared by Evan Kent       |
===========================*/
// Changelog:
// Fixed Round Robin Test cleaning children by Evan Johnson 5/10/19
// Added menu functionality in place of command line arguments, and fixed indentation by Jaime Landers 5/10/19
// Added Exit/Wait test, fixed Kill/Wait Test prompting for Ctrl-r instead of Ctrl-s, fixed Round Robin Test not reaping zombies by Jaime Landers 5/11/19
// Feel free to make any needed changes
 
#ifdef CS333_P3
 
#include "types.h"
#include "user.h"
 
void
controlFTest(void)
{
  int n;
  int pid;
  int times;
 
  times = 4;
 
  for(n=1; n < times; ++n){
    printf(1,"\nFork Call # %d\n",n);
    pid = fork();
    if(pid < 0){
      printf(1,"Failure in %s %s line %d",__FILE__, __FUNCTION__,__LINE__);
    }
    if(pid == 0){
      printf(1, "Child Process %d is now sleeping for %d seconds. Use Control-p followed by Control-f within 5 sec\n", n, times*5);
      sleep(5*(times)*TPS);
      exit();
    }
    else{
      sleep(5*TPS);
    }
  }
  if(pid != 0){
    for(n=1; n < times; ++n){
      wait();
      printf(1,"Child Process %d has exited.\n",n);
    }
  }
}
 
void
controlSTest(void)
{
  int n;
  int pid;
  int times;
 
  times = 4;
 
  for(n=1; n < times; ++n){
    printf(1,"\nFork Call # %d\n",n);
    pid = fork();
    if(pid < 0){
      printf(1,"Failure in %s %s line %d",__FILE__, __FUNCTION__,__LINE__);
    }
    if(pid == 0){
      printf(1, "Child Process %d is now sleeping for %d seconds. Use Control-p followed by Control-s within 5 sec\n", n, times*5);
      sleep(5*(times)*TPS);
      exit();
    }
    else{
      sleep(5*TPS);
    }
  }
  if(pid != 0){
    for(n=1; n < times; ++n){
      wait();
      printf(1,"Child Process %d has exited.\n",n);
    }
  }
}
 
void
controlZTest(void)
{
  int n;
  int pid;
  int times;
 
  times = 4;
 
  for(n=1; n < times; ++n){
    printf(1,"\nFork Call # %d\n",n);
    pid = fork();
    if(pid < 0){
      printf(1,"Failure in %s %s line %d",__FILE__, __FUNCTION__,__LINE__);
    }
    if(pid == 0){
      printf(1, "Child Process %d has exited. Use Control-p followed by Control-z within 5 sec\n", n);
      exit();
    }
    else{
      sleep(5*TPS);
    }
  }
  if(pid != 0){
    sleep(5*TPS);
    for(n=1; n < times; ++n){
      wait();
      printf(1,"Wait() has been called on Child Process %d.\n",n);
    }
  }
}
 
void
controlRTest(void)
{
  int n;
  int pid;
  int times;
 
  times = 10;
 
  for(n=1; n < times; ++n){
    printf(1,"\nFork Call # %d\n",n);
    pid = fork();
    if(pid < 0){
      printf(1,"Failure in %s %s line %d",__FILE__, __FUNCTION__,__LINE__);
    }
    if(pid == 0){
      printf(1, "Child Process %d is running for %d seconds. Use Control-p followed by Control-r within 5 sec\n", n, 5*times);
      int i;
      i = uptime();
      while((uptime()-i) < (1000*5*times)){
      }
      exit();
    }
    else{
      sleep(5*TPS);
    }
  }
  if(pid != 0){
    for(n=1; n < times; ++n){
      wait();
      printf(1,"Wait() has been called on Child Process %d.\n",n);
    }
  }
}
 
void
exitTest(void)
{
  int pid;
  pid = fork();
  if(pid < 0){
    printf(1,"Failure in %s %s line %d",__FILE__, __FUNCTION__,__LINE__);
  }
  if(pid == 0){
    printf(1, "Child Process is running for 5 seconds. Use Control-p and z to show this. \nAfter 5 seconds, the process will be killed.\n");
    int i;
    i = uptime();
    while((uptime()-i) < (1000*5)){
    }
    exit();
  }
  else{
    sleep(8*TPS);
    printf(1, "Child Process %d has been killed using exit(). Use control-p and z to show that its on the zombie list. You have 5 sec\n", pid);
    sleep(5*TPS);
    wait();
    printf(1,"Wait() has been called on Child Process %d. Use control-p, z, f to show that is removed from zombie list and added to unused.\nYou have 10 sec\n",pid);
    sleep(10*TPS);
  }
}
 
void
killTest(void)
{
  int pid;
  pid = fork();
  if(pid < 0){
    printf(1,"Failure in %s %s line %d",__FILE__, __FUNCTION__,__LINE__);
  }
  if(pid == 0)
  {
    printf(1, "Child Process is looping forever. Use Control-p and z to show this.\nAfter 5 seconds, the process will be killed.\n");
    while(1){}
  }
  else{
    sleep(8*TPS);
    kill(pid);
    printf(1, "Child Process %d has been killed using kill(). Use control-p and z to show that its on the zombie list. You have 5 sec\n", pid);
    sleep(5*TPS);
    wait();
    printf(1,"Wait() has been called on Child Process %d. Use control-p, z, f to show that is removed from zombie list and added to unused.\nYou have 10 sec\n",pid);
    sleep(10*TPS);
  }
}
 
void
sleepWakeTest(void)
{
  int pid;
  pid = fork();
  if(pid < 0){
    printf(1,"Failure in %s %s line %d",__FILE__, __FUNCTION__,__LINE__);
  }
  if(pid == 0)
  {
    printf(1, "Child Process is now sleeping for %d seconds. Use Control-p followed by Control-s within 5 sec\n",5);
    sleep(5*TPS);
    printf(1, "Child Process is looping forever. Use Control-p and s to show that the child is removed from the sleeping list.\nAfter 5 seconds, the process will be killed.\n");
    while(1){}
  }
  else{
    sleep(8*TPS);
    kill(pid);
    printf(1, "Child Process %d has been killed. Use control-p and z to show that its on the zombie list. You have 5 sec\n", pid);
    sleep(5*TPS);
    wait();
    printf(1,"Wait() has been called on Child Process %d. Use control-p, z, f to show that is removed from zombie list and added to unused.\nYou have 10 sec\n",pid);
    sleep(10*TPS);
  }
}
 
// This is meant to be a drop-in replacement for
// the roundRobinTest() in p3-test-Evan-Kent.c
//
// credit to Cole Christian for the original code
// and Evan Kent for sharing with the class
//
// modified by Evan Johnson
// 10 May 2019
 
void
roundRobinTest(void) {
  int n;
  int pid;
  int times;
  int count;
 
  count = 0;
  times = 20;
 
  int children[times]; // to track child PIDs
 
  for (n = 0; n < times; ++n) {
    pid = fork();
    if (pid < 0) {
      printf(1, "Failure in %s %s line %d", __FILE__, __FUNCTION__, __LINE__);
      // clean up and exit on fork error
      for (int i = 0; i < count; ++i) {
        kill(children[i]);
        exit();
      }
    }
    if (pid == 0) {
      // spin forever (children never fork)
      while (1) {}
    }
    // track the new child
    children[n] = pid;
    ++count;
  }
 
  // children are stuck in loops, so only the original
  // will run this code
  printf(1, "%d Child Processes Created and are looping forever. Parent is now sleeping for 30 sec. Use control-r rapidly", count);
  sleep(30 * TPS);
  for (int i = 0; i < times; ++i) {
    // kill the child processes
    kill(children[i]);
    printf(1, "Killed child process %d with PID %d.\n", i, children[i]);
    wait();
  }
}
 
int
menu()
{
  int test = -1;
  char choice[1];
 
  printf(1,"\nChoose an option from the following:\n\n1. Control-f Test\n2. Control-s Test\n3. Control-z Test\n4. Control-r Test\n5. Kill() and Wait() Test\n6. Exit() and Wait() Test\n7. Sleep/Wake Test\n8. Round Robin Test\n9. Exit Test Suite\n**Feel free to make any changes\n");
  printf(1, "\nEnter selection: ");
 
  gets(choice, 2);
 
  test = atoi(choice);
 
  switch(test){
    case 1:
      printf(1,"\n----------- TEST 1 Control-f ----------\n");
      controlFTest();
      printf(1,"\n---------- TEST 1 COMPLETE ----------\n");
      break;
    case 2:
      printf(1,"\n----------- TEST 2 Control-s ----------\n");
      controlSTest();
      printf(1,"\n---------- TEST 2 COMPLETE ----------\n");
      break;
    case 3:
      printf(1,"\n----------- TEST 3 Control-z ----------\n");
      controlZTest();
      printf(1,"\n---------- TEST 3 COMPLETE ----------\n");
      break;
    case 4:
      printf(1,"\n----------- TEST 4 Control-r ----------\n");
      controlRTest();
      printf(1,"\n---------- TEST 4 COMPLETE ----------\n");
      break;
    case 5:
      printf(1,"\n----------- TEST 5 Kill() and Wait() ----------\n");
      killTest();
      printf(1,"\n---------- TEST 5 COMPLETE ----------\n");
      break;
    case 6:
      printf(1,"\n----------- TEST 6 Exit() and Wait() ----------\n");
      exitTest();
      printf(1,"\n---------- TEST 6 COMPLETE ----------\n");
      break;
    case 7:
      printf(1,"\n----------- TEST 7 Sleep/Wake Test ----------\n");
      sleepWakeTest();
      printf(1,"\n---------- TEST 7 COMPLETE ----------\n");
      break;
    case 8:
      printf(1,"\n----------- TEST 8 Round Robin Test ----------\n");
      roundRobinTest();
      printf(1,"\n---------- TEST 8 COMPLETE ----------\n");
      break;
    case 9:
      printf(1,"\n----------- Exiting P3-Test Suite ----------\n");
      return 0;
    default:
      printf(1,"\nYou did not enter a valid option, try again...\n");
      break;
 
  }
 
  gets(choice, 2); // Fix for stream
  return 1;
}
 
int
main()
{
  printf(1, "\n----------- Welcome to P3-Test Suite -----------\n");
 
  while (menu() != 0);
 
  exit();
}
 
#endif // CS333_P3
