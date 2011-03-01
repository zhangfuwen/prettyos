#include "userlib.h"
#include "stdio.h"


size_t failures = 0;

void testFailed()
{
	failures++;
	textColor(0x0C);
	printf(" Test failed!");
	textColor(0x0F);
}

void testPassed()
{
	textColor(0x0A);
	printf(" Test passed!");
	textColor(0x0F);
}


///mutex_t* taskingTestProtector = 0;
void taskingTest();
int main()
{
	textColor(0x0E);
	printf("*UNDER CONSTRUCTION*   Welcome to the PrettyOS test suite   *UNDER CONSTRUCTION*");
	textColor(0x07);
	printf("\n\nThe suite is divided into several part. The system will count the number of failures to print it at the end. The goal is to have no failures. For some tests, when the system is not able to determine whether it failed or not, it will ask you. Please answer 'f' if the test failed or any other key otherwise. If the system immediatly terminates the test, reboots or shows error messages, it has obviously failed.");


	textColor(0x0E);
	printf("\n\n\nVIDEO");
	textColor(0x0F);
	printf("\nFirst of all, we will test the text capabilities of PrettyOS.");
	printf("\nDo you see the text output above and does it use different colors?");
	if(getchar() == 'f') testFailed();
	else testPassed();


	textColor(0x0E);
	printf("\n\n\nTASKING");
	textColor(0x0F);
	printf("\nAfter having examined whether you are able to even see the test results, we will now go on with the interna of the OS. The OS will now create a thread in a different console.");
	taskingTest();
	printf("\nHas a new, empty console appeared?");
	if(getchar() == 'f') testFailed();
	else testPassed();

	printf("\n\nNow the OS tests the mutexes. In the console that has just appeared, you should see strings of 'A's and 'B's.");
	///mutex_unlock(taskingTestProtector); // Let the tasking test thread step further
	printf("\nDoes each of those strings consist of 20 equal letters?");
	if(getchar() == 'f') testFailed();
	else testPassed();
	///mutex_unlock(taskingTestProtector); // Let the tasking test thread step further to free ressources it allocated


	printf("\n\n\nAll tests Finished.");
	textColor(0x09);
	printf("\n\nResult: ");
	if(failures == 0) textColor(0x0A);
	else textColor(0x0C);
	printf("%u", failures);

	textColor(0x0F);
	printf("\n\nPress any key to continue...");
	getchar();

	return 0;
}


///mutex_t* taskingTestMutex;
bool taskingTestRunning = true;
void taskingTestFunc1()
{
	while(taskingTestRunning)
	{
		///mutex_lock(taskingTestMutex); // Let only one of the threads inside the critical section
		for(int i = 0; i < 20; i++)
			printf("A");
		///mutex_unlock(taskingTestMutex);
	}
}
void taskingTestFunc2()
{
	while(taskingTestRunning)
	{
		///mutex_lock(taskingTestMutex); // Let only one of the threads inside the critical section
		for(int i = 0; i < 20; i++)
			printf("B");
		///mutex_unlock(taskingTestMutex);
	}
}
void taskingTestFunc() // TODO: Check if it gives different results with and without mutexes
{
	///mutex_lock(taskingTestProtector); // Halt the task until the main function steps to the next test
	///taskingTestMutex = mutex_create();

	for(int i = 0; i < 5; i++)
	{
		///CreateThread(&taskingTestFunc1);
		///CreateThread(&taskingTestFunc2);
	}
	taskingTestFunc1(); // Use this task as the eleventh instance of the testing tasks
}
void taskingTest()
{
	///taskingTestProtector = mutex_create();
	///mutex_lock(taskingTestProtector); // Halt the created task until the main function steps to the next test
	///CreateConsoleThread(&taskingTestFunc);

	///mutex_lock(taskingTestProtector); // Halt the created task until the main function steps to the next test
	taskingTestRunning = false;
	sleep(100); // Wait to give the other tasks enough time to finish
	///mutex_free(taskingTestMutex);
	///mutex_free(taskingTestProtector);
}