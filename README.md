# M5-Signal-Multimeter
Displays THD, FFT, Oscillo of a signal

![](https://i.imgur.com/sfoJP5h.jpg)

When connected to an split core current transformer like the SCT013-020, it helps determine how clean the electricity is and you can identify if a motor is failing.

THD: Total Harmonic Distortion is an indication of how dirty the electricity is. 11% of the maximum for anything below 20kV.
FFT: The spectrogram of your signal shows you the spikes if you have strong harmonics, this helps identify where the distortion comes from. For example a strong 3rd peak usually indicates electric pollution by computer, LED, CFL
Oscillo: no trigger lock yet, will eventually be useful to see what machine is eating electricity

USAGE: Compile with M5Stack in your library folder and eft32 in your hardware folder.

NOTE: measurements are inacurate and the FFT looks off even for a square wave
