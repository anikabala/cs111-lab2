## UID: 006030487

# You Spin Me Round Robin

This program takes a given text file of processes and their respectiven arrival times, burst times, and a quantum length, schedules them via a Round Robin algorithm, and outputs the average waiting time and average response time for the processes.

## Building

To build this, you just need to compile this using gcc once the program and directory have been installed, complete with process files. The command used is:

gcc rr.c -o rr

This creates the executable file.
The 'make' command can also make the process.

## Running

The first argument should be the text file of process information, and the second the quantum length. The format of the processes files should be:

Number of processes
Process number, arrival time, burst time
...

To run, call the executable:

./rr processes.txt 3

The results of the above are:
Average waiting time: 7.00
Average response time: 2.75

## Cleaning up

Using 'make clean' will get rid of any executable and binary files. The executable can also be removed by using the 'rm' command.
