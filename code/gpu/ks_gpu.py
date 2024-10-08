# -*- coding: utf-8 -*-

# Commented out IPython magic to ensure Python compatibility.
# %reset -f
import numpy as np
from numba import cuda
from scipy import stats
import math
import time
import cupy as cp
import numba
from numba.types import int32, float64, uint32, uint8

"""# **1. CPU Functions (Position Arguments)**"""

#aggregate shocks and mean-capital grids
def IXAKM_CPU(a,km):
    ixakm = a * ngridkm + km
    return ixakm

#aggregate shocks, mean-capital grids, idiosyncratic shocks and captial grids
def IXV_CPU(a,km,i,k): 
    ixv = ngridkm * nstates_id * ngridk * a\
        + nstates_id * ngridk * km\
        + ngridk * i\
        + k
    return ixv

#current and future aggregate/idiosyncratic shocks
def IXP_CPU(a,i,ap,ip):
    ixp = nstates_id * nstates_ag * nstates_id * a \
        + nstates_ag * nstates_id * i\
        + nstates_id * ap\
        + ip
    return ixp

def find_range_100_CPU(p,x):
  if(p == x[0]):
    return 0, 1
  elif(p > x[99]):  
    return 98, 99
  else:
    result_1=0
    result_2=0
    result_3=0
    for ii in range(99,0, -20):     
      if ii<0:
        ii=0
      if (p <= x[ii]):
        result_1 = ii
    for ii in range(4,-1, -1):     
      if (p <= x[result_1]):
        result_2 = result_1   
      result_1 = result_1 - 5
    for ii in range(5,-1, -1):     
      if (p <= x[result_2]):
        result_3 = result_2
      result_2 = result_2-1
    return result_3-1, result_3
def find_range(p,x,x_dim):
  for ii in range(2,x_dim):
    if p < x[ii-1] :
      idx_min = ii - 2
      idx_max = ii - 1
      return idx_min, idx_max
  return x_dim-2, x_dim-1

"""# **2. GPU Functions (Position, Interpolation)**

## **Position Arguments_GPU**
"""

#grid = [nstates_ag,ngridkm,nstates_id,ngridk]

############################# Used for Individual #############################

#aggregate shocks and mean-capital grids
@cuda.jit(device=True)
def IXAKM_GPU(a,km,grid):
    ixakm = a * grid[1] + km
    return int(ixakm)

@cuda.jit(device=True)
def IXV_GPU(a,km,i,k,grid): ##also used in aggregate_st, but name is IXVGPU
    ixv = grid[1] * grid[2] * grid[3] * a\
        + grid[2] * grid[3] * km\
        + grid[3] * i\
        + k
    return int(ixv)

#current and future aggregate/idiosyncratic shocks
@cuda.jit(device=True)
def IXP_GPU(a,i,ap,ip,grid):
    ixp = grid[2] * grid[0] * grid[2] * a \
        + grid[0] * grid[2] * i\
        + grid[2] * ap\
        + ip
    return int(ixp)

#aggregate/idiosyncratic shocks
@cuda.jit(device=True)
def IXAI_GPU(a,i,grid):
    ixai = a * grid[2] + i
    return ixai

"""## **Find Range**"""

@cuda.jit(device=True)
def find_range_100(p,x):
  if(p == x[0]):
    return 0, 1
  elif(p > x[99]):  
    return 98, 99
  else:
    result_1=0
    result_2=0
    result_3=0
    for ii in range(99,0, -20):     
      if ii<0:
        ii=0
      if (p <= x[ii]):
        result_1 = ii
    for ii in range(4,0, -1):     
      if (p <= x[result_1]):
        result_2 = result_1
      result_1 = result_1 - 5
    for ii in range(5,0, -1):     
      if (p <= x[result_2]):
        result_3 = result_2
      result_2 = result_2-1
    return result_3-1, result_3

@cuda.jit(device=True)
def find_range_4(p,x):
  for ii in range(2,4):
    if p < x[ii-1] :
      idx_min = ii - 2
      idx_max = ii - 1
      return idx_min, idx_max
  return 2, 3

