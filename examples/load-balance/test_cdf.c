#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <math.h>

#include "cdf.h"


#define TG_GOODPUT_RATIO (1448.0 / (1500 + 14 + 4 + 8 + 12))

// Port from Traffic Generator
// Acknowledged to https://github.com/HKUST-SING/TrafficGenerator/blob/master/src/common/common.c
double poission_gen_interval(double avg_rate)
{
    if (avg_rate > 0)
       return -logf(1.0 - (double)rand() / RAND_MAX) / avg_rate;
    else
       return 0;
}

/* generate poission process arrival interval */
double poission_gen_interval_ORIG(double avg_rate)
{
	if (avg_rate > 0)
		return -logf(1.0 - (double)(rand() % RAND_MAX) / RAND_MAX) / avg_rate;
	else
		return 0;
}

int main()
{


	printf ("Initialize CDF Table\n");
	struct cdf_table *cdfTable = (struct cdf_table*)malloc(sizeof(struct cdf_table));//= cdf_table ();
	init_cdf (cdfTable);
	load_cdf (cdfTable, "DCTCP_CDF.txt");
	printf ("Average request size: %.1f bytes\n",  avg_cdf(cdfTable) );
	printf ("CDF Table\n");
	print_cdf (cdfTable);


	double SERVER_COUNT			= 16;
	double LEAF_SERVER_CAPACITY	= 1000000000;//10e9; // 1Gbps
	double SPINE_LEAF_CAPACITY	= 1000000000;//10e9; // 1Gbps
	double SPINE_COUNT			= 8;
	double LINK_COUNT			= 1;
	double load					= 0.9;

	printf ("Network Load\t: %.1f\n", load);

	double oversubRatio = (SERVER_COUNT * LEAF_SERVER_CAPACITY) / (SPINE_LEAF_CAPACITY * SPINE_COUNT * LINK_COUNT);
	printf ("Over-subscription ratio (server/leaf)\t: %.0f:1 \n" , oversubRatio);

	double requestRate = load * LEAF_SERVER_CAPACITY * SERVER_COUNT / oversubRatio / (8 * avg_cdf (cdfTable)) / SERVER_COUNT ;
	printf ("RequestRate\t: %.1f/sec\n" ,  requestRate);
	double sleep_us = poission_gen_interval(requestRate); 
	printf ("sleep_us\t: %.2f sec\n", sleep_us);
	int i;
	for (i=0; i<50; i++)
		printf ("%.2f, ", poission_gen_interval(requestRate));

	double period_us = (double)(avg_cdf(cdfTable) * 8 / load / TG_GOODPUT_RATIO); /* average request arrival interval (in microseconds) */
	printf ("period_us\t: %.2f/sec\n", (double)(period_us*(double)10e-9));
	double req_sleep_us = poission_gen_interval_ORIG(1.0/period_us); /* sleep interval based on poission process */
	printf ("req_sleep_us\t: %.2f sec\n", req_sleep_us*10e-9);
	for (i=0; i<50; i++)
		printf ("%.2f, ", poission_gen_interval_ORIG(1.0/period_us));

return 0;
}
