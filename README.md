# OpenDXMC
Monte Carlo radiation dose scoring application for diagnostic x-ray imaging

OpenDXMC is an application for calculation and visualization of dose distributions in diagnostic x-ray examinations. Its primary audience are medical phycisists and researchers who wants to perform monte carlo dose calculations for typical diagnostic x-ray examinations except mammography. The application is currently in development and an alpha release is available. The Monte Carlo code is not yet validated and use of this application should be for educational purposes only.

Dose calculations are done by a Monte Carlo method optimized for a voxelized geometry. However only photons are modelled and essentially only KERMA is scored. The application supports dose calculations on CT series of individiual patients and standard human reference phantoms made by ICRU (see report 110 and 143). Phantoms by [HelmholtzZentrum](https://www.helmholtz-muenchen.de/en/irm/service/virtual-human-download-portal/virtual-human-phantoms-registration/index.html) are supported but not distributed with the application as they require an individual licence. Implementation details for the monte carlo code can be found at https://dxmclib.readthedocs.io/ and source code for the library at https://github.com/medicalphysics/DXMClib.    

![Dose calculation of dual source Flash scan](docs/screenshotdethorax.png?raw=true)

There are currently two major show-stoppers for users of this application; lack of documentation and lack of validation of the monte-carlo code. In the short term, these issues are the main focus of further development of the application.

Please do not hesitate if you encounter bugs or errors in the application, or simply missing a feature. The easiest way to report bugs or feature requests is to open an issue: https://github.com/medicalphysics/OpenDXMC/issues. 

## Installation
Precompiled binaries are distributed for Windows 64 bits only and may be obtained here: https://github.com/medicalphysics/OpenDXMC/releases

## Build from source
OpenDXMC is build with CMake and depends on the following libraries:
* xraylib by T. Schoonjans: https://github.com/tschoonj/xraylib
* VTK Visualization Toolkit: https://vtk.org/. Version 8.2 is not supported due to issues with vtkDICOM module. For OpenDXMC it is recommended to download and bild VTK master branch for linking with OpenDXMC. When building vtk set the flag VTK_MODULE_ENABLE_vtkDICOM to WANT, since OpenDXMC require this module.
* Qt: https://www.qt.io/ version >= 5.12.
* HDF5: https://www.hdfgroup.org/.

These libraries must be installed. Running cmake with standard settings should suffice to generate makefiles for your platform. The application is currently only tested for Windows 10, but should compile on Linux and Mac. Errors when building for Linux and Mac are considered bugs. 


