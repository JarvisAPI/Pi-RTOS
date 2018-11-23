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


tasks = []
tasks.append(make_task(1, 0.1))
tasks.append(make_task(2, 0.3))
tasks.append(make_task(3, 0.2))
tasks.append(make_task(4, 0.3))
tasks.append(make_task(5, 0.1))
tasks.append(make_task(6, 0.2))
tasks.append(make_task(7, 0.1))
tasks.append(make_task(8, 0.2))
tasks.append(make_task(9, 0.1))



bins = []
num_processors = 4
for i in range(0, num_processors):
    bins.append(0)

tasks.sort(key=lambda x: x.utilization, reverse=True)

for task in tasks:
    for i in range(0, len(bins)):
        if bins[i] + task.utilization <= 1:
            bins[i] += task.utilization
            task.processor = i
            break

print(bins)

for task in tasks:
    print(task.tasknum)
    print(task.processor)
    print(task.utilization)