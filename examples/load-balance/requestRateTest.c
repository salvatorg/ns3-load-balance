// gcc requestRateTest.c cdf.c  -lm && ./a.out

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <math.h>

#include "cdf.h"

#define max(a,b) \
({ __typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a > _b ? _a : _b; })

#define TG_GOODPUT_RATIO (1448.0 / (1500 + 14 + 4 + 8 + 12))

// Port from Traffic Generator
// Acknowledged to https://github.com/HKUST-SING/TrafficGenerator/blob/master/src/common/common.c
double poission_gen_interval_Clove(double avg_rate)
{
    if (avg_rate > 0)
       return -logf(1.0 - (double)rand() / RAND_MAX) / avg_rate;
    else
       return 0;
}

/* generate poission process arrival interval */
double poission_gen_interval(double avg_rate)
{
	if (avg_rate > 0)
		return -logf(1.0 - (double)(rand() % RAND_MAX) / RAND_MAX) / avg_rate;
	else
		return 0;
}

int main()
{
	long double load					= 0.9;
	/* average request arrival interval (in microseconds) */
	long double period_us;
	long double req_total_num			= 0; /* total number of requests to generate */
	long double req_total_time 			= 4;    /* total time to generate requests (in seconds) */

	long double SERVER_COUNT			= 16;
	long double LEAF_SERVER_CAPACITY	= 10000;//in Mbps
	long double SPINE_LEAF_CAPACITY		= 10000;//in Mbps
	long double SPINE_COUNT				= 8;
	long double LINK_COUNT				= 1;

	long double oversubRatio = (SERVER_COUNT * LEAF_SERVER_CAPACITY) / (SPINE_LEAF_CAPACITY * SPINE_COUNT * LINK_COUNT);
	long double requestRate;

	printf ("Initialize CDF Table\n");
	struct cdf_table *cdfTable = (struct cdf_table*)malloc(sizeof(struct cdf_table));//= cdf_table ();
	init_cdf (cdfTable);
	load_cdf (cdfTable, "DCTCP_CDF.txt");
	/* get average value of CDF distribution */
	printf ("[CDF] Average value of CDF distribution: %.1f bits\n",  avg_cdf(cdfTable)*8 );
	// printf ("CDF Table\n");
	// print_cdf (cdfTable);
	printf ("Network Load\t: %.2Lf\n", load);

    /* calculate average request arrival interval */
    if (load > 0)
    {
		// load should be in Mbps here
		long double load_tg = 80000; // 10000Mbps=10Gbps
		/* average request arrival interval (in microseconds) */
		// printf ("[TG] TG_GOODPUT_RATIO: %.1f\n",TG_GOODPUT_RATIO);
        period_us = avg_cdf(cdfTable) * 8 / ( load_tg * load ) ;/// TG_GOODPUT_RATIO;
        if (period_us <= 0)
            printf ("[TG] Error: period_us is not positive\n");
		else
		{
			printf("->[TG] load: %.2Lf Gbps (%.2Lf Gbits for %.0Lf sec)\n", load_tg * load / 1e3 , req_total_time * load_tg * load / 1e3, req_total_time);
			printf("->[TG] period_us. request arrival interval(in us): %Lf us\n" , period_us);
			/* transfer time to the number of requests */
			req_total_num = max((long double)(req_total_time * 1e6) / period_us, 1);
			printf("[TG] Total number of requests: %.2Lf\n", req_total_num);
		}
        period_us = avg_cdf(cdfTable) * 8 / ( load * (LEAF_SERVER_CAPACITY * SERVER_COUNT / oversubRatio) ) ;
        if (period_us <= 0)
            printf ("[Clove] Error: period_us is not positive\n");
		else
		{
			printf("->[Clove] load: %.2Lf Gbps\n", ( load * ((LEAF_SERVER_CAPACITY * SERVER_COUNT) / oversubRatio) ) / 1e3);
			printf("->[Clove] period_us. request arrival interval(in us): %Lf us\n" , period_us);
			/* transfer time to the number of requests */
			req_total_num = max((long double)(req_total_time * 1000000) / period_us, 1);
			printf("[Clove] Total number of requests: %.2Lf\n", req_total_num);
		}
    }


	printf ("[Clove] Over-subscription ratio (server/leaf)\t: %Lf:1 \n" , oversubRatio);

	requestRate = load * LEAF_SERVER_CAPACITY * 1e6 * SERVER_COUNT / oversubRatio / (8 * avg_cdf (cdfTable)) /SERVER_COUNT ;
	printf ("[Clove] period_us. request arrival interval: %Lf/s\n" ,  requestRate);

	// req_total_num = req_total_time * 1e6 / requestRate ;
	// printf("[Clove] Total number of requests: %Lf\n", req_total_num);

	// double sleep_us = poission_gen_interval(requestRate); 
	// printf ("sleep_us\t: %.2f sec\n", sleep_us);
	// int i;
	// for (int i=0; i<50; i++)
	// 	printf ("%f, %f\n", poission_gen_interval(1/period_us)/1000000, poission_gen_interval_Clove(requestRate));

	printf ("==== Based on FLOW_LAUNCH_END_TIME (%0.Lfsec) ====\n", req_total_time);
	double time=0;
	uint cnt=0;
	double size=0;
	while (time<=req_total_time)
	{
		time += poission_gen_interval(1/period_us) / 1e6;
		cnt++;
		size += gen_random_cdf (cdfTable);
	}
	printf ("STATS:\t\t\t%d flows,\t%.2f Gbits,\t%.2f GBytes\n",cnt , size*8*1e-9, size*1e-9);

	time=0;
	cnt=0;
	size=0;
	while (time<=req_total_time)
	{
		time += poission_gen_interval_Clove(requestRate);
		cnt++;
		size += gen_random_cdf (cdfTable);
	}
	printf ("STATS(One Server):\t%d flows,\t%.2f Gbits,\t%.2f GBytes\n",cnt , size*8*1e-9, size*1e-9);
	printf ("STATS(%.0Lf Servers):\t%d flows,\t%.2f Gbits,\t%.2f GBytes\n",SERVER_COUNT, cnt*(uint)SERVER_COUNT , size*8*(uint)SERVER_COUNT*1e-9, size*(uint)SERVER_COUNT*1e-9);

	// // period_us = (double)(avg_cdf(cdfTable) * 8 / load / TG_GOODPUT_RATIO); /* average request arrival interval (in microseconds) */
	// printf ("period_us\t: %.2f/sec\n", (double)(period_us*(double)10e-9));
	// double req_sleep_us = poission_gen_interval_ORIG(1.0/period_us); /* sleep interval based on poission process */
	// printf ("req_sleep_us\t: %.2f sec\n", req_sleep_us*10e-9);
	// for (i=0; i<50; i++)
	// 	printf ("%.2f, ", poission_gen_interval_ORIG(1.0/period_us));

return 0;
}
