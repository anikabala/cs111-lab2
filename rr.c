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
  struct process_list ready_queue;
    TAILQ_INIT(&ready_queue);

    u32 current_time = 0;
    u32 completed_processes = 0;

// Loop until all the processes are executed
while (completed_processes < size) {
    // Add the processes to the ready queue that have arrived at the current time
    for (u32 i = 0; i < size; ++i) {
        if (data[i].arrival_time <= current_time && data[i].burst_time > 0 && data[i].pointers.tqe_prev == NULL) {
            TAILQ_INSERT_TAIL(&ready_queue, &data[i], pointers);
            data[i].response_time = current_time - data[i].arrival_time; // Calculate response time
        }
    }

    // If the ready queue is empty, just increase the current time
    if (TAILQ_EMPTY(&ready_queue)) {
        current_time++;
        continue;
    }

    // Get the process from the head of the ready queue and execute it for the quantum length
    struct process *current_process = TAILQ_FIRST(&ready_queue);
    TAILQ_REMOVE(&ready_queue, current_process, pointers);
    u32 execution_time = min(current_process->burst_time, quantum_length);
    current_process->burst_time -= execution_time;

    // Increase the current time by the execution time
    current_time += execution_time;

    // Add the current process back to the ready queue if it has not completed
    if (current_process->burst_time > 0) {
        TAILQ_INSERT_TAIL(&ready_queue, current_process, pointers);
    } else {
        completed_processes++;
        total_waiting_time += current_time - current_process->arrival_time - execution_time; // Calculate waiting time
    }
}

// Calculate average waiting and response times
for (u32 i = 0; i < size; ++i) {
    total_response_time += data[i].response_time;
}
total_waiting_time /= size;
total_response_time /= size;

  
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
