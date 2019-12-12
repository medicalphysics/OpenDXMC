# OpenDXMC
Monte Carlo radiation dose scoring application for diagnostic x-ray imaging

OpenDXMC is a computer package to calculate and visualize dose distributions in diagnostic x-ray examinations. This package consists of a small library for Monte Carlo dose calculations for diagnostic x-ray energies. In addition the package includes a GUI for calculation of radiation dose from common CT examinations and planar x-ray apparatus. The application is currently in early development and an alpha release is available. The Monte Carlo code is not yet validated and use of this application should be for educational purposes only.

Dose calculations are done by a Monte Carlo method optimized for a voxelized geometry. However only photons are modelled and essentially only KERMA is scored. The application supports dose calculations on CT series of individiual patients and standard human reference phantoms made by ICRU (see report 110). Phantoms by [HelmholtzZentrum](https://www.helmholtz-muenchen.de/en/irm/service/virtual-human-download-portal/virtual-human-phantoms-registration/index.html) are supported but not distributed with the application. 

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


