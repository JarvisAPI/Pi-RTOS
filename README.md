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

**Important Note:** The kernel itself is a task, so the decision of which 
core runs the kernel task at any time should be considered in any 
multiprocessor scheduling scheme. _For simplicity, always execute the kernel 
on core 0 so that it is pinned to that core forever, even in global scheduling_. For the purposes of this assignment there is no need to migrate the kernel.

<!-- **Bonus:** If you are _really_ (like, _really_) up for a challenge, you may  -->
<!-- want to consider implementing resource sharing. Two (or more) tasks running on different cores might share resources.   -->

## [Subtask A] Partitioned EDF
The complexity in partitioned approaches to task scheduling comes from the 
hardness of the task partitioning part (the "spatial" dimension), since this is essentially a bin-packing problem (([Wiki](https://en.wikipedia.org/wiki/Bin_packing_problem))) and bin-packing is NP-Complete in the strong sense. In the bin-packing problem, a list of real numbers in (0,1] is to be packed into a minimal number of bins, each of which holds a total of at most 1. The latter describes the _optimization_ version of bin-packing. The task partitioning problem is actually equivalent to the _decision_ version of the bin-packing problem, which reads 
> Given a list of real numbers in (0,1] and _m_ bins each having capacity 1, is there a packing of the list into the _m_ bins such that the capacity of each bin is not exceeded?


Since exact solutions to the task partitioning problem are computationally intractable (unless P = NP), our best
next hope is an approximation scheme with provable bounds on the quality of the solutions returned. Luckily for us, such approximation schemes exist for the partitioning problem, and you will be implementing one such scheme (and one optional scheme if you opt to do it for extra credit; both described below). 

Once an assignment of tasks to processor is determined, we can simply run all the tasks allocated to one 
processor using the preemptive uniprocessor EDF policy. Once a task is "pinned" to a processor after the task partitioning stage, the task executes for good on that processor and never migrates. A task might be preempted on its assigned processor but should resume on the same processor. Consequently, each processor can have its own ready queue. Thus, the "temporal" dimension of the partitioned scheduling problem is relatively easy, since uniprocessor scheduling approaches may be leveraged directly. Moreover, the tasks will be assumed to be independent (e.g., there is no resource sharing), so there is no need for the cores to communicate or synchronize, further simplifying matters. 

Since all processors are identical, it is rather safe to assume that any task can be allocated to any processor. You will be using your EDF implementation from the previous project as the local scheduling policy.

You will be implementing and comparing **two** task partitioning schemes: An _exact_ scheme based on solving an Integer Linear Program (ILP), and the 
First Fit Decreasing (FFD) heuristic, which is an approximation. 
_For extra credit, you may implement an optional approximate scheme that returns a partitioning whose quality is specified by a given user-supplied 
error tolerance parameter, and which trades accuracy for running-time_. Such schemes are called are generally called Polynomial-time Approximation Schemes 
(PTAS) because the running time is polynomial is the size of the instance (but generally not in the error parameter). See below for details.

**Note:** All the task partitioning scheme that you are asked to implement are executed _offline_ (i.e., prior to system operation), so a valid design choice is to write the partitioning functionality as a _tool_ that is entirely separate from FreeROTS, and just feed FreeRTOS, at system startup, the task set and the partitioning produced by running your tool on the input task set. You may use any programming language for your partitioning tool. Make sure to provide detailed usage and compilation instructions for your tool, preferably a script for the latter. If your tool does not compile or we cannot figure out how to use it then we will not grade it. Again, make sure to document and justify your design choices.

**Quantitative study**  
You will be comparing the exact and the approximate approach(es) to task 
partitioning in terms of (1) the running-time, and (2) the quality of the solutions returned. The approximation factor for the FFD heuristic (below) considers worst-case and most difficult instances that push FFD to perform as poorly as possible relative to optimal. In practice, however, the instances of the task partitioning problem the are related to our domain may not realize the worst-case behavior, and the heuristics may perform much better than their worst-case. That's why it becomes important to know how far from optimal the heuristic we are considering is, using empirical analysis on real instances that are related to the application (here the application is scheduling). 


### [Subtask A.i] Exact Task Partitioning through ILP
In general, Linear programming (LP) is a method to achieve the best outcome (such as maximum profit or lowest cost) in a mathematical model whose requirements are represented by linear relationships. A linear programming problem may be defined as the problem of 
maximizing or minimizing a linear function subject to linear constraints. The constraints may be equalities or inequalities ([Wiki](https://en.wikipedia.org/wiki/Linear_programming)). 

For instance, consider the following optimization problem.
>>  A company makes two products (X and Y) using two machines (A and B). Each unit of X that is produced requires 50 minutes processing time on machine A and 30 minutes processing time on machine B. Each unit of Y that is produced requires 24 minutes processing time on machine A and 33 minutes processing time on machine B. At the start of the current week there are 30 units of X and 90 units of Y in stock. Available processing time on machine A is forecast to be 40 hours and on machine B is forecast to be 35 hours. The demand for X in the current week is forecast to be 75 units and for Y is forecast to be 95 units. Company policy is to maximize the combined sum of the units of X and the units of Y in stock at the end of the week.

How do we formulate this problem as an optimization program? What is an objective function that encodes the quantity to be maximized? Here, we need to decide how much of each product (X and Y) to make in the current week so as to maximize the combined sum of X and Y units that are left in stock at end of the week). To this end, let 
* x be the number of units of X produced in the current week
* y be the number of units of Y produced in the current week.

x and y are the _decision variables_, and we need assign them appropriate values to maximize the desired objective. Now let us write down the objective function as a function of our decision variables x and y. Going back to the objective, what is the number of units left in stock at the end of the week if we produce x units of product X and y units of product Y? For product x, there are 30 units in stock already, and the forecast demand of product X is 75 units, so if we produce x units of product X, then at the end of the week we will have (x+30-75) = (x-45) units of product X left in stock. Similarly, for product y, there are 90 units in stock already, and the forecast demand of product Y is 95 units, so if we produce y units of product Y, then at the end of the week we will have (y+90-95) = (y-5) units of product Y left in stock. Then the combined number of units of both products left in stock at the end of the week would be (x-45) + (y-5) = x+y-50 if we decide to produce x units of X and y units of Y. Thus the objective function (x+y-50), which is a linear function in the decision variables x and y.

Now what is the range of values that each of variables x and y may assume? Can they be assigned any real values? The answer is NO. For instance, x and y cannot be negative (they represent number of units produced), so one constraint is that x ≥ 0 and y ≥ 0. Notice that we are writing the constraints on the choices for x and y in terms of the decision variables x and y. Are there other constraints in the problem description that further limit the ranges of x and y? Here are the other constraints from the problem description, which come from the available processing times on each of machines A and B:
* _Available processing time on machine A is forecast to be 40 hours and on machine B is forecast to be 35 hours_

* _Each unit of X that is produced requires 50 minutes processing time on machine A and 30 minutes processing time on machine B_ **and** _each unit of Y that is produced requires 24 minutes processing time on machine A and 33 minutes processing time on machine B_.  
  Looking at each machine separately, we find that producing x and y units of each product would demand 50x + 24y minutes on machine A, which should not exceed the 40 hour processing time available on machine A. We also need 30x + 33y minutes on machine B, which should not exceed the 35 hour processing time available on machine B. Thus, we have the following two constraints
  
```
		50x + 24y ≤ 40 × 60 (mins/h)
		30x + 33y ≤ 35 × 60 (mins/h)
```

* Finally, the number of units produced of each of X and Y, in addition to the stock available already, should meet the forecast demand, thus we also have:

```
		x ≥ 75 (demand) - 30 (initial stock) 
		y ≥ 95 (demand) - 90 (initial stock)
```

Thus the combined linear program may be written as follows:

```
	maximize x+y-50
	subject to:
		50x + 24y ≤ 40 × 60
		30x + 33y ≤ 35 × 60
		x ≥ 45
		y ≥ 5
```

The constraints define the _feasibility region_ of the LP (which, here, is a subset of ℝ<sup>2</sup>), and is a convex polytope, which is the set defined as the intersection of the finitely many half spaces defined by the inequalities. If the LP is feasible, then an optimal solution would be a point lying on the boundary of constraint-defined convex polytope, and is really one of the vertices (i.e, one of the intersection points of the half spaces) of the polytope.


There are many algorithms to solve LPs, the most famous of which is the [Simplex Algorithm](https://en.wikipedia.org/wiki/Simplex_algorithm) (and all its variants). Although the Simplex algorithm runs in exponential time in the worst-case, it shows excellent average case behavior, and is usually preferred to other polynomial-time methods, such as _interior point_ methods (the LP problem is poly-time solvable). Note, however, that in the example LP above, we did not insist on integer solutions, so an optimal solution to this LP could very well be 
fractional. To add integrality constraints, one has to further restrict the decision variables x and y to be positive integers. This change dramatically changes the complexity of the problem; the problem becomes that of Integer Linear Programming (ILP) ([Wiki](https://en.wikipedia.org/wiki/Integer_programming)), and is NP-Complete in the strong sense. The LP algorithms mentioned above are no longer applicable when at least one of the decision variables is required to be an integer. There are exact solution methods for ILPs and, as you may expect, they are all exponential-time. 


In this part, you will be considering exact solution methods for ILPs (as you will formulate the task partitioning problem as an ILP), _but you will not be concerned here about how the solution methods work_. Instead, you will be using ready-made LP solver that you can use to solve your ILP 
_programatically_. That is, we will be using (I)LP solvers that expose an API through which you can specify your LP and invoke functions to return optimal solutions (if such solutions exist). There are many LP solver out there, but one of the best freely available API-based ILP solvers is `lp_solve` ([link to lp_solve](http://lpsolve.sourceforge.net/5.5/)). `lp_solve` is a free (see LGPL for the GNU lesser general public license) linear (integer) programming solver based on the revised simplex method and the Branch-and-bound method for the integers. It can also be called as a library from different languages like C, VB, .NET, Delphi, Excel, Java. It can also be called from AMPL, MATLAB, O-Matrix, Scilab, Octave, R via a driver program. `lp_solve` is written in ANSI C and can be compiled on many different platforms like linux and WINDOWS. Basically, lp_solve is a library, a set of routines, called the API that can be called from almost any programming language to solve ILP problems. There are several ways to pass the data to the library: Via the API, Via input files, and Via an IDE (all the details on how to use `lp_solve` are contained in the link above). 


Another API-based ILP solver is the commercial [Gurobi](http://www.gurobi.com). This is regarded as the most reliable and extensive ILP solver available. The Gurobi API is available in many programming languages, including C, C++, Java, .NET, Python, MATLAB, and R. This is a _really_ expensive optimization API, but, luckily for us, they have an educational license that gives you access to the full fledged Gurobi for _free_. However, in order to authenticate that you are affiliated with an educational institute, the license should be activated from within the UBC network, so make you sure that you are on campus when activating your Gurobi license or just be connected to the UBC network through VPN. The documentation, including installation and license activation instructions, is comprehensive, and is made available for every supported programming language.

**\*Note and Disclaimer:** I am in no way affiliated with Gurobi, and you are not restricted to using Gurobi, or even `lp_solve`. You may wish to use other LP solver out there; you have complete freedom to do so.

**ILP formulation of the task partitioning problem**  
The first step is to formulate the task partitioning problem as an ILP that you can feed to the ILP solver. Because (1) we are using EDF as the local (per core) scheduling policy, (2) our cores are identical, and (3) we are considering implicit-deadline tasks, the LL EDF utilization bound is both sufficient and necessary, and therefore you may view each core as a bin of unit capacity. The items to be packed into each core are the task utilizations _u_<sub>1</sub>, ..., _u_<sub>_n_</sub>, where _n_ is the number of tasks. 

We wish to find an assignment of tasks to cores so that no core overflows. If no such assignment exists, then you should declare that the task set not schedulable. To represent an assignment of tasks to processors, one may utilize a variable _x<sub>i,j</sub>_ such that _x<sub>i,j</sub>_ = 1 if task _i_ assigned to processor _j_, and is 0 otherwise. If the number of processors is _m_, then we will need _nm_ variables. These variables will be the decision variables of your ILP, and since each is 0-1 valued, we are dealing with an Integer LP. You task is to write the (utilization) constraints of the ILP as linear functions of the variables _x<sub>i,j</sub>_, _i_ ∈ {1, ..., _n_}, _j_ ∈ {1, ..., _m_}. There is one constraint per processor.



### [Subtask A.ii] Approximate Task Partitioning through FFD
Since exact solution methods to the task partitioning are doomed to be exponential-time (unless P = NP), one is sometimes willing to trade exactness for computational efficiency. In the face of intractability, one's next best option is an approximation scheme that runs in time that is polynomial in the size of the input _with provable bounds on the quality of the solutions_. 

For instance, The First-Fit Decreasing (FFD) algorithm for the bin-packing problem packs each number in order of non-increasing size into the first bin in the list of open bins so far with sufficient remaining space. If none of the bins used so far have sufficient remaining capacity to hold the new item, a new bin is opened and the item is inserted into this new bin. The FFD heuristic produces, for every instance I of the bin-packing problem, a packing of items to bins that uses at most (11/9)OPT(I) + 6/9 bins, where OPT(I) is the number of bins in an optimal packing of instance I, and this bound is [tight](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.158.4834&rep=rep1&type=pdf).  



### [Optional] A PTAS for task partitioning (Bonus 25 points) 
In general, an algorithm A is a **PTAS** for a problem if for every instance I of the problem at hand and every given error-tolerance parameter ɛ>0 (this is the desired accuracy of the solution returned by A and is supplied by the user):
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
| 1  (CBS) | 25% |
| 2 (CASH) | 15% |		
| 3 (Multiprocessor: Partitioned (exact + FFD) + Global) | 40%: (15% + 10%) + 15% |
| 4 (Top tool) | 20% |

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
