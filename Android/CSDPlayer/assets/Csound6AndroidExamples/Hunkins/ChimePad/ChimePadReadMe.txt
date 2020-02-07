INSTRUCTIONS FOR RUNNING CHIMEPAD.CSD ON ANDROID DEVICES
Art Hunkins (9/17/2012)
http://www.arthunkins.com
abhunkin@uncg.edu

IMPORTANT NOTE:
ChimePad.zip and ChimePad.csd represent an *advanced version* of the Android app, ChimePad.apk. Like ChimePad.zip, ChimePad.apk is available free at http://www.arthunkins.com. If you are not interested in either 1) substituting your own sound sets or 2) possibly tweaking various features of the Csound file itself (ChimePad.csd), you should download ChimePad.apk instead. It is much simpler to install and prepare for performance. (It also does not require knowledge of your device's user-available file system or how it interfaces with a computer.)

Even if you are interested in these advanced features, you might want to try ChimePad.apk first.

The Sample Performance outlined at the end of this document works equally with ChimePad.apk.


GENERAL

CHIMEPAD consists of ChimePad.csd, a Csound text file to be executed by CSD Player (an Android app). It also includes two sets of soundfile samples: 0.wav through 5.wav, and soundin.10 through soundin.15. The samples are all selected from the author's CHIMEPLAY (2012) for Csound5 (found at: www.arthunkins.com). Each set consists of one background loop (0.wav or 10.wav) and five chime samples. Both sets come from the St. Francis Prayer Center in Stoneville, NC; set 1 are outside chimes, set 2, inside. The user is free to substitute his/her own sample sets (see below).

CSD Player comprises both a version of Csound for Android and a basic GUI that can write WAV output or perform Csound (.csd) files in real time. One of the purposes of CHIMEPAD is to demonstrate the full potential of the performance interface. (I believe it to be the first creative effort to do so.)

PREPARATION

CHIMEPAD requires Android OS 2.3 or higher. CsoundApp-5.18.apk (or a more recent version) must first be downloaded and installed from:
http://sourceforge.net/projects/csound/files/csound5/Android/
There are two ways of preparing for ChimePad performance, depending on whether or not a computer is available. (Preparation is simpler if you have a computer.)

With a computer:
1) Either download and install CsoundApp-5.18.apk to your Android device, or download it to your computer, copy it via thumb drive or USB cable to your device (Download folder) and install it.
2) Download ChimePad.zip to your computer (if you haven't already) and unzip it, then copy all its files via thumb drive or USB cable to a folder (perhaps the Music folder or a new Csound folder) on your device.

Without a computer:
1) Download and install CsoundApp-5.18.apk to your Android device.
2) Unless already installed, download and install the AndroZip File Manager app from play.google.com . (Note: the WinZip app does not extract zip archives!)
3) Directly download ChimePad.zip to your device and use AndroZip to extract the archive to a folder of your choice (perhaps the Music folder or a new Csound folder).
 
RUNNING CHIMEPAD

Start CSD Player by clicking on its icon, Browse to the folder where you have placed ChimePad.csd and click on the file. Before hitting the CSD Player's OFF/ON button to start the performance, you may need to preset slider 1 (see below). All other settings may be selected and/or changed during performance, though the other sliders may be preset as well. When you are finished performing, hit OFF.

THE PERFORMANCE CONTROLS

General Rule: only one control can be activated (touched) at a time. CSD Player is truly a one-finger interface!

SLIDER 1 (must be set before performance begins):
This control selects which of the two sets of samples will be performed: below 50% on the slider, it is set 1 (outside chimes); above, set 2 (inside chimes). In addition, below 25% and above 75%, single-shot samples are given a long (2.025 second) decay; such a decay is appropriate for gradually decaying sounds (like the default chime sets). At a setting of between 25% and 75%, decay is short (.025 seconds); this is appropriate for most other samples.
NOTE: for the long decay setting, samples should be a minimum of 2.5 seconds in length.

SLIDER 2:
When set to zero, this control selects "manual play" mode, or "auto/random play" when set higher. Just above zero, notes are played by random chimes every 2 seconds; beyond this point a random number of seconds is added to this 2 seconds until at maximum setting, random bells occur from two to 20 seconds apart (i.e., the bells are increasingly infrequent).

SLIDER 3:
Controls random pan position during "auto play" only. At zero there is no effect; pan position is centered. As the control is raised, pan position randomly varies from the center position until at max, the entire range of positions from far left to far right is in play.

SLIDER 4:
Controls random amplitude of samples during "auto play" only. At zero, there is no effect; all notes are at full volume. As the control is raised amplitude varies from max to increasingly less - until at slider maximum, amplitude can be anywhere from max to zero.  

