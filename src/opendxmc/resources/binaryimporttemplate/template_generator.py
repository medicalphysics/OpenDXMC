# -*- coding: utf-8 -*-
"""
Created on Wed Nov  6 20:00:12 2019

@author: ander
"""

import numpy as np

def generate():
    
    dim = 256
    shape = (dim, dim, dim)
    #constructing water sphere
    matArr = np.zeros(shape, dtype=np.uint8)
    densArr = np.zeros_like(matArr, dtype=np.double)
    
    ind = np.indices(shape)
    sphere_radius = dim//4
    sphere_center = dim//2
    
    s_ind = (ii-sphere_center for ii in ind)
    idx, idy, idz = s_ind
    
    r = (idx*idx+idy*idy+idz*idz) < sphere_radius**2
    matArr[r] = 1
    densArr[:] = 0.001225 # air density g/cm3
    densArr[r] = 1.0 # water density
    
    material_map = list()
    material_map.append("0, Air, N0.75O0.25")
    material_map.append("1, Water, H2O")
    
    
    ##writing files
    with open("materialTemplate.dat", 'bw') as matFile:
        matFile.write(matArr.tobytes())
    with open("densityTemplate.dat", 'bw') as densFile:
        densFile.write(densArr.tobytes())
    with open("materialMapTemplate.dat", 'w') as matmapFile:
        matmapFile.write('\n'.join(material_map))
    
    
if __name__=='__main__':
    generate()
