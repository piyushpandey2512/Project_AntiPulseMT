# Project_AntiPulse: Modular J-PET Simulation for Antimatter Gravity Studies

This repository contains the **Geant4-based simulation software** developed to assess the feasibility of using the **Modular J-PET (Jagiellonian Positron Emission Tomograph)** technology as a vertex detector for antimatter gravity experiments (specifically the AEgIS experiment at CERN).

## 🧩 Project Overview
The primary goal of this project is to simulate the detection of antihydrogen ($\overline{H}$) and antiproton ($\bar{p}$) annihilation events using plastic scintillator modules. By reconstructing the annihilation vertex with high precision, this system aims to measure the gravitational fall of antimatter using a Moiré deflectometer.

## ⚡ J-PET Technology & Implementation
Unlike traditional PET scanners that use crystal scintillators, **J-PET** utilizes cost-effective plastic scintillators with superior timing properties. This simulation models a dedicated **Modular J-PET** setup designed for the geometric constraints of the antimatter beamline.

### Key Simulation Features:
* **Detector Geometry:** Simulates a modular setup consisting of plastic scintillator strips (dimensions: $50 \times 0.6 \times 2.4$ cm$^3$) arranged in customizable configurations (e.g., 2-module or 4-module setups) to optimize angular acceptance.
* **Physics Models:** Utilizes the **FTFP_INCLXX** physics list to accurately model low-energy antiproton-nuclei annihilation and the subsequent production of secondary particles (charged pions $\pi^\pm$, kaons, etc.).
* **Signal Response:** Models the interaction of high-energetic pions (~230 MeV) in plastic scintillators, simulating energy deposition and hit positions essential for Time-of-Flight (TOF) reconstruction.
* **Vertex Reconstruction:** Includes algorithms to reconstruct the physical annihilation vertex by extrapolating tracks from consecutive hits in the scintillator modules.

## 📄 Related Publication
This software supports the results presented in the following paper:

> **Feasibility of $\overline{H}$ Annihilation-Vertex Reconstruction with Modular J-PET: A Simulation-Based Study**
> *P. Pandey, S. Sharma, P. Moskal, R.C. Ferguson, R. Caravita, et al.*
> Acta Physica Polonica A, Vol. 148, No. 6 (2025)
> [DOI: 10.12693/APhysPolA.148.S169](http://dx.doi.org/10.12693/APhysPolA.148.S169)

## 🛠️ Dependencies
* **Geant4** (v11.0+)
* **ROOT** (for data analysis and visualization)
* **CADMesh** (for importing STL geometries of the experimental chamber)