@cuda.jit(device=True)
def find_range(p,x,x_dim):
  for ii in range(2,x_dim):
    if p < x[ii-1] :
      idx_min = ii - 2
      idx_max = ii - 1
      return idx_min, idx_max
  return x_dim-2, x_dim-1

"""## **Interpolation**"""

@cuda.jit(device=True)
def linear_interp2d(points,values,qp,qp2,grid):
    x0 = points[0]
    x1 = points[1]
    x0_dim = x0.size
    x1_dim = x1.size


    i0_min, i0_max = find_range(qp[0], x0, x0_dim)    #kmgrid
    i1_min, i1_max = find_range_100(qp[1], x1)        # kgrid
    '''
    i0_min, i0_max = find_range_4(qp[0], x0)          #kmgrid
    i0_min, i0_max = find_range(qp[0], x0, x0_dim)    #kmgrid
    i1_min, i1_max = find_range(qp[1], x1, x1_dim)
    '''
    
    #0:min, 1:max
    f_00 = values[IXV_GPU(qp2[0],i0_min,qp2[1],i1_min,grid)]
    f_10 = values[IXV_GPU(qp2[0],i0_max,qp2[1],i1_min,grid)]
    f_01 = values[IXV_GPU(qp2[0],i0_min,qp2[1],i1_max,grid)]
    f_11 = values[IXV_GPU(qp2[0],i0_max,qp2[1],i1_max,grid)]
    #weights
    tx = (qp[0] - x0[i0_min]) / (x0[i0_max] - x0[i0_min])
    ty = (qp[1] - x1[i1_min]) / (x1[i1_max] - x1[i1_min])
    #compute
    fp = f_00 * (1-tx) * (1-ty) + \
         f_10 *    tx  * (1-ty) + \
         f_01 * (1-tx) *    ty  + \
         f_11 *    tx  *    ty 
        
    return fp

"""# **3. Model Setup**

## **Parameters**
"""

beta = 0.99 #Discount factor
gamma = 1 #Utility function parameter
alpha = 0.36 #Capital Share in production function
delta = 0.025 #Deperciation Rate
delta_a = 0.01 #Production level; bad state : "1-delta_a", good state : "1+delta_a"
mu = 0.15 #Unemployment benefits (as share of wage)
l_bar = 1/0.9 #Time endowment; normalizes to 1 in bad state

epsilon_u = 0 #Idiosyncratic Shock if Unemployed
epsilon_e = 1 #Idiosyncratic Shock if Employed
ur_b = 0.1 #Unemployment Rate in Bad State
ur_g = 0.04 #Unemployment Rate in Good State
er_b = (1 - ur_b) #Employment Rate in Bad State
er_g = (1 - ur_g) #Employment Rate in Good State
kss = ((1/beta - (1-delta))/alpha)**(1/(alpha-1)) #Steady State Capital

"""## **States**"""

N = 10000 #Number of agents
T = 1100 #Period length
nstates_id = 2 #Number of States for the Idiosyncratic Shock
nstates_ag = 2 #Number of States for the Aggregate Shock
ngridk = 100 #Number of grids for k
ngridkm = 4 #Number of grids for m
nstates = nstates_id * nstates_ag * ngridk * ngridkm
grid = np.array([nstates_ag,ngridkm,nstates_id,ngridk]) #for gpu position argument in interpolation (not allocated yet)

#employment status array
epsilon = np.zeros(nstates_id)
epsilon[0] = epsilon_u
epsilon[1] = epsilon_e
#employment status array for stochastic simulation
epsilon2 = np.zeros(nstates_id)
epsilon2[0] = 1
epsilon2[1] = 2
#aggregate productivity shock array
a = np.zeros(nstates_ag)
a[0] = 1 - delta_a
a[1] = 1 + delta_a
#aggregate productivity shock array for stochastic simulation
a2 = np.zeros(nstates_ag)
a2[0] = 0
a2[1] = 1
#unemployment rate array
ur = np.zeros(nstates_ag)
ur[0] = ur_b
ur[1] = ur_g
#employment rate array
er = np.zeros(nstates_ag)
er[0] = er_b
er[1] = er_g
#kcross
kcross = np.zeros(N) + kss

