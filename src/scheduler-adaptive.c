#include <stdio.h>
#include "utlist.h"
#include "utils.h"

#include "memory_controller.h"
#include "params.h"

/* A basic FCFS policy augmented with a not-so-clever close-page policy.
   If the memory controller is unable to issue a command this cycle, find
   a bank that recently serviced a column-rd/wr and close it (precharge it). */

extern long long int CYCLE_VAL;

const short int COUNT_MAX = 15, COUNT_MIN = 0, HIGH_COUNT = 11, LOW_COUNT = 6;      //constants for the counter

enum Policy{OPEN = 0, CLOSED = 1};
enum Policy paging_policy = OPEN;

short int counter = 8;      //4-bit saturation counter

long long int policy_switches = 0, open_accesses = 0, closed_accesses = 0, open_misses = 0, closed_hits = 0, total_accesses = 0;

// policy_switches : no of times we switch from closed to open & vice-versa
// open_accesses   : no of accesses done during open page policy
// closed_accesses : no of accesses done during closed page policy
// open_misses     : no of misses incurred during open page policy
// closed_misses   : no of misses incurred during closed page policy
// total_accesses  : total no of read/writes

void inc_counter()
{
    if(counter < COUNT_MAX)
        counter++;
}

void dec_counter()
{
    if(counter > COUNT_MIN)
        counter--;
}

/* A data structure to see if a bank is a candidate for precharge. */
int recent_colacc[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];

/* A data structure to keep track of the previously active row */
int prev_active_row[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];

/* Keeping track of how many preemptive precharges are performed. */
long long int num_aggr_precharge = 0;

void init_scheduler_vars()
{
	// initialize all scheduler variables here
	int i, j, k;
	for (i=0; i<MAX_NUM_CHANNELS; i++) {
	  for (j=0; j<MAX_NUM_RANKS; j++) {
	    for (k=0; k<MAX_NUM_BANKS; k++) {
	      recent_colacc[i][j][k] = 0;
          prev_active_row[i][j][k] = -1;
	    }
	  }
	}

	return;
}

// write queue high water mark; begin draining writes if write queue exceeds this value
#define HI_WM 40

// end write queue drain once write queue has this many writes in it
#define LO_WM 20

// 1 means we are in write-drain mode for that channel
int drain_writes[MAX_NUM_CHANNELS];

/* Each cycle it is possible to issue a valid command from the read or write queues
   OR
   a valid precharge command to any bank (issue_precharge_command())
   OR
   a valid precharge_all bank command to a rank (issue_all_bank_precharge_command())
   OR
   a power_down command (issue_powerdown_command()), programmed either for fast or slow exit mode
   OR
   a refresh command (issue_refresh_command())
   OR
   a power_up command (issue_powerup_command())
   OR
   an activate to a specific row (issue_activate_command()).

   If a COL-RD or COL-WR is picked for issue, the scheduler also has the
   option to issue an auto-precharge in this cycle (issue_autoprecharge()).

   Before issuing a command it is important to check if it is issuable. For the RD/WR queue resident commands, checking the "command_issuable" flag is necessary. To check if the other commands (mentioned above) can be issued, it is important to check one of the following functions: is_precharge_allowed, is_all_bank_precharge_allowed, is_powerdown_fast_allowed, is_powerdown_slow_allowed, is_powerup_allowed, is_refresh_allowed, is_autoprecharge_allowed, is_activate_allowed.
   */


