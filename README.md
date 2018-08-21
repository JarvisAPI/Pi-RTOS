CPEN 432 / Project 3

Introduction
===
In this programming activity you will extend an industry standard Real-Time Operating System (RTOS), namely FreeRTOS, to support task-dynamic priority scheduling and resource access control. The following are some goals to this assignment:

* Understand the architecture and design choices behind a real-time operating system kernel;
* Extend the capabilities of the RTOS kernel;
* Gain an appreciation for the challenges of embedded software development;
* Improve upon work of others by examining prior engineering choices;
* Design and implement techniques for testing your modifications to the kernel.



**This programming assignment consists of 3 tasks**, and each section 
describing a deliverable is labeled **[Task n]**, where n is the deliverable number. Make sure to read all the material preceding a task before 
attempting it. This assignment is worth 15% of your _raw_ grade. 

------------------------------------------------------------------------------- 

# [Task 1] Install FreeRTOS and understand the scheduler
We will be working with a specific _port_ of the FreeRTOS kernel on RPi 2B. 
FreeRTOS is a generic RTOS for embedded computers and microcontrollers, and the 
official vanilla release (https://www.freertos.org) does not come with any 
hardware-specific implementations or device drivers. FreeRTOS, however, is 
_portable_, in the sense that it provides facilities for programmers to adapt
it to any platform/architecture. Typically, one has to provide 
hardware-specific implementations for interrupt handling, timer functionality, CPU operations (e.g., context switching code), MMIO, and any device drivers. The process of porting an OS to a new hardware platform should not alter the core code of the kernel; one 
instructs the ported OS to use these hardware-specific functions seamlessly via 
special interfaces (macros and stubs) that are designed for this purpose. The core of the OS remains oblivious to the specific platform upon which it is running. 

Luckily, there is a well-tested RPi 2B port of FreeRTOS that we will be using: 
https://github.com/iSerge/RPi2-FreeRTOS. The first step is to obtain the source code of this port and add it to your repository. 

The core files of the FreeRTOS kernel are `tasks.c`, `queue.c`, and `list.c`. There are three subtasks to carry out here:

1. Examine the porting mechanism that is used by the FreeRTOS port above. Write a short description of how porting is done, and give an example from 
the source code of how one hardware component is linked to FreeRTOS (provide a code snippet + explanation). 

2. Examine the implementation of the scheduler in FreeRTOS. Write a short 
description of the structure and the working of the scheduler. You may choose to include a flow chart. Also answer the following question: What is the purpose 
of the _idle task_ in FreeRTOS? Identify the idle task in FreeRTOS and briefly 
describe its implementation. 

3. Describe the sequence of events that take place from the moment a task 
   announces that it has finished execution for the current period until the 
   time the scheduler is called and the next task dispatched. Your description
   should include the sequence of function calls that take place, and how the 
   scheduler is invoked (does this happen on timer expiration? Are there other
   ways by which the scheduler may be invoked?). Pay particular attention to 
   how _software interrupts_ are used to invoke the scheduler. Include 
   the code snippet that contains the call to the software interrupt vector
   (this is in-line assembly) that ultimately leads
   to calling the scheduler, and describe briefly how the software interrupt 
   handler (service routine) in our port works and the basic operations it performs. 

Create a Wiki page `FreeRTOS Workings` and include in it your responses to the three foregoing subtasks. 

FreeRTOS is very well-documented, and you will find plenty of resources on FreeRTOS's official website. In particular, you will find the books listed in https://www.freertos.org/Documentation/RTOS_book.html extremely useful. However, nothing is as useful and rewarding as _reading the kernel code_ to understand how 
the components of interest work. 

# [Task 2] EDF Support
 Extend the scheduler to support EDF. You need to support _constrained-deadline_ systems where D<sub>i</sub> < P<sub>i</sub>. Implement admission control in the kernel, and provide the ability to accept new tasks as the system is running. 
 If the task set is constrained-deadline (there is a task T<sub>i</sub> with P<sub>i</sub> < D<sub>i</sub>), then you will have to carry out _exact_ EDF analysis (processor demand) to decide schedulability. Otherwise (i.e., implicit-deadline), of course one would use the LL bound since it is sufficient and necessary. 

You may use your `printk()` (after modification to use the UART implementation of the FreeRTOS port) to debug your scheduler. It is crucial that you 
demonstrate that the schedules produced are indeed EDF. Some messages printed to the console using your `printk()` should suffice. 

Also one has to deal with the situation when a deadline-miss occurs (transient overload). The 
overrun management mechanism is left to you as design choice. Would you drop the late job? Make it the lowest priority? Or restart the system altogether? 
Document your design choices clearly. 

Indicate all the changes that you made to FreeRTOS in order to support EDF. 
Document all the changes in a Wiki page titled `Changes to FreeRTOS`.

In a configuration file, expose some configurable constants that can be used by the user to enable/disable EDF and/or SRP. If the user chooses not to use your extensions to FreeRTOS, you should instead run the default mechanisms provided by FreeRTOS.

**Testing**: To see the difference in performance between the processor demand analysis and the LL bound, perform admission control on roughly 100 periodic 
tasks running concomitantly.  

# [Task 3] Support stack-based resource-access control (SRP)
To support resource sharing with the EDF scheduler, extend the semaphore implementation of FreeRTOS to use SRP. You only need to extend FreeRTOS with SRP for 
_binary semaphores_. The resource access protocol used in FreeRTOS is the Priority Inheritance Protocol.

Also extend the admission control tests developed in the previous part to include blocking times and test schedulability using EDF + SRP. It is up to you to decide how the worst-case estimates of the lengths of the critical sections are given to the scheduler.

**Hint:** You may want to utilize a stack (in the kernel) to keep track of the 
system ceiling and its evolution.

One of the consequences of using SRP is the ability to share the **run-time stack** (which is different from the stack used to implement SRP). Recall that each 
process must have a private run-time stack space, sufficient to store its context (the content of the CPU registers) and its local variables. Observe that in 
SRP, if two tasks have the same preemption level, they can never occupy stack 
space at the same time. Thus, the space used by one such task can be "reused" by the other, eliminating the need to allocate two separate stack spaces, one for each task, even if both are active (ready for execution). This is a consequence of the fact that in SRP, once a task starts execution, it can never blocked on any resources. Note that the storage savings resulting from sharing the run-time stack become more substantial as the number of tasks with the same preemption level increase. _In your implementation of SRP, provide support for sharing the run-time stack among the running tasks._ Read section 7.8.5 of the 
text, as well [T.P. Baker's paper](http://ieeexplore.ieee.org/document/128747/) on SRP for more details.

Again, develop suitable tests to demonstrate that your implementation of SRP 
and stack sharing are correct. To see the savings we reap as a result of sharing the run-time stack, carry out a quantitative study with stack sharing vs. no stack sharing. Report the gains in terms of the maximum run-time stack storage used. It is sufficient to run 100 tasks concomitantly for your quantitative analysis. 



-------------------------------------------------------------------------------


### Grading Guidelines

We will use the following _approximate_ rubric to grade your work:		

| Task | Grade Contribution |		
|:----:|:---:|
| 1  (understanding FreeRTOS) | 20% |
| 2 (EDF) | 40% |		
| 3 (SRP) | 40% |

Functionality apart, we will evaluate your submissions carefully along the following dimensions:
+ code style (i.e., readability and judicious choice of variable and function 
  names, etc);
+ clear Doxygen documentation of every function;
+ implementation of tests (high test coverage);
+ code-level comments as appropriate.

Remember that a substantial percentage of your grade will be assigned during 
the demos, so _prepare a nice demo!_ 


# Submission instructions
1. For Task 1 questions, submit your answers in a markdown file named `FreeRTOS Workings`
2. In `Changes to FreeRTOS`, record all of the changes that you needed to make to 
   FreeRTOS in order to support EDF and SRP. Make sure to include all the 
   files that you altered as well as the functions you changed, and also
   list the new functions that you added. 
3. Create a Wiki page named `Design`. In it, 
   include the all your design choices, including the overrun management 
   mechanism, as well as your EDF and SRP implementation strategies.
4. Create a Wiki page titled `Quantitative Evaluation of SRP` and include in it the general strategy 
   you used to measure and evaluate the savings in the run-time stack space as
   a result of implementing stack sharing. Also record your findings, and 
   include graphs if necessary. 
5. **Testing**. Create a Wiki page named `Testing`. For every task,
   include the general testing methodology you used, in addition 
   to all the test cases. For each test case, provide an explanation
   as to the specific functionality/scenario that it tests. Also indicate the 
   result of each test case. 
6. **Bugs**. Create a Wiki page titled `Bugs` and include a list of the current bugs in your implementations.
7. **Future improvements**. Create a Wiki page titled `Future Work` and include a list 
   of the things you could do to improve your implementations, 
   have you had the time to do them. This includes optimizations, 
   decision choices, and basically anything you deemed lower priority `TODO`.
