/*
 * Student Name: Kevin Usuanlele / Scott Weaver
 * Student ID:   3103649 / 3144661
 * Date:         May 13, 2026
 * File:         sched.c
 * Description:  Read a workload file that lists processes by process ID, arrival time,
 *               and required CPU burst and simulates execution on a single CPU printing
 *               both a timeline of execution and detailed scheduling metrics
 * Reference(s): strncmp() - https://www.w3schools.com/c/ref_string_strncmp.php
 *               strtok() - https://www.w3schools.com/c/ref_string_strtok.php
 *               Scheduling Queue - https://www.digitalocean.com/community/tutorials/queue-in-c
 *                                - https://www.geeksforgeeks.org/c/queue-in-c/
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sched.h"
#include "stdbool.h"

// Macros
#define BUFFER 256
#define DELIMS " \t\n" // Seperate fields at: space, tab, or newline

static void usage(const char* prog){
    /* one-line usage only (per spec) */
    fprintf(stderr, "Usage: %s --policy=FCFS|RR [--quantum=N] --in=FILE\n", prog);
}

int parse_args(int argc, char** argv, sim_cfg_t* cfg, const char** in_path)
/*
    Purpose: Parse command line args and fill in cfg and in_path with values
    Params: argc: number of command line args, argv: array of command line arg strings
            cfg: pointer to sim_cfg_t struct to hold policy and quantum values
            in_path: pointer to char* to hold path value
    Return: 0 on success, calls usage() and returns 1 on error (invalid args or combos)
*/
{
    bool quantum = false, policy = false, path = false;
    char *value;

    // Check if the number of args is valid
    if (argc < 3)
    {
        usage(argv[0]);
        return 1;
    }

    // Parse through args => ./sched --policy=FCFS --in=W1.txt
    for (int i = 1; i < argc; i++) // start at 1 to skip program name
    {
        value = strchr(argv[i], '='); // find '=' in arg and point to value after it
        if (value != NULL) // if '=' is found
        {
            value++; // move pointer to FCFS/RR, path, or quantum
        }
        else // input not valid => ./sched --policyFCFS --inW1.txt
        {
            usage(argv[0]);
            return 1;
        }

        // Read policy value
        if (strncmp(argv[i], "--policy=", 9) == 0) // compare first 9 chars ("--policy=") with arg to check if it's policy arg
        {
            if (strcmp(value, "FCFS") == 0) cfg->policy = POL_FCFS;
            else if (strcmp(value, "RR") == 0) cfg->policy = POL_RR;
            else {usage(argv[0]); return 1;}
            policy = true;
        }

        // Read Path
        else if (strncmp(argv[i], "--in=", 5) == 0)
        {
            *in_path = value; // value should point to path
            path = true;
        }

        // Read Quantum
        else if (strncmp(argv[i], "--quantum=", 10) == 0)
        {
            for (char* c = value; *c != '\0'; c++) // check if value is only digits
            {
                if (*c < '0' || *c > '9')
                {
                    usage(argv[0]);
                    return 1;
                }
            }
            cfg->quantum = atoi(value);
            if (cfg->quantum <= 0) // quantum must be > 0
            {
                usage(argv[0]);
                return 1;
            }
            quantum = true;
        }

        else
        {
            usage(argv[0]);
            return 1;
        }
    }
    // Validate flag combos
    if (!policy || !path) {usage(argv[0]); return 1;}
    if (cfg->policy == POL_FCFS && quantum) {usage(argv[0]); return 1;}
    if (cfg->policy == POL_RR && !quantum) {usage(argv[0]); return 1;}
    return 0;
}

static int job_sort(const void *a, const void *b)
/*
    Purpose: Comparison function for qsort to sort job_t's by arrival time, then PID
    Params: a and b - pointers to job_t's to compare
    Return: negative if a < b, 0 if a == b, positive if a > b (for sorting in ascending order)
*/
{
    // Cast void* to job_t* to access fields for comparison
    const job_t *a_job = (const job_t *) a;
    const job_t *b_job = (const job_t *) b;

    // Sort by arrival time first, if arrival times are equal, sort by PID
    if (a_job->arrival != b_job->arrival) return a_job->arrival - b_job->arrival; // sort by arrival time
    return a_job->pid - b_job->pid; // if arrival times are equal, sort by PID
}

