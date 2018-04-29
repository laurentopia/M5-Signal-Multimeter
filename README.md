# M5-Signal-Multimeter
Displays FFT, Waterfall, Hz, THD and Oscillo of a signal

![](https://i.imgur.com/sfoJP5h.jpg) ![](https://i.imgur.com/nnu565o.jpg)

Currently not very useful because the Hz measurement is wrong, the FFT scale is linear and should be log, but it's pretty.

The goal:
When connected to an split core current transformer like the SCT013-020, it helps determine how clean the electricity is and you can identify if a motor is failing.

THD: Total Harmonic Distortion is an indication of how dirty the electricity is. 11% of the maximum for anything below 20kV.
FFT: The spectrogram of your signal shows you the spikes if you have strong harmonics, this helps identify where the distortion comes from. For example a strong 3rd peak usually indicates electric pollution by computer, LED, CFL
Oscillo: no trigger lock yet, will eventually be useful to see what machine is eating electricity

USAGE: Compile with M5Stack in your library folder and eft32 in your hardware folder.
Connect the source to pin 35 and the ground to GND

NOTE: measurements are inacurate
