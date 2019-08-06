#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import numpy as np
import lp_schedule
import ffd
import time
import task_gen

class MockArgs(object):
    def __init__( self, procs, utilization, numtasks ):
        self.procs = procs
        self.utilization = utilization
        self.numtasks = numtasks


def main(args):
    
    for i in range(0.1, procs, 0.1):
        mock_args = MockArgs(args.procs, i, args.numtasks)
        task_gen.main(mock_args)


if __name__ == "__main__":
    PARSER = argparse.ArgumentParser(description='Generates datasets of total utilization'
                                                 'from 0.1 to max, in 0.1 increments, and compares'
                                                 'execution of each partitioning algorithm'
                                                 'takes in number of processors')
    PARSER.add_argument('--procs', dest='procs', action='store',
                        default=4, type=int, help='Number of processors')
    PARSER.add_argument('--numtasks', dest='numtasks', action='store',
                        default=256, type=int, help='Size of taskset')
    ARGS = PARSER.parse_args()
    main(ARGS)
