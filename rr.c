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

    u32 time = 0;  //to keep track of the current time
    u32 completed_processes = 0;

    struct process_list process_queue; //create a process list queue
    TAILQ_INIT(&process_queue); //initialize the queue

    //the initialization -- taking in new processes and setting their fields
    while(completed_processes < size){

      //making sure to take in processes as they come, initialize, add to queue
      for(u32 i = 0; i < size; i++){
        if(data[i].arrival_time <= time & data[i].burst_time > 0 && data[i].pointers.tqe_prev == NULL){
          data[i].remaining_time = data[i].burst_time;
          data[i].response_time = time;
          TAILQ_INSERT_TAIL(&process_queue, &data[i], pointers);
        }
      }

      //if we have no more processes we are done!
      if(TAILQ_EMPTY(&process_queue)){
        time++;
        continue;
      }

      //start the handling of processes
      struct process *current_process = TAILQ_FIRST(&process_queue);
      u32 process_start_time = time;
      current_process->response_time = process_start_time - current_process->arrival_time; // Update response time
      TAILQ_REMOVE(&process_queue, current_process, pointers);

      if(current_process->remaining_time <= quantum_length){
        completed_processes++;
        total_waiting_time += (time + current_process->remaining_time - current_process->arrival_time - current_process->burst_time);
        total_response_time += (time - current_process->response_time - current_process->waiting_time);
        time += current_process->remaining_time;
        current_process->remaining_time = 0;
      } else {
        current_process->remaining_time -= quantum_length;
        current_process->response_time += (process_start_time - current_process->arrival_time);
        total_waiting_time += (time + quantum_length - current_process->arrival_time - current_process->burst_time);
        time += quantum_length;
        TAILQ_INSERT_TAIL(&process_queue, current_process, pointers);
      }
    }

  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
