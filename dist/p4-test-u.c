/*----------------------------------------------+
| Created by: Jacob Collins <jmc27@pdx.edu>     |
| Shared by : Ryan Albazzaz <haider3@pdx.edu>   |
| Updated by : Jaime Landers <jlanders@pdx.edu> |
+----------------------------------------------*/
// Changelog:
// 5/29/19 - Cleaned up code, fixed printing, menu, etc. by jlanders
// 5/29/19 - Added test setpriority by jlanders
// 6/1/19 - Added getpriority tests by jlanders

#ifdef CS333_P4

#include "types.h"
#include "user.h"
#include "param.h"

#define DEFAULT_BUDGET BUDGET
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

// Forward Declarations:
void cleanupProcs(int pid[], int max);
int createInfiniteProc();
int createSetPrioProc(int prio);
int menu();
void sleepMessage(int time, char message[]);
void test1();
void test2();
void test3();
void test4();
void test5();
void test6();
void test7();
void test8();

/* Note:
     You may need to change the names of MAXPRIO and BUDGET to match the
     names you have defined in your code.
*/
const int plevels = MAXPRIO;
const int budget = DEFAULT_BUDGET;
const int promo = TICKS_TO_PROMOTE;

void
cleanupProcs(int pid[], int max)
{
  int i;
  for(i = 0; i < max; i++)
    kill(pid[i]);
  while(wait() > 0);
}

int
createInfiniteProc()
{
  int pid = fork();
  if(pid == 0)
    while(1);
  printf(1, "Process %d created...\n", pid);

  return pid;
}

int
createSetPrioProc(int prio)
{
  int pid = fork();
  if(pid == 0)
    while(1)
      setpriority(getpid(), prio);

  return pid;
}

int
main(int argc, char* argv[])
{
  printf(1, "\nWelcome to p4 Test Suite!\n");
  printf(1, "p4test starting with: MAXPRIO = %d, DEFAULT_BUDGET = %d, TICKS_TO_PROMOTE = %d\n",
      plevels, budget, promo);

  while(menu() != 0);
  printf(1, "\nGoodbye!\n");

  exit();
}

int
menu(void)
{
  int select;
  char buf[5];

  while(1) {
    int i = 1;
    printf(1, "\n%d. Round robin scheduling for %d queues.\n", i++, plevels);
    printf(1, "%d. Promotion should change priority levels for sleeping and running processes.\n", i++);
    printf(1, "%d. Test demotion resetting budget.\n", i++);
    printf(1, "%d. Test scheduler operation.\n", i++);
    printf(1, "%d. Test demotion setting priority level.\n", i++);
    printf(1, "%d. Test scheduler selection, promotion, and demotion.\n", i++);
    printf(1, "%d. Test setpriority() \n", i++);
    printf(1, "%d. Test getpririty() \n", i++);
    printf(1, "%d. Exit program\n", 0);
    printf(1, "Enter test number: ");
    gets(buf, 5);

    if ((buf[0] == '\n') || buf[0] == '\0') {
      continue;
    }

    select = atoi(buf);

    switch(select) {
      case 0:
        return 0;
      case 1:
        test1();
        break;
      case 2:
        test2();
        break;
      case 3:
        test3();
        break;
      case 4:
        test4();
        break;
      case 5:
        test5();
        break;
      case 6:
        test6();
        break;
      case 7:
        test7();
        break;
      case 8:
        test8();
        break;
      default:
        printf(1, "Error: invalid test number.\n");
        break;
    }
  }
}

void
sleepMessage(int time, char message[])
{
  printf(1, message);
  sleep(time);
}

void
test1()
{
  int i;
  int max = 8;
  int pid[max];

  printf(1, "+=+=+=+=+=+=+=+=+\n");
  printf(1, "| Start: Test 1 |\n");
  printf(1, "+=+=+=+=+=+=+=+=+\n");

  for(i = 0; i < max; i++){
    pid[i] = createInfiniteProc();
  }

//  printf(1, "Setting all created process priorities to 0 to test promotion \n");
//  for(i = 0; i < max; i++){
//    setpriority(pid[i], 0) ;
//  }

  for(i = 0; i < 3; i++)
    sleepMessage(2000, "Sleeping... ctrl-p\n");

  cleanupProcs(pid, max);

  printf(1, "+=+=+=+=+=+=+=+\n");
  printf(1, "| End: Test 1 |\n");
  printf(1, "+=+=+=+=+=+=+=+\n");
}

