# Application of NASA core Flight System to Telescope Control Software for 2017 Total Solar Eclipse Observation

## About
The core flight system (cFS), developed by NASA, is a reusable software framework and a set of pluggable software applications that take advantage of the rich heritage of NASA’s successful space missions.
We applied the cFS to the development of telescope control software for the observation of the 2017 total solar eclipse.
We demonstrated the usefulness of the cFS to develop telescope control software through an eclipse observation system, the so-called DICE (DIagnostic Coronagraph Experiment) mission. 

## Instructions
### Embedded Software (FSW)

We developed the four main applications in core Flight System (cFS) 6.5a, but 6.6a is acceptable. To integrate the applications in the cFS, see the cFS deployment guide.

- Camera Control Application (CAM): It handles the QSI-640i model using KAI-04022 interline CCD and USB2 data communication.
- Wheel Control Application (WHL): It handles two filter wheels rotated by the motor, MOOG Animatics’ SM2316D-PLS2, a DC servo type with RS-232 communication.
- Observation Management Application (OBS): It orchestrates CAM and WHL synchronization by messages.
- Data Acquisition and Processing Application (DAP): It performs image processing, real-time image transfer, and science data storing.

### Control Software (GSW)

- TelemetryRouter.py: Standalone program to receive telemetry via UDP and distributes them to the ground systems via TCP/IP.
- bin/cmdutil: Standalone program to send commands via UDP.
- GroundSystem.py: Main control software with GUI.
- TelemetryFactory/: Python module package to create Telemetry object for serializing binary data to python data types.

### Operation Support Materials (OPS)

- GroundSystem.ini: Ground system configuration file.
- sequence#.seq: Examples of observation program files. The files should be located under the cf directory in the embedded computer.
- draw_fig8.pro: IDL procedure to draw images of the polarization characteristics, the polarized brightness (pB), total brightness (tB), degree of polarization (dP), and angle of polarization (aP).