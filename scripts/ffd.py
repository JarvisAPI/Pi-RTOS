#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import subprocess
import re


class Task(object):
    tasknum = 0
    processor = 0
    utilization = 0


def make_task(tasknum, utilization):
    task = Task()
    task.tasknum = tasknum
    task.utilization = utilization
    task.processor = 0
    return task


def parition_ffd(procs, utilizations):
    """Constructs an FFD model."""
    bins = []
    for i in range(procs):
        bins.append(0)
        
    taskset = []
    for i in range(len(utilizations)):
        taskset.append(make_task(i, utilizations[i]))

    taskset.sort(key=lambda x: x.utilization, reverse=True)
    
    for task in taskset:
        for i in range(len(bins)):
            if bins[i] + task.utilization <= 1:
                bins[i] += task.utilization
                task.processor = i
                break

    print(bins)

    taskset.sort(key=lambda x: x.tasknum)
    
    for i in range(procs):
        print("Processor " + str(i) + ":", end=" ")
        for task in taskset:
            if task.processor == i:
                print(" J" + str(task.tasknum), end=" ")
        print()



def main(args):
    """Main."""
    print('Partitioning {} tasks on {} processors.'.format(len(args.utilizations), args.procs))
    
    # Basic error checking
    for utilization in args.utilizations:
        if utilization > 1:
            print('Error: Cannot have a task with utilization greater than 1!')
            return


    parition_ffd(args.procs, args.utilizations)


if __name__ == "__main__":
    PARSER = argparse.ArgumentParser(description='Generates a process partion scheme given a set'
                                     'tasks and number of processors')
    PARSER.add_argument('--procs', dest='procs', action='store',
                                                         default=4, type=int, help='Number of processors')
    PARSER.add_argument('utilizations', metavar='N', type=float, nargs='+',
                                                         help='A task utilization')
    ARGS = PARSER.parse_args()
    main(ARGS)