"""## **Convergence Parameters**"""

#Convergence Parameters
toll_k = 1e-8
update_k = 0.7
toll_coeff = 1e-8
update_b = 0.3
ndiscard = 100

#parameters array (for individual)
params = np.array([alpha,l_bar,delta,mu,gamma,beta,update_k])

"""## **Shocks**"""

#Import Shocks
idshock = np.genfromtxt("./input/idshock.txt",dtype=np.uint8,delimiter='\t')
idshock = idshock - 1
agshock = np.genfromtxt("./input/agshock.txt",dtype=np.uint8,delimiter='\t')
agshock = agshock - 1

#agshock=np.random.randint(0,1,size=T,dtype='uint8')
#idshock=np.random.randint(0,1,size=(T,N),dtype='uint8')

"""## **$k∈[0,k_{max}]$, $m\in[m_{min},m_{max}]$**"""

k_min = 0.
k_max = 1000.
# CUDA kernels do not return anything, so you have to supply for an array to be modified. 
# All arguments have to be arrays, if you work with scalars, make them arrays of length one. 
k_min_aux_gpu = np.full(1,k_min) #np.zeros(1,dtype='float64')
k_max_aux_gpu = np.full(1,k_max)

x = np.linspace(0,0.5,ngridk)
y = (x**7) / np.max(x**7)

#Interval for k
k = k_min + ((k_max - k_min) * y)

km_min = 30.
km_max = 50.

#Interval for m
km = np.linspace(km_min,km_max,ngridkm)

"""## **Transition Matrix**"""

#Transition Matrix
Pr = np.zeros(IXP_CPU(nstates_ag-1,nstates_id-1,nstates_ag-1,nstates_id))          
Pr[IXP_CPU(0,0,0,0)] = 0.525
Pr[IXP_CPU(0,0,0,1)] = 0.35
Pr[IXP_CPU(0,0,1,0)] = 0.03125
Pr[IXP_CPU(0,0,1,1)] = 0.09375
Pr[IXP_CPU(0,1,0,0)] = 0.038889
Pr[IXP_CPU(0,1,0,1)] = 0.836111
Pr[IXP_CPU(0,1,1,0)] = 0.002083
Pr[IXP_CPU(0,1,1,1)] = 0.122917
Pr[IXP_CPU(1,0,0,0)] = 0.09375
Pr[IXP_CPU(1,0,0,1)] = 0.03125
Pr[IXP_CPU(1,0,1,0)] = 0.291667
Pr[IXP_CPU(1,0,1,1)] = 0.583333
Pr[IXP_CPU(1,1,0,0)] = 0.009115
Pr[IXP_CPU(1,1,0,1)] = 0.115885
Pr[IXP_CPU(1,1,1,0)] = 0.024306
Pr[IXP_CPU(1,1,1,1)] = 0.850694

"""## **Wealth and Initial $k'$**"""

trate = np.zeros(nstates_ag)
irate = np.zeros(IXAKM_CPU(nstates_ag-1,ngridkm))
wage = np.zeros(IXAKM_CPU(nstates_ag-1,ngridkm))
wealth = np.zeros(IXV_CPU(nstates_ag-1,ngridkm-1,nstates_id-1,ngridk))
kprime = np.zeros(IXV_CPU(nstates_ag-1,ngridkm-1,nstates_id-1,ngridk))

