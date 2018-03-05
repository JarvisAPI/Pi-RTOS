# Supporting Aperiodic Tasks and Multiprocessor Scheduling in FreeRTOS

CPEN 432 / Project 4 


## Introduction
In this programming assignment you will be extending FreeRTOS in two key directions. First, you will add support for servicing aperiodic, soft real-time requests alongside hard real-time tasks, by implementing reservation-based, dynamic server schemes for handling aperiodic tasks, on top of EDF. You will be using your EDF implementation from the previous programming assignment.

Second, you will be adding support for multiprocessor (multicore) real-time scheduling, making use of the four cores available in our RPi. You will implement both partitioned and global scheduling schemes. You will also develop a top-like tool that would allow the user to see the activity happening on each of teh four cores, including the tasks already running on the processor and their states.


**This programming assignment consists of 4 tasks**, and each section 
describing a deliverable is labeled **[Task n]**, where n is the deliverable number. Make sure to read all the material preceding a task before 
attempting it. This assignment is worth 15% of your _raw_ grade.

# [Task 1] Constant Bandwidth Server (CBS) Support
 Read the chapter on Dynamic Priority Servers in the text (chapter 7). In essence, CBS allows tasks that exceed their nominal CPU reserve to continue execution, or stay in the ready queue, by postponing the deadline of the task. FreeRTOS provides methods for creating and managing periodic tasks. For this project, add a new type of task that would be managed by a constant bandwidth server. Creating a task that is managed by a CBS would be similar to creating a regular periodic task.
 
 Add the primitives needed to manage constant bandwidth servers at the programming interface and within the kernel/scheduler. It is possible to have multiple CBS tasks running concurrently. A software developer should be able to build an application that uses a combination of regular periodic tasks and constant bandwidth servers. Tasks are scheduled using EDF. Remember that priority ties are always broken in favor of the server. To simplify matters, you may assume that a job of a task is not released until the previous job of the same task is complete.
 
 
