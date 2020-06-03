#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define MARKING_THR_10G 91000	// 91K Bytes, 65 packets
#define MARKING_THR_1G 28000	// 28K bytes, 20 packets

#if !(defined _POSIX_C_SOURCE)
typedef long int ssize_t;
#endif

/* Only include our version of getline() if the POSIX version isn't available. */

#if !(defined _POSIX_C_SOURCE) || _POSIX_C_SOURCE < 200809L

#if !(defined SSIZE_MAX)
#define SSIZE_MAX (SIZE_MAX >> 1)
#endif

ssize_t getline(char **pline_buf, size_t *pn, FILE *fin)
{
  const size_t INITALLOC = 16;
  const size_t ALLOCSTEP = 16;
  size_t num_read = 0;

  /* First check that none of our input pointers are NULL. */
  if ((NULL == pline_buf) || (NULL == pn) || (NULL == fin))
  {
    errno = EINVAL;
    return -1;
  }

  /* If output buffer is NULL, then allocate a buffer. */
  if (NULL == *pline_buf)
  {
    *pline_buf = malloc(INITALLOC);
    if (NULL == *pline_buf)
    {
      /* Can't allocate memory. */
      return -1;
    }
    else
    {
      /* Note how big the buffer is at this time. */
      *pn = INITALLOC;
    }
  }

  /* Step through the file, pulling characters until either a newline or EOF. */

  {
    int c;
    while (EOF != (c = getc(fin)))
    {
      /* Note we read a character. */
      num_read++;

      /* Reallocate the buffer if we need more room */
      if (num_read >= *pn)
      {
        size_t n_realloc = *pn + ALLOCSTEP;
        char * tmp = realloc(*pline_buf, n_realloc + 1); /* +1 for the trailing NUL. */
        if (NULL != tmp)
        {
          /* Use the new buffer and note the new buffer size. */
          *pline_buf = tmp;
          *pn = n_realloc;
        }
        else
        {
          /* Exit with error and let the caller free the buffer. */
          return -1;
        }

        /* Test for overflow. */
        if (SSIZE_MAX < *pn)
        {
          errno = ERANGE;
          return -1;
        }
      }

      /* Add the character to the buffer. */
      (*pline_buf)[num_read - 1] = (char) c;

      /* Break from the loop if we hit the ending character. */
      if (c == '\n')
      {
        break;
      }
    }

    /* Note if we hit EOF. */
    if (EOF == c)
    {
      errno = 0;
      return -1;
    }
  }

  /* Terminate the string by suffixing NUL. */
  (*pline_buf)[num_read] = '\0';

  return (ssize_t) num_read;
}

#endif