int load_workload(const char* path, job_t** jobs, int* n)
/*
    Purpose: Read a workload file and populate a dyn array of jobs
    Params: path - path to file
            jobs - array of job_t structs
            n - number of jobs
    Return: 0 on success, 1 on error
*/
{
    // Read file in path
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {perror("Error opening file"); return 1;}

    // Initialize variables
    char line[BUFFER]; // buffer to hold each line read from file
    *jobs = NULL; // pointer to array of job_t's
    *n = 0; // number of jobs

    // Read line by line
    while (fgets(line, BUFFER, fp))
    {
        if (line[0] == '#' || line[0] == '\n') continue; // skip headers and blank lines in file

        // Break valid lines into PID, Arrival, CPU_Time
        char *pid_tok = strtok(line, DELIMS); 
        char *arrival_tok = strtok(NULL, DELIMS); // NULL - pick up where prev strtok left off
        char *cpu_time_tok = strtok(NULL, DELIMS);

        // Validate tokens
        if (pid_tok == NULL || arrival_tok == NULL || cpu_time_tok == NULL)
        {
            fprintf(stderr, "Error: Broken Process\n");
            free(*jobs);
            fclose(fp);
            return 1;
        }

        // Convert tokens to int
        int pid = atoi(pid_tok);
        int arrival = atoi(arrival_tok);
        int cpu_time = atoi(cpu_time_tok);

        // Validate values
        if (pid < 0 || arrival < 0 || cpu_time <= 0)
        {
            fprintf(stderr, "Error: Invalid Process Values\n");
            free(*jobs);
            fclose(fp);
            return 1;
        }
        
        // Expand array of job_t's to hold new job_t instance
        job_t *new_arr = realloc(*jobs, (*n + 1) * sizeof(job_t)); // resize array to hold one extra job_t instance at a time
        if (new_arr == NULL) // new_arr should point to first element of resized array, if NULL realloc failed
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            free(*jobs);
            fclose(fp);
            return 1;
        }
        *jobs = new_arr; // update *jobs to point to newly resized array, incase realloc moved it to a new location

        // Fill in new job_t instance with values and increment job count
        (*jobs)[*n].pid = pid;
        (*jobs)[*n].arrival = arrival;
        (*jobs)[*n].cpu_time = cpu_time;
        (*n)++; // increment job count
    }

    // Use qsort to sort jobs by arrival time, then PID
    qsort(*jobs, *n, sizeof(job_t), job_sort);
    fclose(fp);
    return 0;
}

