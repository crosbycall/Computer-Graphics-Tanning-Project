COMP308 Final Project - Tanning Simulator
Tyler Jones
Callum Crosby
Tobias Mikkelsen


Building Instructions and Run Instructions (Do top to bottom):
mkdir build
cd build
cmake ../COMP308_FinalProject
make
cd ..
./build/bin/a3


Controls:
Left click drag: Move Camera around
Scrolling: Zoom camera in and out

Right click drag: Rotate selected joint about selected axis (only works if a joint is selected)
Control + Right click: Joint picking (click on sphere of the joint)
X key: change axis of rotation on selected joint

P key: pause / unpause tanning (prints current pause mode to console)
M key: rotate the torus 180 degrees about the y axis
R key: reset the torus' texture (reverts all tanning)
- key: decreases tan time (best to pause first then change time, also prints tan time to console)
= key: increase tan time
arrow keys: move monkey on x and y axis

Tan time is the amount of time it takes in seconds for a face to tan from full pale to full tan when light hits the face with dot product of 1.
Essentially its how long it takes for the torus to tan.

Selecting root joint works, it just wont highlight the joint