for ia in range(nstates_ag):
    #Tax Rate
    trate[ia] = mu * (1 - er[ia]) / er[ia]
    tr = trate[ia]
    for ikm in range(ngridkm):
        #Interest Rate
        irate[IXAKM_CPU(ia,ikm)] = alpha * a[ia] * ((km[ikm]/(er[ia]*l_bar)) ** (alpha-1))
        ir = irate[IXAKM_CPU(ia,ikm)]
        #Wage
        wage[IXAKM_CPU(ia,ikm)] = (1-alpha) * a[ia] * ((km[ikm]/er[ia]/l_bar)**alpha)
        w = wage[IXAKM_CPU(ia,ikm)]
        for iid in range(nstates_id):
            for ik in range(ngridk):
                #Wealth
                wealth[IXV_CPU(ia,ikm,iid,ik)] = ((1-delta+ir) * k[ik])\
                                            + (w*l_bar - mu*w*((1-er[ia])/er[ia]))*epsilon[iid]\
                                            + mu*w*(1-epsilon[iid])
                #kprime
                kprime[IXV_CPU(ia,ikm,iid,ik)] = 0.9 * k[ik]

"""## **Initial Aggregate Law of Motion**"""

kmprime = np.zeros(IXAKM_CPU(nstates_ag-1,ngridkm))
B = np.array([0,1,0,1])
regressors = 2

"""# **4. Individual**

## **Individual Kernel**
"""

@cuda.jit
def ind_ker(kprime_gpu,kmprime_gpu,kprimen_gpu,grid_gpu,trate_gpu,irate_gpu,wage_gpu,wealth_gpu,k_gpu,km_gpu,ur_gpu,er_gpu,a_gpu,epsilon_gpu,Pr_gpu,params_gpu,dif_k_gpu,k_max_gpu,k_min_gpu):
  #shared across all threads
  k_min_shd = cuda.shared.array(1,'float64')
  k_max_shd = cuda.shared.array(1,'float64')
  alpha_gpu = cuda.shared.array(1,'float64')
  l_bar_gpu = cuda.shared.array(1,'float64')
  delta_gpu = cuda.shared.array(1,'float64')
  mu_gpu = cuda.shared.array(1,'float64')
  gamma_gpu = cuda.shared.array(1,'float64')
  beta_gpu = cuda.shared.array(1,'float64')
  update_k_gpu = cuda.shared.array(1,'float64')
  km_shd = cuda.shared.array(4,'float64')
  k_shd = cuda.shared.array(100,'float64') 
  kprime_shd = cuda.shared.array(1600,'float64')
  wealth_shd = cuda.shared.array(1600,'float64')

  grid_shd = cuda.shared.array(4,'float64')
  ur_shd = cuda.shared.array(2,'float64')
  a_shd = cuda.shared.array(2,'float64')
  epsilon_shd = cuda.shared.array(2,'float64')
  Pr_shd = cuda.shared.array(16,'float64')

  km_shd = km_gpu
  k_shd = k_gpu
  kprime_shd = kprime_gpu
  k_min_shd = k_min_gpu[0]  
  k_max_shd = k_max_gpu[0]  
  alpha_gpu = params_gpu[0]
  l_bar_gpu = params_gpu[1]
  delta_gpu = params_gpu[2]
  mu_gpu = params_gpu[3]
  gamma_gpu = params_gpu[4]
  beta_gpu = params_gpu[5]
  update_k_gpu = params_gpu[6]
  wealth_shd = wealth_gpu

  ur_shd = ur_gpu
  a_shd = a_gpu
  epsilon_shd = epsilon_gpu
  Pr_shd = Pr_gpu
  grid_shd = grid_gpu

  tx = cuda.threadIdx.x #thread id
  ty = cuda.blockIdx.x #block id
  bw = cuda.blockDim.x #block dimension
  pos = (ty * bw) + tx #global thread id

  if pos < len(kprime_gpu) :
    #grid = [nstates_ag,ngridkm,nstates_id,ngridk]
    nstates_ag = int(grid_gpu[0])
    ngridkm = int(grid_gpu[1])
    nstates_id = int(grid_gpu[2])
    ngridk = int(grid_gpu[3])

    dim1 = ngridk * ngridkm * nstates_ag
    dim2 = ngridk * ngridkm
  
    iid = int(math.floor( pos / dim1 ))
    ia = int(math.floor( (pos - iid * dim1) / dim2 ))
    ikm = int(math.floor( (pos - iid * dim1 - ia * dim2) / ngridk))
    ik = pos - iid * dim1 - ia * dim2 - ikm * ngridk


    IXV_lcl = IXV_GPU(ia,ikm,iid,ik,grid_gpu)
    IXAKM_lcl = IXAKM_GPU(ia,ikm,grid_gpu)

    kmp = kmprime_gpu[IXAKM_lcl]
    kp = kprime_gpu[IXV_lcl]

    EMU = 0
    for iap in range(nstates_ag) :
      urp = ur_shd[iap]
      erp = 1 - urp
      ir = alpha_gpu * a_shd[iap] * ((kmp/(erp*l_bar_gpu)) ** (alpha_gpu-1))
      imrt = (1 - delta_gpu + ir)
      w = (1-alpha_gpu) * a_shd[iap] * ((kmp/(erp*l_bar_gpu))**alpha_gpu)
      for iidp in range(nstates_id) :
        k2 = linear_interp2d((km_shd,k_shd),kprime_shd,(kmp,kp),(iap,iidp),grid_shd)
        ep = epsilon_shd[iidp]
        c2 = (imrt*kp) - k2 + (w*ep*l_bar_gpu - mu_gpu*w*(urp/(1-urp)))*iidp + (mu_gpu*w)*(1-iidp)
        MU2 = (c2 ** (-gamma_gpu)) * imrt
        EMU += Pr_shd[IXP_GPU(ia,iid,iap,iidp,grid_gpu)] * MU2
    cn = (beta * EMU) ** (-1/gamma) 

    kprimen_gpu[IXV_lcl] = wealth_shd[IXV_lcl] - cn
    if kprimen_gpu[IXV_lcl] > k_max_shd :
      kprimen_gpu[IXV_lcl] = k_max_shd
    if kprimen_gpu[IXV_lcl] < k_min_shd :
      kprimen_gpu[IXV_lcl] = k_min_shd
    
    dif_k_gpu[IXV_lcl] = kprimen_gpu[IXV_lcl] - kprime_shd[IXV_lcl]
    if dif_k_gpu[IXV_lcl] >= 0 :
      dif_k_gpu[IXV_lcl] = dif_k_gpu[IXV_lcl]
    else :
      dif_k_gpu[IXV_lcl] = - dif_k_gpu[IXV_lcl]
            
    kprime_gpu[IXV_lcl] = update_k_gpu * kprimen_gpu[IXV_lcl] + (1-update_k_gpu) * kprime_shd[IXV_lcl]
    cuda.syncthreads()