/* TODO: discrete-time CPU simulation with FIFO ready queue.
   FCFS: never preempt. RR: quantum countdown; when it hits 0 and job not finished, enqueue at tail.
   Track: first run (first time scheduled) and completion (t+1 on finish).
   Build timeline run[t]=pid or -1 (idle). After finishing:
     - ctx = count PID->PID changes (ignore idle)
     - TAT = completion-arrival; RESP = first run-arrival
   Print exactly:
     time: 0 1 2 ... T-1
     run : <pid or -> per tick
     Pk: first run=... completion=... TAT=... RESP=...
     System: ctx_switches=..., avgTAT=..., avgRESP=...
*/
int simulate(const job_t* jobs, int n, const sim_cfg_t* cfg, sim_metrics_t* out)
/*
    Purpose: Simulate execution of jobs on a single CPU with given scheduling policy and quantum (if RR)
    Params: jobs - array of job_t's to schedule
            n - number of jobs
            cfg - sim_cfg_t struct with scheduling policy and quantum (if RR)
            out - sim_metrics_t struct to fill in with results
    Return: 0 on success, 1 on error
*/
{
    // Initialize arrays to track job states
    int *time_remaining = malloc(n * sizeof(int)); // array to track remaining CPU time for each job
    bool *in_queue = malloc(n * sizeof(bool)); // array to track if job is in ready queue
    int *queue = malloc(n * sizeof(int)); // array to implement ready queue (store indices of jobs)

    // Initialize Queue variables
    int head = 0, tail = 0, curr = -1, t = 0, jobs_completed = 0;

    /* Initialize arrays for: 
                            Turnaround Time = completion[i] - jobs[i].arrival
                            Response Time = first_run[i] - jobs[i].arrival
    */ 
    int *first_run = malloc(n * sizeof(int)); // array to track first run time for each job
    int *completion = malloc(n * sizeof(int)); // array to track completion

    // Verify memory allocation for all arrays
    if (!time_remaining || !in_queue || !queue || !first_run || !completion)
    {
        fprintf(stderr, "Memory Error: Allocation Failed.");
        free(time_remaining); free(in_queue); free(queue); free(first_run); free(completion);
        return 1;
    }

    // Assign elements in arrays
    for (int i = 0; i < n; i++)
    {
        time_remaining[i] = jobs[i].cpu_time;
        in_queue[i] = false;
        completion[i] = -1; // placeholder int
        first_run[i] = -1; // placeholder int
    }

    // Set up timeline array to keep track of which job is running at each time tick
    int timeline_size = 32; // will grow as needed
    int *timeline = malloc(timeline_size * sizeof(int)); // array to track which job is running at each time tick (store PID or -1 for idle)
    if (!timeline)    
    {
        fprintf(stderr, "Memory Error: Allocation Failed.");
        free(time_remaining); free(in_queue); free(queue); free(first_run); free(completion); free(timeline);
        return 1;
    }

    // Main simulation loop
    while (jobs_completed < n)
    {
        // Queue jobs that arrive at t
        for (int i = 0; i < n; i++) // check all jobs to see if they arrive at current time t
        {
            if (jobs[i].arrival == t && !in_queue[i])
            {
                queue[tail] = i; // queue job at end of queue
                in_queue[i] = true;
                tail++;
            }
        }
        
        // Schedule job at head of queue
        if (curr == -1 && head < tail) 
        {
            curr = queue[head]; // schedule job at head of queue
            head++;
            if (first_run[curr] == -1) first_run[curr] = t; // record first run
        }

        // Update Timeline if needed 
        if (t >= timeline_size) // if the number of ticks exceed the timeline array size then resize the array
        {
            timeline_size *= 2;
            int *new_timeline = realloc(timeline, timeline_size * sizeof(int));
            if (!new_timeline)
            {
                fprintf(stderr, "Memory Error: Allocation Failed.");
                free(time_remaining); free(in_queue); free(queue); free(first_run); free(completion); free(timeline);
                return 1;
            }
            timeline = new_timeline; // sync timeline pointer to new array
        }

        // Fill in current tick with pid or -1 for idle
        if (curr == -1) timeline[t] = -1; // if no job is scheduled, mark timeline as idle
        else timeline[t] = jobs[curr].pid; // otherwise, mark timeline with PID of scheduled job
        
        // Run the tick
        if (curr != -1) // if a job is scheduled
        {
            time_remaining[curr]--; // decrement a unit of CPU time from the current job
            if (time_remaining[curr] == 0) // if the job is completed
            {
                completion[curr] = t + 1;
                curr = -1; // re-idle CPU
                jobs_completed++;
            }
        }
        t++;
    }

}

int main(int argc, char** argv)
{
    sim_cfg_t cfg; const char* in_path=NULL;
    int pr = parse_args(argc, argv, &cfg, &in_path);
    if (pr != 0) return 1;

    job_t* jobs=NULL; int n=0;
    if (load_workload(in_path, &jobs, &n) != 0) return 2;

    sim_metrics_t m;
    if (simulate(jobs, n, &cfg, &m) != 0) { free(jobs); return 3; }

    free(jobs);
    return 0;
}