void
test2()
{
  int i;
  int pid[2];
  pid[0]  = createInfiniteProc();

  printf(1, "+=+=+=+=+=+=+=+=+=\n");
  printf(1, "| Start: Test 2a |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");

  setpriority(getpid(), plevels);

  i = 0;
  while(i <= plevels) {
    sleepMessage(2000, "Sleeping... ctrl-p\n");
    i++;
  }

  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 2a |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");
  printf(1, "| Start: Test 2b |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");

  pid[1] = createSetPrioProc(0);
  setpriority(pid[0], plevels);

  i = 0;
  while(i <= plevels) {
    sleepMessage(2000, "Sleeping... ctrl-p\n");
    i++;
  }

  cleanupProcs(pid, 2);

  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 2b |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");
  printf(1, "| Start: Test 2c |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");

  int cPid = getpid();
  int rc = 0;

  printf(1, "Setting priority of the current process to %d... ", plevels);

  rc = setpriority(cPid, plevels);

  if (rc == -1) {
    printf(2, "setpriority to %s failed!\n\tThis test FAILED.\n", plevels);
    goto end;
  }

  rc = getpriority(cPid);

  if (rc != plevels) {
    printf(2, "getpriority failed! %d != %d\n\tThis test FAILED.\n", plevels, rc);
    goto end;
  }

  printf(1, "This test PASSES.\n");

end:
  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 2c |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");
}

void
test3()
{
  printf(1, "+=+=+=+=+=+=+=+=+\n");
  printf(1, "| Start: Test 3 |\n");
  printf(1, "+=+=+=+=+=+=+=+=+\n");

  int i;
  int max = 8;
  int pid[max];

  for(i = 0; i < max; i++)
    pid[i] = createInfiniteProc();

  for(i = 0; i <= plevels; i++)
    sleepMessage(2000, "Sleeping... ctrl-p OR ctrl-r\n");

  cleanupProcs(pid, max);

  printf(1, "+=+=+=+=+=+=+=+\n");
  printf(1, "| End: Test 3 |\n");
  printf(1, "+=+=+=+=+=+=+=+\n");

}

void
test4()
{
  printf(1, "+=+=+=+=+=+=+=+=+\n");
  printf(1, "| Start: Test 4 |\n");
  printf(1, "+=+=+=+=+=+=+=+=+\n");

  int i;
  int max = 10;
  int pid[max];

  for(i = 0; i < max/2; i++)
    pid[i] = createSetPrioProc(plevels);
  for(i = max/2; i < max; i++)
    pid[i] = createSetPrioProc(0);

  for(i = 0; i < 2; i++)
    sleepMessage(2000, "Sleeping... ctrl-p\n");

  cleanupProcs(pid, max);

  printf(1, "+=+=+=+=+=+=+=+\n");
  printf(1, "| End: Test 4 |\n");
  printf(1, "+=+=+=+=+=+=+=+\n");
}

void
test5()
{
  printf(1, "+=+=+=+=+=+=+=+=+\n");
  printf(1, "| Start: Test 5 |\n");
  printf(1, "+=+=+=+=+=+=+=+=+\n");

  int i, prio;
  int max = 10;
  int pid[max];

  for(i = 0; i < max; i++) {
    prio = i%(plevels+1);
    pid[i] = createSetPrioProc(prio);
    printf(1, "Process %d will be at priority level %d\n", pid[i], prio);
  }

  sleepMessage(2000, "Sleeping... ctrl-p\n");
  sleepMessage(2000, "Sleeping... ctrl-r\n");

  cleanupProcs(pid, max);
  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 5 |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");
}

void
test6()
{
  printf(1, "+=+=+=+=+=+=+=+=+=\n");
  printf(1, "| Start: Test 6a |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");

  int i;
  int max = 8;
  int pid[max];

  for(i = 0; i < max/2; i++)
    pid[i] = createInfiniteProc();

  for(i = 0; i <= plevels; i++)
    sleepMessage(2000, "Sleeping... ctrl-p OR ctrl-r\n");
  if(plevels == 0)
    sleepMessage(2000, "Sleeping... ctrl-p OR ctrl-r\n");

  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 6a |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");

  printf(1, "+=+=+=+=+=+=+=+=+=\n");
  printf(1, "| Start: Test 6b |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");

  for(i = max/2; i < max; i++)
    pid[i] = createInfiniteProc();

  for(i = 0; i <= plevels+1; i++)
    sleepMessage(2000, "Sleeping... ctrl-p OR ctrl-r\n");

  cleanupProcs(pid, max);

  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 6b |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");
}

