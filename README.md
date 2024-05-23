# WWVB_Controlled_Clock

The WWVB signal is a 60khz signal broadcasted by the National Institute and Standards of Technologies (NIST) which contains a "timecode" of the atomic clock. This clock is the most precise in the world, and a sample of the "time" is sent over broadcast for others to pick up and synchronize their watches/clocks with. Our goal was to capture this signal, demodulate it, and update our clock based on the signal received.

This project was the culmination of myself and two other Computer Engineers work. Our capstone project before graduating from Auburn University focused on creating the code necessary to decode a pulse-width-modulated signal and translate the "timecode" received from the said signal into input values. By translating the signal we could update the clock to the correct time before continuing with time-keeping.

I do not take credit for the work performed related to the "capturing" of the signal, or the "decoding" of the signal. My portion of the project was strictly the "time-keeping" portion. While I did create many variables, used some of the variables initialized in other methods, and large portions of the loop were written by me; the code specifically for the capturing and decoding was not written by me. I did the "time-keeping".
