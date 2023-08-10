# CardioSim

This repository contains C++ code for simulation of two dimensional cardiac tissue electrophysiology. Electrical excitation is represented by (Fenton-Karma 1998) equations and conductance - by four-state model of gap junction channel voltage gating (Snipas et al. 2020).

This model was used to perform numerical experiments in publication (will be published in Scientific Reports 2023?) and presented at ESC Congress of Cardiology (Maciunas et al. 2022).

Most of calculations are performed in parallel using GPU. Currently, we provide OpenGL version of code, because it is compatible with broad range of GPU's, even integrated ones, which are often used in laptops.

Altrough this repository contains fully functional code, it is under revision to make code more compact and reusable. Hopefully this will be done by the end of year 2023.
