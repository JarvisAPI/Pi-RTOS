
from pyscipopt import Model, quicksum, multidict

def BinPackingExample():
    B = 9
    w = [2,3,4,5,6,7,8]
    q = [4,2,6,6,2,2,2]
    s=[]
    for j in range(len(w)):
        for i in range(q[j]):
            s.append(w[j])
    return s,B


def FFD(s, B):
    remain = [B]
    sol = [[]]
    for item in sorted(s, reverse=True):
        for j,free in enumerate(remain):
            if free >= item:
                remain[j] -= item
                sol[j].append(item)
                break
        else:
            sol.append([item])
            remain.append(B-item)
    return sol


def bpp(s,B):
    n = len(s)
    U = len(FFD(s,B))
    model = Model("bpp")
    x,y = {},{}
    for i in range(n):
        for j in range(U):
            x[i,j] = model.addVar(vtype="B", name="x(%s,%s)"%(i,j))
    for j in range(U):
        y[j] = model.addVar(vtype="B", name="y(%s)"%j)
    for i in range(n):
        model.addCons(quicksum(x[i,j] for j in range(U)) == 1, "Assign(%s)"%i)
    for j in range(U):
        model.addCons(quicksum(s[i]*x[i,j] for i in range(n)) <= B*y[j], "Capac(%s)"%j)
    for j in range(U):
        for i in range(n):
            model.addCons(x[i,j] <= y[j], "Strong(%s,%s)"%(i,j))
    model.setObjective(quicksum(y[j] for j in range(U)), "minimize")
    model.data = x,y
    return model

    return model, x, y


def solveBinPacking(s,B):
    n = len(s)
    U = len(FFD(s,B))
    model = bpp(s,B)
    x,y = model.data
    model.optimize()
    bins = [[] for i in range(U)]
    for (i,j) in x:
        if model.getVal(x[i,j]) > .5:
            bins[j].append(s[i])
    for i in range(bins.count([])):
        bins.remove([])
    for b in bins:
        b.sort()
    bins.sort()
    return bins


s,B = BinPackingExample()
solveBinPacking(s,B)