void
test7()
{
  int i = 0;
  int max = 8;
  int pid[max];
  int rc = 0;

  printf(1, "+=+=+=+=+=+=+=+=+=\n");
  printf(1, "| Start: Test 7a |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");

  for(i = 0; i < max; i++)
    pid[i] = createInfiniteProc();

  for(i = 0; i <= plevels+1; i++)
    sleepMessage(1000, "Sleeping... ctrl-p OR ctrl-r\n");

  printf(1, "Using setpriority() to reset priority to MAXPRIO\nCalling: \n");
  for(i = 0; i < max; i++) {
    printf(1, "setpriority(%d, %d)\n", pid[i], MAXPRIO);
    setpriority(pid[i], MAXPRIO);
  }

  sleepMessage(2000, "Use ctrl-p and ctrl-r\n");

  cleanupProcs(pid, max);

  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 7a |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");

  printf(1, "\n+=+=+=+=+=+=+=+=+=\n");
  printf(1, "| Start: Test 7b |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");

  for(i = 0; i < max; i++)
    pid[i] = createInfiniteProc();

  printf(1, "\nUsing setpriority() to set priority to same priority it had previously\n");
  sleepMessage(2000, "Sleeping first... ctrl-p and ctrl-r\n");
  printf(1, "Calling:\n");

  for(i = 0; i < max; i++) {
    int tpid = getpriority(pid[i]);
    printf(1, "setpriority(%d, %d)\n", pid[i], tpid);
    setpriority(pid[i], tpid);
  }
  sleepMessage(2000, "Use ctrl-p and ctrl-r\n");

  cleanupProcs(pid, max);

  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 7b |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");

  printf(1, "\n+=+=+=+=+=+=+=+=+=\n");
  printf(1, "| Start: Test 7c |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");

  printf(1, "\nUsing setpriority() to set priority to invalid PID \n");
  rc = setpriority(-1, MAXPRIO);
  printf(1, "setpriority(-1, %d) returned %d\n", MAXPRIO, rc);
  if(rc != 0)
    printf(1, "TEST PASSED\n\n");
  else
    printf(1, "TEST FAILED\n\n");

  printf(1, "\nUsing setpriority() to set priority out of range \n");

  for(i = 0; i < max/2; i++)
    pid[i] = createInfiniteProc();

  sleepMessage(2000, "Use ctrl-p and ctrl-r to show priority and budget before calling setpriority()\n");

  int err = 0;

  for(i = 0; i < max/2; i++){
    rc = setpriority(pid[i], MAXPRIO + 1);
    printf(1, "setpriority(%d, %d) returned %d\n", pid[i], MAXPRIO + 1, rc);
    if (rc != 0)
      err = rc;
  }

  if(err != 0)
    printf(1, "TEST PASSED\n");
  else
    printf(1, "TEST FAILED\n");

  sleepMessage(5000, "Use ctrl-p and ctrl-r a few times to show priority and budget after setpriority()\n");

  cleanupProcs(pid, max);

  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 7c |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");

}

void
test8()
{
  int i = 0;
  int max = 2;
  int pid[max];
  int rc = 0;

  printf(1, "\n+=+=+=+=+=+=+=+=+=\n");
  printf(1, "| Start: Test 8 |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");

  for(i = 0; i < max/2; i++)
    pid[i] = createInfiniteProc();

  printf(1, "\nThe current process' pid is %d\n", pid[0]);

  rc = getpriority(pid[0]);
  printf(1, "getpriority(%d) returned %d\n", pid[0], rc);

  sleepMessage(2000, "Use ctrl-p to show priorities match \n");

  int ppid = getppid();

  printf(1, "\nThe parent process' pid is %d\n", ppid);
  rc = getpriority(ppid);
  printf(1, "getpriority(%d) returned %d\n", ppid, rc);

  sleepMessage(2000, "Use ctrl-p to show priorities match \n");

  printf(1, "\nTesting getpriority on invalid PID");
  rc = getpriority(-1);
  printf(1, "\ngetpriority(%d) returned %d\n", -1, rc);

  if (rc != 0)
    printf(1, "TEST PASSED\n");
  else
    printf(1, "TEST FAILED\n");

  cleanupProcs(pid, max);

  printf(1, "\n+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 8 |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");
}

#endif