# [Task 2]  Implement the CASH approach for capacity sharing
CBS has a limitation in that when tasks do not fully use their budget, other tasks cannot exploit this extra processing capacity, i.e., all tasks use only their own budget and cannot leverage extra capacity that may result from jobs 
completing early. CASH is an approach that addresses this limitation. Read the article titled [Capacity Sharing for Overrun Control](http://ieeexplore.ieee.org/document/896018/) that appeared in the IEEE Real-Time Systems Symposium in 2000 and implement the CASH method described by Caccamo, et al. Several real-time operating systems do implement schemes that are rather similar to CASH when they co-schedule hard real-time and soft real-time tasks.


# [Task 3] Multiprocessor Real-time schemes
In this part you will be considering only _implicit-deadline_ periodic tasks. You will implement both partitioned and global EDF in FreeRTOS running on top of our RPi. The goal of this part is to learn the algorithmic challenges of multiprocessor scheduling, as we as gain an appreciation of the implementation issues. 

Our ARM processor has _four_ identical cores, so we will be considering the _identical machines_ multiprocessor model. In computer architecture, this is called the Symmetric MultiProcessor (SMP) architecture. In the case of multi-core processors, the SMP architecture applies to the cores, treating them as separate processors. SMP systems are tightly coupled multiprocessor systems with a pool of homogeneous processors running independently of each other. Each processor, executing different programs and working on different sets of data, has the capability of sharing common resources (memory, I/O device, interrupt system and so on) that are connected using a system bus or a crossbar.  

## [Subtask A] Partitioned EDF
The complexity in partitioned approaches to task scheduling come from the 
hardness of the task partitioning part (the "spatial" dimension), since the BIN PACKING problem is NP-Complete in the strong sense. Once an assignments of tasks to processor is found, we can simply run all the task allocated to one 
processor using the uniprocessor EDF policy. Thus, the "temporal" dimension of the partitioned scheduling problem is easy. Moreover, the tasks will be assumed to be independent (e.g., there is no resource sharing), so there is no need for the cores to communicate or synchronize, further simplifying matters. 

You will be using your EDF implementation from the previous project as the local scheduling policy. Since all processors are identical, it is rather safe to assume that any task can be allocated to any processor.

You will be implementing a Polynomial Time Approximation Scheme (PTAS) for task partitioning. In general, an algorithm A is called a PTAS for a problem if for every instance I of the problem and every given error tolerance parameter ɛ>0 (this is the desired accuracy of the solution returned by A and is supplied by the user):
1. The value of the solution returned by the algorithm, which we denote as A(I), is at most (1+ɛ) away from the value of the optimal solution, and 
2. it runs in time that is polynomial in |I|, where |I| is the size of the instance in binary encoding (but not necessarily polynomial in 1/ɛ). 

For instance, if the problem at hand is a 
minimization problem, and we denote the value of the optimal (minimum-value) solution as OPT(I) for instance I, then a PTAS A := A(I, ɛ) for the problem is such that A(I) ≤ (1+ɛ)OPT(I) for every instance I and ɛ. A valid running time for A would be, for instance, O((n/ɛ)<sup>1/ɛ<sup>2</sup></sup>), where n := |I|. 

[Hochbaum and Shmoys]() designed a PTAS for the partitioning of implicit-deadline sporadic task systems that behaves as follows. Given any positive constant 
_s_, if an optimal algorithm can partition a given task system _τ_ upon _m_ processors, then the algorithm will, in time polynomial in the representation of _τ_, partition _τ_ upon _m_ processors each of speed (1 + _s_). This can be thought of as a _resource augmentation_ result: the algorithm can partition, in polynomial time, any task system that can be partitioned upon a given platform by an optimal algorithm, provided it (the algorithm) is given augmented resources (in terms of faster processors) as compared to the resources available to the optimal algorithm. 

# [Task 4] Top-like Tool 

-------------------------------------------------------------------------------


### Grading Guidelines

We will use the following _approximate_ rubric to grade your work:		

| Task | Grade Contribution |		
|:----:|:---:|
| 1  (CBS) | 25% |
| 2 (CASH) | 25% |		
| 3 (Multiprocessor) | 50% |

Functionality apart, we will evaluate your submissions carefully along the following dimensions:
+ code style (i.e., readability and judicious choice of variable and function 
  names, etc);
+ clear Doxygen documentation of every function;
+ implementation of tests (high test coverage);
+ code-level comments as appropriate.

Remember that a substantial percentage of your grade will be assigned during 
the demos, so _prepare a nice demo!_ 

# Submission instructions
1. For Task 1 questions, submit your answers in a markdown file named `freertos-workings.md`
2. In `changes.md`, record all of the changes that you needed to make to 
   FreeRTOS in order to support EDF and SRP. Make sure to include all the 
   files that you altered as well as the functions you changed, and also
   list the new functions that you added. 
3. Create a markdown document named `design.md`. In this document, 
   include the all your design choices, including the overrun management 
   mechanism, as well as your EDF and SRP implementation strategies.
4. Create a document named `quant.md` and include in it the general strategy 
   you used to measure and evaluate the savings in the run-time stack space as
   a result of implementing stack sharing. Also record your findings, and 
   include graphs if necessary. 
5. **Testing**. Create a markdown document named `testing.md`. For every task,
   include the general testing methodology you used, in addition 
   to all the test cases. For each test case, provide an explanation
   as to the specific functionality/scenario that it tests. Also indicate the 
   result of each test case. 
6. **Bugs**. Create a file named `bugs.md` and include a list of the current bugs
   in your implementations.
7. **Future improvements**. Create a file named `future.md` and include a list 
   of the things you could do to improve your implementations, 
   have you had the time to do them. This includes optimizations, 
   decision choices, and basically anything you deemed lower priority `TODO`.




