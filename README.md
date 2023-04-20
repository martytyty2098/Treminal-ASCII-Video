![GitHub](https://img.shields.io/github/license/hunar4321/life_code)
# Terminal ASCII Video
The C++ program that takes a video file and plays it in your terminal with grayscale ASCII characters\
This program is built on pure C++ with the help of OpenCV library.
# Usage
- Download `bin1.zip`, `bin2.zip` and `executable.zip`\
_The size limit on github is kinda low, so i split the files into different zip folders_
- Extract all the files into the same folder and run the executable like this:
<img width="523" alt="Screenshot_1" src="https://user-images.githubusercontent.com/108870368/232514678-c9d7df83-4f79-429a-bdc3-889626971b04.png">

- Follow displayed instructions

**`COOL THINGS`**
- The console window is fully resizable during real time video playback, but note that larger window size can result in a decrease in FPS, because more characters need to be rendered.
- The program supports all types of fonts, so you can set a small font to increase the resolution of the ASCII video, but too small a font can result in a decrease in FPS, as more characters must be displayed on the console screen.
# A shortcut
if you want to access a video on your desktop, instead of typing the full desktop path like:
```
C:\Users\username\Desktop\
```
you can just type:
```
-d
```
And then you will only need to pass name of the video with its extension:
```
videofile.mp4 (any other extension is also valid)
```
# Known issues
- This program only works on Windows
- The file path to the video passed to the program must be in english characters
- The shortcut to your desktop (`-d`) often doesnt work
- The higher the resolution of the original video, the lower the FPS, because it takes more time to shrink each frame to the size of the console window
- .dll files in `bin1.zip` and `bin2.zip` are kinda bulky
- FPS suddenly drops when there is a lot of motion and many different colors, and i dont know why.
- My code is bad and unreadable
