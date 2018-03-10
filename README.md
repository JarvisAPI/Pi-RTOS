# Supporting Aperiodic Tasks and Multiprocessor Scheduling in FreeRTOS

CPEN 432 / Project 4 


## Introduction
In this programming assignment you will be extending FreeRTOS in two key directions. First, you will add support for servicing aperiodic, soft real-time requests alongside hard real-time tasks, by implementing reservation-based, dynamic server schemes for handling aperiodic tasks, on top of EDF. You will be using your EDF implementation from the previous programming assignment.

Second, you will be adding support for multiprocessor (multicore) real-time scheduling, making use of the four cores available in our RPi. You will implement both partitioned and global scheduling schemes. You will also develop a top-like tool that would allow the user to see the activity happening on each of the four cores, including the tasks already running on the processor and their states.


**This programming assignment consists of 4 tasks**, and each section 
describing a deliverable is labeled **[Task n]**, where n is the deliverable number. Make sure to read all the material preceding a task before 
attempting it. This assignment is worth 15% of your _raw_ grade.

# [Task 1] Constant Bandwidth Server (CBS) Support
 Read the chapter on Dynamic Priority Servers in the text (chapter 7). In essence, CBS allows tasks that exceed their nominal CPU reserve to continue execution, or stay in the ready queue, by postponing the deadline of the task. FreeRTOS provides methods for creating and managing periodic tasks. For this project, add a new type of task that would be managed by a constant bandwidth server. Creating a task that is managed by a CBS would be similar to creating a regular periodic task.
 
 Add the primitives needed to manage constant bandwidth servers at the programming interface and within the kernel/scheduler. It is possible to have multiple CBS tasks running concurrently. A software developer should be able to build an application that uses a combination of regular periodic tasks and constant bandwidth servers. Tasks are scheduled using EDF. Remember that priority ties are always broken in favor of the server. To simplify matters, you may assume that a job of a task is not released until the previous job of the same task is complete.
 
 