SLIDER 5:
Varies amplitude of the background loop (0.wav or 10.wav).

BUTTONS 1 to 5:
Trigger samples from set 1 (1.wav to 5.wav) or set 2 (10.wav to 15.wav), depending on set selected by Slider 1.
NOTE: Samples can be manually triggered (by buttons) even during "auto play" mode. Also, there is a notable delay between pressing a button and the beginning of the sample (up to .2 seconds). This is called "latency"; it will hopefully decrease with later versions of the Android OS. (Latency is also highly dependent on the CPU power of the particular Android device.)

TRACKPAD:
The trackpad simultaneously controls both pan position (x axis) and sample amplitude (y axis). Since only one control can be touched at a time, the trackpad cannot be touched while a button is being pushed. Instead, the trackpad "remembers" its last-touched values and applies them to the following event (e.g., button press or next "auto play" note). When Sliders 3 or 4 random action is initiated, the trackpad no longer controls whichever slider is advanced - though it continues to control whichever parameter is *not* set to random.

ALTERNATE SAMPLE SETS

Anyone can substitute his/her own sample set for set one (0.wav to 5.wav), set two (10.wav to 15.wav) or both sets. Each set can have a background loop or not, and up to five samples. Unused "sample slots" are ignored by the program. The only "requirement" is that you record in WAV format. (Also, single-shot samples should be at least 2.5 seconds in length, when "long decays" are in play [see SLIDER 1 above].) Users are particularly encouraged to create sets of closely related natural sounds.

Higher quality recording and editing is best done on equipment other than smartphones and tablets; the author records on a good handheld digital recorder, then edits with the excellent cross-platform Audacity software (free). Sound files are then simply copied from the editing computer (via USB cable or thumb drive) to the appropriate folder on the Android device.

It is possible, however, to do everything on your device, by downloading and installing the "TapeMachine Lite Recorder" app (from the Android store). This outstanding utility is by far the best free sound-editing app I've seen. TapeMachine carries no ads; it records WAV (at a variety of sample rates), loops, adjusts volume, and includes a multitude of user settings. It even sports a helpful user manual! The only limitation is that you can't record samples more than a minute long (which should not be a problem).

Recording hints for natural sounds:  
1) Record everything with the same equipment and settings.
2) Record everything in the same environment.
3) The volumes of single-shot samples should match (editing can help here, but try to minimize the necessity for it).
4) Avoid background noise. (Selecting the best location and time of day is often key.)
5) Your loop track should come from the same environment as your samples. Your loop is as consistent and "feature-free" as possible, and especially be devoid of your "one-shot" sounds. (The loop is primarily designed to mask or level background "noise.")
6) Be sure not to clip the head or tail of your samples. (The program has a built-in .025 second attack and decay time to help deal with clicks and other unwanted noise at these points.)

A SAMPLE PERFORMANCE

The sample performance "score" outlined below illustrates most of the capability of the CSD Player GUI, as well as what might be done with CHIMEPAD.csd. This realization can be heard at the composer's website (www.arthunkins.com).

Pay particular attention to the two spots where the manual and automatic (random) speeds of triggering should match. (A few extra Button triggers can help a potentially uneven transition, as can adjustment of the Slider 2 settings in steps 4 and 9.) Sudden shifts in level of activity are inappropriate for this realization.

The term "slap to 0" below means "touch 0." Alternately, "slide fast to 0." Remember, notes *cannot* be triggered manually while another finger is down.


Setup:
All initial settings are default (0). Do not touch Trackpad until step 12. OFF/ON button is preset to OFF.

Performance:
1) ON
2) Slider 5 gradually to 60%
3) Randomly trigger Buttons 1-5
4) Slap Slider 2 to 40% (setting should match rate of previous manual action); stop random triggering
5) Slider 3 gradually to max
6) Slider 2 gradually to 60%
7) Slider 4 gradually to max
8) Slider 2 gradually to 30%
9) Slap Slider 2 to 0, start to randomly trigger Buttons 1-5 (match rate of last auto-trigger)
10) Slap Slider 3 in several stages to 10%, continuing to random trigger
11) Slap Slider 4 in several stages to 10%, continuing to random trigger
12) Slap Slider 3 to 0, continue random triggering and add Trackpad X-axis random position (pan)
13) Slap Slider 4 to 0, continue triggering and add Trackpad Y-axis (amplitude) to random Trackpad positioning
14) Gradually slow Button triggering and lower the overall Y-axis (amplitude) on Trackpad
15) Repeat trigger a single Button (chime), slowing, at ever lower amplitude (Y-axis) and moving to an extreme left or right pan position
16) Stop random triggering; Slider 5 gradually to 0
17) OFF
