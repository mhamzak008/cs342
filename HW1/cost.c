#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

void pro()
{

}

int main()
{
	// Measuring Time cost for System Call
	clock_t sTimeSys;
	clock_t eTimeSys;
	double timeElapsedSys;

	sTimeSys = clock();

	getpid();

	eTimeSys = clock();

	timeElapsedSys = (eTimeSys - sTimeSys) / (CLOCKS_PER_SEC/1000000);

	printf("\nTime Cost for System Call: ");
	printf("%lf", timeElapsedSys);

	// Measuring Time cost for Procedural Call
	clock_t sTimePro;
	clock_t eTimePro;
	double timeElapsedPro;

	sTimePro = clock();

	pro();

	eTimePro = clock();

	timeElapsedPro = (eTimePro - sTimePro) / (CLOCKS_PER_SEC/1000000);

	printf("\nTime Cost for Procedural Call: ");
	printf("%lf", timeElapsedPro);



	printf("\n");
	return 0;
}