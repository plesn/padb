# -*- mode: markdown -*-
Title: Using PADB to make a binding checker ?
Author: piotr.lesnicki@atos.net


# Padb overview:

Not just clush+pstack:

- 10 kloc of perl : perl scripts that
  - detects launcher and calls
  - calls itself on the nodes, provides handlers at each step
  - attaches with gdb to get stack traces
- 3 kloc of C : uses MPIR for portable MPI queue info from gdb

usage:

- padb -atx : pstack like with tree folding

```
Stack trace(s) for thread: 1
  1: -----------------
  1: Branch 1 of 1, 10 processes, 100% of total [0-9]
  1: -----------------
  1: main() at ?:?
Stack trace(s) for thread: 2
  1: -----------------
  1: Branch 1 of 1, 10 processes, 100% of total [0-9]
  1: -----------------
  1: start_thread() at ?:?
  2:  mxm_async_thread_func() at mxm/core/async.c:316
  3:   epoll_wait() at ?:?
Stack trace(s) for thread: 3
  1: -----------------
  1: Branch 1 of 1, 10 processes, 100% of total [0-9]
  1: -----------------
  1: start_thread() at ?:?
  2:  progress_engine() at runtime/opal_progress_threads.c:105
  3:   opal_libevent2022_event_base_loop() at event.c:1630
  4:    poll_dispatch() at poll.c:165
```

- padb -a --proc-summary : top like

```
rank  hostname  pid    vmsize     vmrss     S  uptime  %cpu  lcore  command        
   0      btp1  18673  224524 kB  44836 kB  R    3.95    24      0  omp_hybrid_hell
   1      btp2  26360  224504 kB  44812 kB  R    0.90    99      0  omp_hybrid_hell
   2      btp3  26783  224552 kB  44808 kB  R    0.96    99      0  omp_hybrid_hell
   3      btp4  32548  224392 kB  44804 kB  R    0.96    99      0  omp_hybrid_hell
   4      btp7  26616  224520 kB  44788 kB  R    0.88    99      0  omp_hybrid_hell
   5      btp8  22057  224388 kB  44804 kB  R    0.97    99      0  omp_hybrid_hell
   6      btp9   9280  224536 kB  44812 kB  R    0.94    99      0  omp_hybrid_hell
   7     btp10  28189  224520 kB  44808 kB  R    1.00    99      0  omp_hybrid_hell
   8     btp11   7518  224388 kB  44824 kB  R    0.95    99      0  omp_hybrid_hell
   9     btp12  14812  236148 kB  54900 kB  R    0.96    99      0  omp_hybrid_hell
```

- padb -a --lstopo
  shows node topology, but not useful in this form


# code use cases:

- launch command per MPI process or per node: ok
  - made option: `padb --bindings` (calling hwloc-ps)
- attach to process using gdb : no simple copy-paste, but ok
- call code per thread : stack traces / thread, so must be possible

# timing

simple timing of gdb attachment to procs:
- N=1 n=4 : 7s with padb and clush
- N=2 n=64 : 32s with padb and 2m33 with clush
- N=10 n=10 : 7s with padb and 2s with clush

- padb has scalable attachement (can use clush as a backend)
- clush is more scalable by node ?? (strange

# PADB functionality:

- job discovery : supports multiple launchers
  - supports: slurm, ompi, mpich2, mpd, pbs, lsf
  - slurm : squeue, scontrol listpids
  - ompi : ompi-ps
  - mpich2 : mpdlistjobs

- spawning on nodes (padb calls padb --inner on nodes)
  - spawning : can use srun / ssh / pdsh / clush as a backend
  - communication by sockets : binary tree to inner padbs
  - handlers:
    - handler : called on proc
    - handler_out : called on node
    - out_handler : called on restult printing
    - communication between handlers : ??
  - data at different levels: $inner_conf

- attach to process using gdb
  - optimisation : gdb attachmenet is asynchronous : attaches to all, then queries

- output formatting:
  - function to format top-like data
  - functions to display tree of stack traces

- other utilities:
  - launching commands, get process infos
  - deadlock detection…




# binding checker: the steps

1. find job info: use slurm, ompi-ps…
   - slurm : scontrol show job <jobid> : nodes
   - slurm : ranks->pids : node$ scontrol listpids : + ps
   - ompi-ps

2. find runtime details about a thread/process, 2 methods:
   21. query runtime for info (mpi,srun)
      - srun ranks : sattach --layout <jobid.stepid>
      - mpirun ranks: ompi-ps

   22. use gdb and attach
      - getting pid with OMPI : orte_process_info.my_name.vpid)
      - use MPIR for portability
      - required for omp tid: omp_get_thread_num()

   Attaching is slow, avoid it if possible

3. binding information, also 2 methods:
   21. call hwloc-ps -t (no need attaching, call per node)
       - to filter (filter out orteds) and map to ranks
   22. withing the process using gdb:
       - dlopen a library using hwloc

   testing with threads:
   ```
   env OMP_NUM_THREADS=2 KMP_AFFINITY=granularity=fine,proclist=[0,1,16,17],explicit  salloc -J hello -N1 --exclusive -n 2 mpirun --bind-to socket --map-by socket ~/src/mpi/omp/omp_hybrid_hello20 --crunch
   ```

4. gather and format results
    - filter out orted (easy: orted is not bound)
    - fold using both node similarity (hwloc-ps --no-caches --no-io
      ?) and binding results
    - threads => tree binding


All of this fit quite well withing PADB.



# conclusion : binding checker with padb

+ available features (job discovery, MPI info, process attachmement)
- maintenance ? (last version 2010 vs clush in 2015)
- design choice : to attach or not to attach ? pb with OMP threads…