int main(int argc, char* argv[])
{
	FILE *fp = NULL;
	uint threshold=0;
    if (argc == 2 || argc == 3)
	{
		if( access( argv[1], F_OK ) != -1 ) {;}
		else 
		{
				fprintf(stderr, "error: FILE doesnt exist\n");
				return EXIT_FAILURE;
		}
         fp = fopen(argv[1], "r");
		 // TODO needs to be a filename NOT A PATH
		 if(argv[argc-1][1]=='0' && argv[argc-1][0]=='1' && argv[argc-1][2]=='G')
		 	threshold = MARKING_THR_10G;
		 else if(argv[argc-1][0]=='1' && argv[argc-1][1]=='G')
		 	threshold = MARKING_THR_1G;
		//  else if(argc == 3)
		//  	threshold = atoi(argv[2]);
		 else
		 {
			 printf("### COULDNT SET THE MARKING THRESHOLD ###");
			 return EXIT_FAILURE;
		 }
	}
    else 
	{
         fprintf(stderr, "error: wrong number of arguments\n"
                         "usage: %s textfile\n", argv[0]);
         return EXIT_FAILURE;
    }


  /* Open the file for reading */
  char *line_buf = NULL;
  size_t line_buf_size = 0;
  int line_count = 0;
  ssize_t line_size;

  /* Get the first line of the file. */
  line_size = getline(&line_buf, &line_buf_size, fp);
  /* Get the first line and tokenize it. */
  char * pch;
  uint row_idx=0;
  char * tmp_line_buf;
  uint class=0;
  uint cnt_elem=0;

  char * flow_timestamp;
  char * queue_timestamp;
  char flow_path_str[3];
  uint flow_path;
  uint flow_log_cnt=0;
  uint queue_log_cnt=0;
  uint src_tor=0;
  uint dst_tor=0;
  uint spine=0;
  uint new_path_congested=0;
  /* Loop through until we are done with the file. */
  while (line_size >= 0)
  {
	tmp_line_buf = strdup(line_buf);
	pch = strtok(tmp_line_buf," ");

	if(strcmp("Flow:", pch)==0)
	{	class = 1; flow_log_cnt = flow_log_cnt + 1; }
	else if (strcmp("CheckQueueSize", pch)==0)
	{	class = 2; queue_log_cnt = queue_log_cnt + 1; }
	else 
		class = 0;


	if( class == 1 )
	{	// Start strping of the line
		while (pch != NULL)
		{
			cnt_elem = cnt_elem + 1;
			if(cnt_elem==3)
				src_tor = pch[1] - '0';
				// printf ("%s\n",pch);
			// Location of the NEW(OLD) path
			if(cnt_elem==8)
			{	
				flow_path_str[0] = pch[0];
				flow_path_str[1] = pch[1];
				flow_path_str[2] = pch[2];
				// printf ("%s\n",flow_path_str);
				flow_path = atoi(flow_path_str);
				flow_path = flow_path - 17;
				dst_tor = ( flow_path / 100 ) - 1;
				spine = flow_path % 100;
				// printf ("%d\n",flow_path);
			}
			// Location of the timestamp
			if(cnt_elem==10)
				flow_timestamp = strdup(pch);
				// printf ("%s\n",pch);
			// Get next token
			pch = strtok (NULL, " ");
		}
		cnt_elem=0;
	}
	else if(class == 2 )
	{	// Start strping of the line
		while (pch != NULL)
		{
			cnt_elem = cnt_elem + 1;

			// Location of timestamp
			if(cnt_elem==4)
				queue_timestamp = strdup(pch);
			// Location of the queues utilisations
			if(cnt_elem>4)
			{
				// printf ("%s  %s\n",queue_timestamp,flow_timestamp);

				// printf ("%d , %d .. %d -> %d\n",(src_tor*16)+(spine*2),(dst_tor*16)+(spine*2)+1,(cnt_elem-5),strcmp(queue_timestamp,flow_timestamp));

				// Check if there is congestion on src_tor -> spine , (src_tor*16)+(spine*2) uplink
				// Check of there is congestion on spine -> dst_tor, (dst_tor*16)+(spine*2)+1 downlink
				if( (src_tor*16)+(spine*2) == (cnt_elem-5) || (dst_tor*16)+(spine*2)+1 == (cnt_elem-5))
					if(atoi(pch)>threshold)
					{
						new_path_congested++;
						// printf("%d ",new_path_congested);
						break;
					}
						// printf (">%d %s\n",cnt_elem-5,pch);
				// }
			}


			pch = strtok (NULL, " ");
		}
		cnt_elem=0;
	}
	else
		;
    /* Increment our line count */
    line_count++;

    /* Show the line details */
    // printf("line[%06d]: chars=%06zd, buf size=%06zu, contents: %s", line_count, line_size, line_buf_size, line_buf);

    /* Get the next line */
    line_size = getline(&line_buf, &line_buf_size, fp);
  }

  if(flow_log_cnt != queue_log_cnt)
	printf("SOMETHING WENT WRONG\nRESULTS MIGHT BE CORRUPTED!!!\n");


   printf("\n %d / %d  =  %.2f ",new_path_congested ,flow_log_cnt, (float)new_path_congested/(float)flow_log_cnt);
   printf("\tPercentage\t: %.1f%%\n",((float)new_path_congested/(float)flow_log_cnt)*100);
   printf("Stats Flows,Queues\t: %d, %d\n", flow_log_cnt,queue_log_cnt);
	printf("Threshold\t: %d \n",threshold);
  /* Free the allocated line buffer */
  free(line_buf);
  //line_buf = NULL;

  /* Close the file now that we are done with it */
  fclose(fp);

  return EXIT_SUCCESS;
}