void schedule(int channel)
{
	request_t * rd_ptr = NULL;
	request_t * wr_ptr = NULL;
	int i, j;


	// if in write drain mode, keep draining writes until the
	// write queue occupancy drops to LO_WM
	if (drain_writes[channel] && (write_queue_length[channel] > LO_WM)) {
	  drain_writes[channel] = 1; // Keep draining.
	}
	else {
	  drain_writes[channel] = 0; // No need to drain.
	}

	// initiate write drain if either the write queue occupancy
	// has reached the HI_WM , OR, if there are no pending read
	// requests
	if(write_queue_length[channel] > HI_WM)
	{
		drain_writes[channel] = 1;
	}
	else {
	  if (!read_queue_length[channel])
	    drain_writes[channel] = 1;
	}

	if(counter > HIGH_COUNT)
    {
        policy_switches++;
        paging_policy = CLOSED;
    }

    if(counter < LOW_COUNT)
    {
        policy_switches++;
        paging_policy = OPEN;
    }


	// If in write drain mode, look through all the write queue
	// elements (already arranged in the order of arrival), and
	// issue the command for the first request that is ready
	if(drain_writes[channel])
	{

		LL_FOREACH(write_queue_head[channel], wr_ptr)
		{
			if(wr_ptr->command_issuable)
			{
                if(paging_policy == CLOSED)
                {
                    /* Before issuing the command, see if this bank is now a candidate for closure (if it just did a column-rd/wr).
                    If the bank just did an activate or precharge, it is not a candidate for closure. */
                    if (wr_ptr->next_command == COL_WRITE_CMD)
                    {
                        closed_accesses++;

                        if(wr_ptr->dram_addr.row == prev_active_row[channel][wr_ptr->dram_addr.rank][wr_ptr->dram_addr.bank])
                        {
                            dec_counter();
                            closed_hits++;
                        }
                        else
                            prev_active_row[channel][wr_ptr->dram_addr.rank][wr_ptr->dram_addr.bank] = wr_ptr->dram_addr.row;

                        recent_colacc[channel][wr_ptr->dram_addr.rank][wr_ptr->dram_addr.bank] = 1;

                    }
                    if (wr_ptr->next_command == ACT_CMD) {
                        recent_colacc[channel][wr_ptr->dram_addr.rank][wr_ptr->dram_addr.bank] = 0;
                    }
                    if (wr_ptr->next_command == PRE_CMD) {
                        recent_colacc[channel][wr_ptr->dram_addr.rank][wr_ptr->dram_addr.bank] = 0;
                    }
                }
                else
                {
                    open_accesses++;
                    if(wr_ptr->next_command == COL_WRITE_CMD && wr_ptr->dram_addr.row != prev_active_row[channel][wr_ptr->dram_addr.rank][wr_ptr->dram_addr.bank])
                    {
                        inc_counter();
                        open_misses++;
                        prev_active_row[channel][wr_ptr->dram_addr.rank][wr_ptr->dram_addr.bank] = wr_ptr->dram_addr.row;
                    }
                    issue_request_command(wr_ptr);
                    return;
                }

				issue_request_command(wr_ptr);
				break;
			}
		}
	}

	// Draining Reads
	// look through the queue and find the first request whose
	// command can be issued in this cycle and issue it
	// Simple FCFS
	if(!drain_writes[channel])
	{
		LL_FOREACH(read_queue_head[channel],rd_ptr)
		{
			if(rd_ptr->command_issuable)
			{
                if(paging_policy == CLOSED)
                {
                    /* Before issuing the command, see if this bank is now a candidate for closure (if it just did a column-rd/wr).
				   If the bank just did an activate or precharge, it is not a candidate for closure. */
                    if (rd_ptr->next_command == COL_READ_CMD)
                    {
                        closed_accesses++;
                        // If it incurrs a hit while being in closed page policy, we decrement the counter
                        if(rd_ptr->dram_addr.row == prev_active_row[channel][rd_ptr->dram_addr.rank][rd_ptr->dram_addr.bank])
                        {
                            dec_counter();
                            closed_hits++;
                        }
                        else
                            prev_active_row[channel][rd_ptr->dram_addr.rank][rd_ptr->dram_addr.bank] = rd_ptr->dram_addr.row;

                        recent_colacc[channel][rd_ptr->dram_addr.rank][rd_ptr->dram_addr.bank] = 1;
                    }
                    if (rd_ptr->next_command == ACT_CMD) {
                        recent_colacc[channel][rd_ptr->dram_addr.rank][rd_ptr->dram_addr.bank] = 0;
                    }
                    if (rd_ptr->next_command == PRE_CMD) {
                        recent_colacc[channel][rd_ptr->dram_addr.rank][rd_ptr->dram_addr.bank] = 0;
                    }
                }
                else
                {
                    open_accesses++;
                    // If it incurrs a miss while being in open page policy, we increment the counter
                    if(rd_ptr->next_command == COL_READ_CMD && rd_ptr->dram_addr.row != prev_active_row[channel][rd_ptr->dram_addr.rank][rd_ptr->dram_addr.bank])
                    {
                        inc_counter();
                        open_misses++;
                        prev_active_row[channel][rd_ptr->dram_addr.rank][rd_ptr->dram_addr.bank] = rd_ptr->dram_addr.row;
                    }
                    issue_request_command(rd_ptr);
                    return;
                }

				issue_request_command(rd_ptr);
				break;
			}
		}
	}

    if(paging_policy == CLOSED)
    {
        /* If a command hasn't yet been issued to this channel in this cycle, issue a precharge. */
        if (!command_issued_current_cycle[channel])
        {
            for (i=0; i<NUM_RANKS; i++)
            {
                for (j=0; j<NUM_BANKS; j++)         /* For all banks on the channel.. */
                {
                    if (recent_colacc[channel][i][j])  /* See if this bank is a candidate. */
                    {
                        if (is_precharge_allowed(channel,i,j)) {  /* See if precharge is doable. */
                            if (issue_precharge_command(channel,i,j))
                            {
                                num_aggr_precharge++;
                                recent_colacc[channel][i][j] = 0;
                            }
                        }
                    }
                }
            }
        }
    }
}

void scheduler_stats()
{
  /* Nothing to print for now. */
    total_accesses = open_accesses+closed_accesses;

    printf("Number of policy switches = %lld\n",policy_switches);
    printf("%% Open Policy Accesses = %lf\n",(double)open_accesses*100/total_accesses);

    printf("%% Closed Policy Accesses = %lf\n",(double)closed_accesses*100/total_accesses);
    printf("%% Misses when in Open Policy = %lf\n",(double)open_misses*100/open_accesses);

    printf("%% Hits when in Closed Policy = %lf\n",(double)closed_hits*100/closed_accesses);
    printf("Number of aggressive precharges: %lld\n", num_aggr_precharge);

}