"""## **Idividual Main**"""

def indiv_main(kmprime,kprime,B):
  #kmprime setup
  for ia in range(nstates_ag):
    const_coef = ia * regressors
    km_coef = ia * regressors + 1
    for ikm in range(ngridkm):
      kmprime[IXAKM_CPU(ia,ikm)] = np.exp(B[const_coef] + (B[km_coef] * np.log(km[ikm])))
      if kmprime[IXAKM_CPU(ia,ikm)] > km_max :
        kmprime[IXAKM_CPU(ia,ikm)] = km_max
      if kmprime[IXAKM_CPU(ia,ikm)] < km_min :
        kmprime[IXAKM_CPU(ia,ikm)] = km_min
  #kprimen setup
  kprimen = kprime.copy()
  dif_k = kprime.copy()
  #to device : kprimen,kmprime,kprime
  kprimen_gpu = cuda.to_device(kprimen.astype(dtype='float64'))
  kmprime_gpu = cuda.to_device(kmprime.astype(dtype='float64'))
  dif_k_gpu = cuda.to_device(dif_k.astype(dtype='float64'))
  kprime_gpu = cuda.to_device(kprime.astype(dtype='float64')) ## also used in aggregate_st, but should be different

  mdif_k = 1
  start1 = time.time()
  while mdif_k > toll_k :
    ind_ker[config1](kprime_gpu,kmprime_gpu,kprimen_gpu,grid_gpu,trate_gpu,irate_gpu,wage_gpu,wealth_gpu,k_gpu,km_gpu,ur_gpu,er_gpu,a_gpu,epsilon_gpu,Pr_gpu,params_gpu,dif_k_gpu,k_max_gpu,k_min_gpu)
    mdif_k_gpu = cp.max(cp.asarray(dif_k_gpu,dtype='float64'))
    mdif_k = cp.asnumpy(mdif_k_gpu)
  kprime=cp.asnumpy(kprime_gpu)
  elapsed1 = time.time() - start1
  print("Individual {}, {} seconds".format(it+1,elapsed1))
  return kprime

