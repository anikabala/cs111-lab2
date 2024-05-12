#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <Kernel/sys/queue.h>

#define min(x, y) ((x) < (y) ? (x) : (y))

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  
  u32 waiting_time; //track the time we wait so we can return it
  u32 response_time; //track the response time so we can return it
  u32 remaining_time; //we want to track the time left

  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path, struct process **process_data, u32 *process_size){
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}


// Function to calculate the total waiting time and total response time
void calculate_waiting_and_response_time(struct process *p, u32 n, u32 quantum) {
    u32 total_waiting_time = 0;
    u32 total_response_time = 0;
    u32 time = 0;  // track the current time

    struct process_list queue;
    TAILQ_INIT(&queue);

    // Loop through all processes
    while (true) {
        // Add all the processes that arrive at the current time to the queue
        for (u32 i = 0; i < n; i++) {
            if (p[i].arrival_time == time) {
                p[i].remaining_time = p[i].burst_time;
                p[i].response_time = time;
                TAILQ_INSERT_TAIL(&queue, &p[i], pointers);
            }
        }

        // If there are no more processes, break the loop
        if (TAILQ_EMPTY(&queue)) {
            break;
        }

        // Get the next process from the queue and process it
        struct process *current = TAILQ_FIRST(&queue);
        TAILQ_REMOVE(&queue, current, pointers);

        if (current->remaining_time > quantum) {
            // The process still has some work left to do after the quantum, so put it back in the queue
            current->remaining_time -= quantum;
            current->waiting_time += time - current->response_time;
            current->response_time = 0;
            TAILQ_INSERT_TAIL(&queue, current, pointers);
            time += quantum;
        } else {
            // The process finished within the quantum, so add its waiting time to the total waiting time
            current->waiting_time += time - current->response_time;
            current->response_time = 0;
            total_waiting_time += current->waiting_time;
            total_response_time += current->waiting_time + current->burst_time;
            time += current->remaining_time;
        }
    }

    // Print the average waiting time and average response time
    printf("Average waiting time: %.2f\n", (float) total_waiting_time / n);
    printf("Average response time: %.2f\n", (float) total_response_time / n);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  /* Your code here */
  /* End of "Your code here" */

  calculate_waiting_and_response_time(data, size, quantum_length);

  // printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  // printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
