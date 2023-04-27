# microtonal max external
This is the code for the max msp external denote_microtonal<br><br>
This external uses extended just intonation techniques and hi resolution pitch bend to create microtonal pieces. It requires an MPE enabled synth to use the per-note pitchbend. This necessarily also limits the maximum number of concurrent microtonal notes to 16 (the number of midi channels)<br><br>

## Requirements
- max sdk >= 8.2
- cmake >= 3.26
<br><br>
## Build instructions
- Get the max sdk, install to your favorite location
- In CMakeLists.txt:
    - Change the MAX_SDK_SRC variable to the source folder in the max-sdk
    - Set the C74_LIBRARY_OUTPUT_DIRECTORY variable to the output location (Ableton bundled Max or just regular Max)
- From the denote_microtonal directory:
    - Open terminal
    - `cmake -S . -B build`
    - `cd build && make -j13`

That should do it! You can check the destination folder for the denote_microtonal.mxo file to confirm everything went as it should have.
