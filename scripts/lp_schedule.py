#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import subprocess
import re

def generate_lp(procs, tasks, file_name):
    """Constructs an LP solve file that contains the LP formulation of the problem."""
    with open(file_name, 'w') as lp_file:

        var_names = ['j{}p{}'.format(task, proc)
                     for proc in range(procs) for task in range(len(tasks))]

        # Generate the optimization function
        lp_file.write('max: ' + ' + '.join(var_names) + ';\n\n')

        # Generate processor utilization bound constraints
        lp_file.write('\n')
        for proc in range(procs):
            proc_vars = var_names[len(tasks) * proc:(len(tasks) * proc)+len(tasks)]
            proc_terms = ['{} {}'.format(coef, proc_var)
                          for coef, proc_var in zip(tasks, proc_vars)]
            lp_file.write(' + '.join(proc_terms) + ' <= 1;\n')

        lp_file.write('\n')

        # Generate the mutually exclusive rules
        for task, _ in enumerate(tasks):
            cross_proc_vars = ['j{}p{}'.format(task, proc) for proc in range(procs)]
            lp_file.write(' + '.join(cross_proc_vars) + ' = 1;\n')

        # Set vars as binaries
        lp_file.write('\n')
        for var in var_names:
            lp_file.write('bin {};\n'.format(var))

        lp_file.write('\n')

def solve_lp(procs, file_name):
    """Runs the LP solver on a given file and parses the output."""

    # Run the LP solver and check for feasibility
    out = subprocess.run(['lp_solve', file_name], capture_output=True, text=True)
    try:
        out.check_returncode()
    except subprocess.CalledProcessError:
        print('Problem is not feasible!')
        return False

    # Parse the LP solver output
    proc_job_map = re.findall(r'j(\d+)p(\d+).*1', str(out.stdout))
    proc_job_map = sorted(proc_job_map, key=lambda item: item[1])
    for proc in range(procs):
        proc_jobs = filter(lambda item: item[1] == str(proc), proc_job_map)
        proc_jobs = map(lambda item: item[0], proc_jobs)
        print('Processor {}: {}'.format(proc, ' '.join(['J{}'.format(job) for job in proc_jobs])))

def main(args):
    """Main."""
    print('Partitioning {} tasks on {} processors.'.format(len(args.utilizations), args.procs))

    # Basic error checking
    for utilization in args.utilizations:
        if utilization > 1:
            print('Error: Cannot have a task with utilization greater than 1!')
            return

    print('Generating LP file')
    generate_lp(args.procs, args.utilizations, args.file)
    print('Generating LP file success')
    solve_lp(args.procs, args.file)


if __name__ == "__main__":
    PARSER = argparse.ArgumentParser(description='Generates a process partion scheme given a set'
                                                 'tasks and number of processors')
    PARSER.add_argument('--procs', dest='procs', action='store',
                        default=4, type=int, help='Number of processors')
    PARSER.add_argument('--file', dest='file', action='store',
                        default='lp.lp', type=str, help='LP file location')
    PARSER.add_argument('utilizations', metavar='N', type=float, nargs='+',
                        help='A task utlizaation')
    ARGS = PARSER.parse_args()
    main(ARGS)