# [Task 2]  Implement the CASH approach for capacity sharing
CBS has a limitation in that when tasks do not fully use their budget, other tasks cannot exploit this extra processing capacity, i.e., all tasks use only their own budget and cannot leverage extra capacity that may result from jobs 
completing early. CASH is an approach that addresses this limitation. Read the article titled [Capacity Sharing for Overrun Control](http://ieeexplore.ieee.org/document/896018/) that appeared in the IEEE Real-Time Systems Symposium in 2000 and implement the CASH method described by Caccamo, et al. Several real-time operating systems do implement schemes that are rather similar to CASH when they co-schedule hard real-time and soft real-time tasks.


# [Task 3] Supporting Multiprocessor Real-time Scheduling in FreeRTOS
 In this part you will implement both partitioned and global EDF in FreeRTOS running on RPi. You will be considering only _implicit-deadline_ periodic tasks, where D<sub>i</sub> = P<sub>i</sub> for every task i. The different multiprocessor scheduling approaches (partitioned vs. global) pose different classes of challenges, both theoretically and implementation-wise, and the goal of this part is to learn the algorithmic complexities of multiprocessor scheduling, as well as gain an understanding and appreciation of the practical issues and 
 challenges related to supporting multiprocessor execution and scheduling.


Our ARM processor has _four_ identical cores, so we will be considering the _identical machine_ multiprocessor model. In computer architecture, this is called the Symmetric MultiProcessor (SMP) architecture. In the case of multi-core processors, the SMP architecture applies to the cores, treating them as separate processors. SMP systems are tightly coupled multiprocessor systems with a pool of homogeneous processors running independently of each other. Each processor, executing different programs and working on different sets of data, has the capability of sharing common resources (memory, I/O device, interrupt system and so on) that are connected using a system bus or a crossbar.  


_This task is fairly challenging!_ It is estimated that this task will consume at least 70% of your development effort for this programming assignment. This is partly due to the scarcity of 
available documentation on how our processor handles SMP. 
However, the following two documents contain most (if not all) of the documentation I know of regarding SMP in the Cortex A7 processor: 
1. [ARM Cortex-A Series Programmer’s Guide Version 4.0](http://cpen432.github.io/resources/arm-cortex-a-prog-guide-v4.pdf)
2. [Cortex-A7 MPCore Technical Reference Manual](http://cpen432.github.io/resources/arm-cortex-a7-mpcore-technical-reference-manual.pdf)

These documents should give you a good head start. Read chapters 13 and 18 of the Cortex-A7 programmer's guide. All the CPU registers relevant to SMP are detailed in the technical reference manual; in particular, the Multiprocessor Affinity Register `MPIDR` and the `CLUSTERID` field within.


_Your initial step should be to develop the code that boots the 4 cores_. It is suggested that you initially do SMP development _bare-metal_, separately from FreeROTS. Once you get the 4 cores up and running and are able to run code on each concurrently, you can start porting (integrating) your code to the given FreeRTOS port incrementally.  


**You will need to extended the FreeRTOS port to support multi-core processing**. Some of the extensions include the ability to specify the core on which a task runs, remove a task from a processor, or change the processor upon which the task is running (migration), etc. You may expose as many (programming) interfaces in order to complete this task in the cleanest possible way. Among the things that you should be thinking about are:

1. Interrupt handling and timer functionality: Do we need a single interrupt controller/timer per core or is one shared interrupt controller/timer sufficient? 
2. How to stop/start each core;
3. Dispatching tasks to certain cores, as well as task migration across cores;
4. Do you need a separate scheduler per core in partitioned scheduling? Is the
   situation different in global scheduling? 

You need not worry about resource sharing, nor do you need to worry about inter-task communication. You may assume that the tasks are independent and do not share resources; this assignment is challenging enough the way it is.


<!-- **Bonus:** If you are _really_ (like, _really_) up for a challenge, you may  -->
<!-- want to consider implementing resource sharing. Two (or more) tasks running on different cores might share resources.   -->

## [Subtask A] Partitioned EDF
The complexity in partitioned approaches to task scheduling comes from the 
hardness of the task partitioning part (the "spatial" dimension), since this is essentially a bin-packing problem and bin-packing is NP-Complete in the strong sense. Since exact solutions to the partitioning problem are computationally intractable (unless P = NP), our best
next hope is an approximation scheme with provable bounds on the quality of the solutions returned. Luckily for us, such approximation schemes exist for the partitioning problem, and you will be implementing one such scheme (described below). 

Once an assignment of tasks to processor is determined, we can simply run all the tasks allocated to one 
processor using the preemptive uniprocessor EDF policy. Once a task is "pinned" to a processor after the task partitioning stage, the task executes for good on that processor and never migrates. A task might be preempted on its assigned processor but should resume on the same processor. Consequently, each processor can have its own ready queue. Thus, the "temporal" dimension of the partitioned scheduling problem is relatively easy, since uniprocessor scheduling approaches may be leveraged directly. Moreover, the tasks will be assumed to be independent (e.g., there is no resource sharing), so there is no need for the cores to communicate or synchronize, further simplifying matters. 

Since all processors are identical, it is rather safe to assume that any task can be allocated to any processor. You will be using your EDF implementation from the previous project as the local scheduling policy. 

You will be implementing a Polynomial Time Approximation Scheme (PTAS) for task partitioning. In general, an algorithm A is a **PTAS** for a problem if for every instance I of the problem at hand and every given error-tolerance parameter ɛ>0 (this is the desired accuracy of the solution returned by A and is supplied by the user):
1. The value of the solution returned by the algorithm, which we denote as A(I), is at most (1+ɛ) away from the value of the optimal solution, and 
2. It runs in time that is polynomial in |I|, where |I| is the size of the instance in binary encoding (but not necessarily polynomial in 1/ɛ). [If the running time is also polynomial in 1/ɛ, then the algorithm is said to be a _Fully_ Polynomial-Time Approximation Scheme FPTAS]. 

For instance, if the problem at hand is a 
minimization problem, and if we denote the value of the optimal (minimum-value) solution as OPT(I) for instance I, then a PTAS A := A(ɛ) (i.e, A is a function of ɛ) for the problem is such that A(I) ≤ (1+ɛ)OPT(I) for every instance I and ɛ. 
The running time of algorithm A is a function T(|I|, 1/ɛ) that is polynomial in |I|. A valid running time for A would, for instance, be O((n/ɛ)<sup>1/ɛ<sup>2</sup></sup>), where n := |I|, or even O(n<sup>exp(1/ε)</sup>). Thus, as the 
tolerable error ɛ approaches 0, the accuracy of the solution approaches optimal, but the running time approaches infinity. This gives a clear tradeoff between running time and the quality of approximation.

As an example of a PTAS, consider the _Euclidean_ Traveling Salesman Problem (TSP): Given _n_ points in ℝ<sup>2</sup> (the plane) with Euclidean distances, i.e., d(x, y) = ||x − y||<sub>2</sub> for points x, y ∈ ℝ<sup>2</sup>, what is a shortest tour that visits each of the _n_ points and returns to the original point? There is a well known PTAS for this problem by Sanjeev Arora [[link to paper](https://dl.acm.org/citation.cfm?doid=290179.290180)] that behaves as follows: For every fixed c > 1 and given any _n_ nodes in ℝ<sup>2</sup>, a randomized version of the scheme finds a (1 + 1/c)-approximation to the optimum traveling salesman tour in O(n (log n)<sup>O(c)</sup>) time. 


Some approximation algorithms provide a solution for a _relaxed instance_ of the problem. For example, in packing problems, an algorithm may pack the items in bins whose sizes are slightly larger than the original. This type of algorithm is called a _dual_ approximation algorithm, or approximation with _resource augmentation_. Since most scheduling and resource allocation problems are packing problems at heart, a significant number of approximation schemes in the scheduling domain are resource augmentation approximations. A number of approximate feasibility tests for the schedulability of sporadic real-time tasks on a single processor have been developed as PTASes in the resource augmentation sense: The test is an approximation with respect to the amount of processor capacity that must be "sacrificed" for the test to become exact. For instance, [Fisher and Baruah](http://www.cs.wayne.edu/~fishern/papers/static_ptas_bounded.pdf) designed an approximation scheme to the response-time analysis whose inexactness may be described as follows (ε ∈ (0, 1)): 

> If the test returns "feasible", then the task set is guaranteed to be feasible on the processor for which it had been specified. If the test returns "infeasible", the task set is guaranteed to be infeasible on a slower processor, of computing capacity (1 - ε) times the computing capacity of the processor for which the task system had been specified.

In other words, if the test returns "infeasible", then the test asserts with certainty that the _more difficult task set where the execution time of every task is inflated by (1 - ε)_ is not feasible on a unit speed processor, but it cannot tell whether the original (easier to schedule) instance is feasible. The main result is the following: For any ε in the range (0, 1), there is an algorithm A(ε) that has running time O(n<sup>2</sup>/ε) (contrast this running time with that of the pseudopolynomial-time exact test) and exhibits the following behavior: 

On any synchronous periodic or sporadic task system τ with constrained deadlines:
* If τ is deadline-monotonic (DM) infeasible on a unit-capacity processor then Algorithm A(ε) correctly identifies it as being DM-infeasible;
* if τ is DM-feasible on a processor of computing capacity (1 − ε) then Algorithm A(ε) correctly identifies it as being DM-feasible;
* else Algorithm A(ε) may identify τ as being either feasible or infeasible.


[Hochbaum and Shmoys](https://dl.acm.org/citation.cfm?id=7535) designed a PTAS for the partitioning of implicit-deadline sporadic task systems. Their algorithm behaves as follows. Given any positive constant 
_s_, if an optimal algorithm can partition a given task system _τ_ upon _m_ processors, then the algorithm will, in time polynomial in the representation of _τ_, partition _τ_ upon _m_ processors _each of speed (1 + _s_)_. This can be thought of as a _resource augmentation_ result: the algorithm can partition, in polynomial time, any task system that can be partitioned upon a given platform by an optimal algorithm, provided it (the algorithm) is given augmented resources (in terms of faster processors) as compared to the resources available to the optimal algorithm. [In fact, the Hochbaum and Shmoys PTAS was designed for minimizing the *makespan of nonpreemptive jobs* on identical machines, but their algorithm extends readily to the implicit-deadline sporadic task model. Their algorithm is such that for any instance I and ɛ>0, the makespan of I corresponding to the allocation returned by the 
algorithm, C<sub>max</sub>(I), is such that C<sub>max</sub>(I) ≤ (1+ɛ)OPT(I), and it runs in time O((n/ɛ)<sup>1/ɛ<sup>2</sup></sup>).]


This is a theoretically significant result since it establishes that task partitioning can be performed to any (constant) desired degree of accuracy in polynomial time. However, the Hochbaum and Shmoys algorithm has poor implementation efficiency in practice: The constants in the run-time expression for this algorithm are prohibitively large. The ideas in Hochbaum and Shmoys' algorithm were applied in [this paper by Chattopadhyay and Baruah](http://ieeexplore.ieee.org/document/5767116/), published in IEEE Real-Time Technology and Applications Symposium (RTAS) in 2011, to obtain an implementation that is efficient enough to often be usable in practice. **Your task to implement the approach in Chattopadhyay and Baruah's paper**. 

The main idea of Chattopadhyay and Baruah's approach is to construct, for each identical multiprocessor platform upon which one will execute implicit-deadline sporadic task systems under partitioned EDF, a lookup table (LUT). Whenever a task system is to be partitioned upon this platform, this table is used to determine the assignment of the tasks to the processors.
The LUT is constructed assuming that the utilizations of all the tasks have values from within a fixed set of distinct values V. When this LUT is later used to actually partition of a given task system τ, each task in τ may need to have its worst-case execution time (WCET) parameter inflated so that the resulting task utilization is indeed one of these distinct values in V. The challenge lies in choosing the values in V in such a manner that the amount of such inflation of WCET’s that is required is not too large. _You will have to read the paper carefully for the implementation details_.

**Note:** The task partitioning scheme that you are asked to implement is executed offline (prior to system operation), so a valid design choice is to write the partitioning functionality as a _tool_ that is entirely separate from freeROTS, and just feed freeRTOS, at system startup, the task set and the partitioning produced by running your tool on the input task set. Make sure to document your design decisions.

## [Subtask B] Global EDF
In contrast to partitioned scheduling, global scheduling permits task migration (i.e., different jobs of an individual task may execute upon different 
processors), as well as job-migration: An individual job that is preempted may resume execution upon a different processor from the one upon which it had been executing prior to preemption. 

Thus, for global EDF scheduling, a single ready queue is maintained for all tasks and processors. Tasks are inserted into the global queue in EDF order, and 
the job at the head of the ready queue is dispatched to any processor.

Implement global EDF and add all the required support in freeRTOS. Also add a simple control test of your choice. 

# [Task 4] Top-like Tool
Here you will design an interactive console application that displays, for each of the four cores (a column for each core?), the tasks that are currently running on the core. This tool is particularly useful when the scheduling algorithm is global and tasks migrate across cores. The tool itself, when started, may be modeled as a periodic task that updates the display every _P_ seconds for some period _P_ of your choice. _The top task should not interfere with the hard-deadline tasks_. You may want to consider using a _server_ task to handle 
top instantiation as well as interactive top updates, which should be reflected on the console as soon as they take place. 

The tool should be started via the command `top` that the user can send to the RPi through UART. Once your kernel 
receives the string `top`, the display on the console to which the UART is connected is updated to show your top table, and no messages that your tasks (or kernel) print to the console (if any) should show while top is running. The top tool is terminated by sending the command `top -t` to the RPi. Only one instance of the top tool should be running at a time: If top is running and the user tries to instantiate another top instance (by sending the string `top` again), then this request is ignored. Add as many bells and whistles as you wish.  


-------------------------------------------------------------------------------


### Grading Guidelines

We will use the following _approximate_ rubric to grade your work:		

| Task | Grade Contribution |		
|:----:|:---:|
| 1  (CBS) | 20% |
| 2 (CASH) | 20% |		
| 3 (Multiprocessor: Partitioned + Global) | 50%: 30% + 20% |
| 4 (Top tool) | 10%|

Functionality apart, we will evaluate your submissions carefully along the following dimensions:
+ code style (i.e., readability and judicious choice of variable and function 
  names, etc);
+ clear Doxygen documentation of every function;
+ implementation of tests (high test coverage);
+ code-level comments as appropriate.

Remember that a substantial percentage of your grade will be assigned during 
the demos, so _prepare a nice demo!_ 

# Submission instructions

1. In a markdown file with the name `changes.md`, record all of the changes 
   (and additions) that you  
   made to FreeRTOS in order to support the functionality in every task. 
   Make sure to include all the 
   files that you altered as well as the functions you changed, and also
   list the new functions that you added. 
3. Create a markdown document named `design.md`. In this document, 
   include all your design choices, including brief explanation of how you
   implemented partitioned and global EDF. Detail the feasibility tests you used in each. Flowcharts are nice here! 
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