"""# **5. Aggregate_ST**

## **Aggregate_ST Kernal**
"""

@cuda.jit
def ag_st_ker(t,k_min_gpu,k_max_gpu,agshock_gpu,idshock_gpu,kcrossn_gpu,k_gpu,km_gpu,kprime_gpu,kmts_gpu,grid_gpu):
  tx = cuda.threadIdx.x #thread id
  ty = cuda.blockIdx.x #block id
  bw = cuda.blockDim.x #block dimension
  ij = (ty * bw) + tx #global thread id

  #constant across all threads
  k_min_shd = cuda.shared.array(1,'float64')
  k_max_shd = cuda.shared.array(1,'float64')
  km_shd = cuda.shared.array(4,'float64')
  k_shd = cuda.shared.array(100,'float64')
  kprime_shd = cuda.shared.array(1600,'float64')
  ia_shd = cuda.shared.array(1,'uint8')
  kmts_shd = cuda.shared.array(1,'float64')
  grid_shd = cuda.shared.array(4,'float64')

  ##declare shared variables
  k_min_shd = k_min_gpu[0]
  k_max_shd = k_max_gpu[0]
  #shared variables for interpolation
  km_shd = km_gpu
  k_shd = k_gpu
  kprime_shd = kprime_gpu
  grid_shd = grid_gpu
  #shared query points
  kmts_shd = kmts_gpu[t]
  ia_shd = agshock_gpu[t]

  if ij < len(kcrossn_gpu):
    kcrossn_lcl = kcrossn_gpu[ij]
    iid_lcl = idshock_gpu[t,ij]
    kcrossn_gpu[ij] = linear_interp2d((km_shd,k_shd),kprime_shd,(kmts_shd,kcrossn_lcl),(ia_shd,iid_lcl),grid_shd)
    if kcrossn_gpu[ij] < k_min_shd:
        kcrossn_gpu[ij] =  k_min_shd
    elif kcrossn_gpu[ij] > k_max_shd:
        kcrossn_gpu[ij] =  k_max_shd

    cuda.syncthreads()

def ag_st_main(kcross,kprime,km_min_gpu,km_max_gpu,grid_gpu,km_gpu,k_gpu,k_min_gpu,k_max_gpu,agshock_gpu,idshock_gpu):
  start2 = time.time()
  kprime_gpu = cuda.to_device(kprime.astype(dtype='float64'))
############################# AGGREGATE_ST KERNAL #############################
  for t in range(T): 
    kmts_gpu[t] = cp.mean(kcrossn_gpu_cp) 
    if kmts_gpu[t] > km_max_gpu:
      kmts_gpu[t] = km_max_gpu
    if kmts_gpu[t] < km_min_gpu:
      kmts_gpu[t] = km_min_gpu
    ag_st_ker[config2](t,k_min_gpu,k_max_gpu,agshock_gpu,idshock_gpu,kcrossn_gpu,k_gpu,km_gpu,kprime_gpu,kmts_gpu,grid_gpu)
###############################################################################
  # COPY BACK  
  kcrossn=kcrossn_gpu.copy_to_host() 
  kmts=cp.asnumpy(kmts_gpu)
  elapsed2 = time.time() - start2
  print("Aggregate {}, {} seconds".format(it+1,elapsed2))
  return kmts,kcrossn

"""# **6. Main**

## **GPU Set Up**
"""

