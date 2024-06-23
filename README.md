# OpenDXMC
Monte Carlo radiation dose scoring application for diagnostic x-ray imaging

OpenDXMC is an application for calculation and visualization of dose distributions in diagnostic x-ray examinations. Its primary audience are medical phycisists and researchers who wants to perform monte carlo dose calculations for typical diagnostic x-ray examinations except mammography. The Monte Carlo code is not yet validated and use of this application should be for educational purposes only.

Dose calculations are done by a Monte Carlo method optimized for a voxelized geometry. However only photons are modelled and essentially only KERMA is scored. The application supports dose calculations on CT series of individiual patients and standard human reference phantoms made by ICRP (see report [110](https://www.icrp.org/publication.asp?id=ICRP%20Publication%20110) and [143](https://journals.sagepub.com/doi/full/10.1177/0146645320915031)). Implementation details for the monte carlo code can be found at https://dxmclib.readthedocs.io/ and source code for the simulation library at https://github.com/medicalphysics/DXMClib.    

![Dose calculation of dual source Flash scan](docs/screenshot/dethorax.png?raw=true)
Screenshot of dose calculation from a dual source CT scanner. The simulation shows radiation dose deposited in patient from a dual source coronary angiography scan technique.

Please do not hesitate if you encounter bugs or errors in the application, or simply missing a feature. The easiest way to report bugs or feature requests is to open an issue: https://github.com/medicalphysics/OpenDXMC/issues. 

## Installation
Precompiled binaries are distributed for Windows 64 bits only and may be obtained here: https://github.com/medicalphysics/OpenDXMC/releases

## Build from source
OpenDXMC is build with CMake and depends on the following libraries:
* VTK Visualization Toolkit: https://vtk.org/. OpenDXMC is tested with VTK version 9.3. When building VTK set the flag VTK_MODULE_ENABLE_vtkDICOM to WANT, since OpenDXMC require this module.
* Qt: https://www.qt.io/, version >= 6.7.
* HDF5: https://www.hdfgroup.org/.
* LibTorch: https://pytorch.org/get-started/locally/, version >= 2.3.1 for basic CT organ segmentation.

These libraries must be installed. Running cmake with standard settings should suffice to generate makefiles for your platform. The application is currently only tested for Windows 11 and Linux, but should compile on Mac. Errors when building for Windows, Linux and Mac are considered bugs. 


