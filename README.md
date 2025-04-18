# LR2HackBox
![preview](https://github.com/user-attachments/assets/3ea78107-1466-4aac-ab60-bcc4ad83cd01)

A collection of various QoL (Quality of Life) features and whatever else i may come up with in the future for Lunatic Rave 2.

The .dll file must be injected into the game process in any convenient way. I recommend using https://github.com/SayakaIsBaka/lr2_chainload for automatic injection on game launch.

ImGui menu from which you can control all of the features can be brought up with 'INSERT' key in-game. It can be later rebound to a different key from the menu behind "Binds" button in the bottom-right corner.

Features:
- Unrandomizer – Allows you to control which arrange 'RANDOM' function will result in. Currently works only for 7K SP mode. Also has a history of each random arrange you got, regardless of using unrandomizer or not.
- R-Random – 'RANDOM' function acts as R-Random, where columns "rotate", or cyclically shift, by some offset, as well as have a chance to mirror.
- Restart Tweaks – Adds ability to restart from play scene with new or same random arrange (if random is used), as well as from result screen.
- Random Select – Adds 'RANDOM SELECT' entry as the last element of song select list, when a song folder is opened. Upon selection of that entry, a song from that folder will be started at random.
- MainBPM hi-speed mode – Replaces AvgBPM hi-speed fix mode with MainBPM, which selects the BPM which most notes in the chart use to calculate the speed against.
- Reroute Screenshots – Reroutes screenshots to save into a separate folder 'screenshots' instead of the root game folder.
- Screenshots into copybuffer – Puts the screenshots in the copybuffer, to later access them with CTRL+V.
- Mirror Lanecover Buttons – Mirrors the binds to adjust lanecover from play scene, making them 2 and 1 instead of 6 and 7, which can be useful for P2 players.
- Hide Scratch Notes – Makes scratch notes invisible. For no practical reason. Only works for 7K SP.
- Metronome – Enables the metronome sound in playing scene. SFX can be customized with 'metronome-beat.wav' and 'metronome-measure' in 'LR2files\Sound\LR2HackBox\' directory for respective sounds. If those files are not present, it will default to using samples for closing and opening (in that order) a folder.