# GPU specs : Individual
threadsperblock1 = 1024
blockspergrid1 = np.ceil(nstates / threadsperblock1).astype('int')
config1 = [threadsperblock1,blockspergrid1]

# GPU specs : Aggregate_ST
threadsperblock2 = 1024
blockspergrid2 = np.ceil(N / threadsperblock2).astype('int')
config2 = [threadsperblock2,blockspergrid2]

############################### to_device ONCE! ###############################

# used for both
k_min_gpu=cuda.to_device(k_min_aux_gpu.astype(dtype='float64'))
k_max_gpu=cuda.to_device(k_max_aux_gpu.astype(dtype='float64'))
km_gpu=cuda.to_device(km.astype(dtype='float64'))
k_gpu=cuda.to_device(k.astype(dtype='float64'))
grid_gpu=cp.asarray(grid)

# used for individual
params_gpu = cuda.to_device(params.astype(dtype='float64'))
trate_gpu = cuda.to_device(trate.astype(dtype='float64'))
irate_gpu = cuda.to_device(irate.astype(dtype='float64'))
wage_gpu = cuda.to_device(wage.astype(dtype='float64'))
wealth_gpu = cuda.to_device(wealth.astype(dtype='float64'))
ur_gpu = cuda.to_device(ur.astype(dtype='float64'))
er_gpu = cuda.to_device(er.astype(dtype='float64'))
a_gpu = cuda.to_device(a.astype(dtype='float64'))
epsilon_gpu = cuda.to_device(epsilon.astype(dtype='float64'))
Pr_gpu = cuda.to_device(Pr.astype(dtype='float64'))

# used for aggregate_st
km_min_gpu=cp.asarray(km_min)
km_max_gpu=cp.asarray(km_max)
agshock_gpu=cuda.to_device(agshock.astype(dtype='uint8'))
idshock_gpu=cuda.to_device(idshock.astype(dtype='uint8')) 

kmts = np.zeros(T)
kcrossn = kcross.copy() 
# cupy arrays.
kmts_gpu = cuda.to_device(kmts.astype(dtype='float64')) 
# cuda arrays
#kprime_gpu = cuda.to_device(kprime.astype(dtype='float64'))
kcross_gpu = cuda.to_device(kcross.astype(dtype='float64'))
kcrossn_gpu = cuda.to_device(kcrossn.astype(dtype='float64'))
kcrossn_gpu_cp = cp.asarray(kcrossn_gpu)

"""## **Main Iteration**"""

dif_B = 99999999
it = 0
init_time = time.time()
while dif_B > toll_coeff : 
  kprime = indiv_main(kmprime,kprime,B) 
  kmts,kcrossn = ag_st_main(kcross,kprime,km_min_gpu,km_max_gpu,grid_gpu,km_gpu,k_gpu,k_min_gpu,k_max_gpu,agshock_gpu,idshock_gpu)
  
  ibad,igood = 0,0
  xbad,ybad,xgood,ygood = [],[],[],[]
    
  for i in range(ndiscard,T-1):
    if agshock[i] == 0:
      ibad = ibad + 1
      xbad.append(np.log(kmts[i]))
      ybad.append(np.log(kmts[i+1]))
    else:
      igood = igood +1
      xgood.append(np.log(kmts[i]))
      ygood.append(np.log(kmts[i+1]))
  xbad = np.array(xbad)
  ybad = np.array(ybad)
  xgood = np.array(xgood)
  ygood = np.array(ygood)
    
  res1,res2 = stats.linregress(xbad,ybad), stats.linregress(xgood,ygood)
  B1 = np.array([res1[1],res1[0],res2[1],res2[0]])
  R2bad,R2good = res1[2]**2, res2[2]**2

  dif_B = np.linalg.norm(B-B1)
    
  B = B1*update_b + B*(1-update_b)
  it = it + 1
  print("---Iteration {}, dif_B = {}".format(it,dif_B))
  print(" ")
elapsed3 = time.time() - init_time
print("Total Iterations = {}, Total Time = {}".format(it,elapsed3))

