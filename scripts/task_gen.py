#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import numpy as np
import lp_schedule
import ffd
import time


class MockArgs(object):
    def __init__( self, procs, utilizations, file ):
        self.file = file
        self.utilizations = utilizations
        self.procs = procs



def main(args):
    tasks = np.random.random(args.tasks)
    tasks *= (args.util /tasks.sum())

    mock_args = MockArgs(args.procs, tasks, 'lp.lp')

    # Run FFD
    ffd_start = time.time()
    ffd.main(mock_args)
    ffd_end = time.time()
    print( 'FFd: ', ffd_end - ffd_start)

    # Run LP
    lp_start = time.time()
    lp_schedule.main(mock_args)
    lp_end = time.time()


    # Print cool results
    print( 'LP: ', lp_end - lp_start)
    print( 'FFd: ', ffd_end - ffd_start)





if __name__ == "__main__":
    PARSER = argparse.ArgumentParser(description='Generates a process partion scheme given a set'
                                                 'of task utilizations and number of processors')
    PARSER.add_argument('--procs', dest='procs', action='store',
                        default=4, type=int, help='Number of processors')
    PARSER.add_argument('--util', dest='util', action='store',
                        default=1.0, type=float, help='Set of task utilizations')
    PARSER.add_argument('--tasks', dest='tasks', action='store',
                        default=256, type=int, help='Size of taskset')
    ARGS = PARSER.parse_args()
    main(ARGS)
