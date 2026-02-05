"""Hypercube Splitting definitions."""

PROB_NAME = "HS" # Short abbreviated name for problem
FULL_PROB_NAME = "Hypercube Splitting" # Full name for problem in Title Caps
PROB_DEF = """A Hypercube Splitting HS(n,p,s) is a set of at least p hyperplanes in n dimensions that split at least s edges of the n-dimensional hypercube with vertices at {-1,+1}^n.  A hyperplane splits an edge between two vertices if one vertex is on one side of the hyperplane, and the other vertex is on the other side (a hyperplane that goes through a vertex does not split any edge that includes that vertex).  Represent the p hyperplanes by a p by n+1 matrix M (p rows, n+1 columns per row) where where hyperplane i is defined by the equation M_i,1*x_1 + M_i,2*x_2 + ... + M_i,n*x_n + M_i,n+1 = 0.5 (the 0.5 is to prevent hyperplanes from going through vertices).  Each element M_i,j is an integer with -20 <= M_i,j <= 20.  Given (n,p,s) we want to construct a HS(n,p,s).  For our purposes, n<20, p<=n, and s<=n*2^(n-1).  The solution matrix M should be output as p lines, one row per line, with n+1 space-separated integers on each line."""
PARAMS = ["n","p","s"] # parameter names

# *** Use these for the small dev instances:
DEV_INSTANCES = [[4,4,32],[5,4,78],[5,5,80],[6,4,184],[6,5,192],[7,4,410],[7,5,440],[7,6,448]]
OPEN_INSTANCES = [[6,4,185],[7,4,411],[7,5,441],[11,9,11259]]

# *** Use these for the large dev instances:
#DEV_INSTANCES = [[5,5,80],[6,5,192],[7,5,440],[7,6,448],[8,6,1016],[8,7,1024],[9,7,2298],[9,8,2304],[10,8,5114],[11,9,11258],[12,10,24576],[13,10,53008],[14,11,114286],[15,12,245252],[16,13,523430],[17,14,1114088]]
#OPEN_INSTANCES = [[10,8,5115],[11,9,11259],[13,10,53009],[15,12,245253]]


import itertools
import utils

def cut(m,v,neighbor):
    for p in m:
        totv = 0
        totn = 0
        for i in range(len(v)):
            totv += p[i]*v[i]
            totn += p[i]*neighbor[i]
        totv += p[len(v)]
        totn += p[len(v)]
        # print(str(totv) + " " + str(totn) + " : " + str(v) + " : " + str(neighbor))
        if (totv>0.5 and totn<0.5) or (totv<0.5 and totn>0.5):
            return True
    return False

def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    p = parmlist[1]
    array = utils.parsesoltext(soltext,p)
    return array

def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    n = parmlist[0]
    p = parmlist[1]
    s = parmlist[2]
    m = getsol(soltext,parmlist)
    if len(m)!=p:
        return False
    for r in m:
        if len(r)!=n+1:
            return False

    vertices = itertools.product([-1,1],repeat=n)
    numcut = 0
    for vt in vertices:
        v = list(vt)
        for i in range(n):
            if v[i]==1:
                neighbor = list(vt)
                neighbor[i] = -1
                if cut(m,v,neighbor):
                    numcut += 1
    print(str(numcut))
    if numcut>=s:
        return True
    else:
        return False

# x = """
# 1 0 0 0
# 0 1 0 0
# 0 0 1 0
# """
# print(str(v(x,[3,3,12])))
