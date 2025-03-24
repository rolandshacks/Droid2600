Droid2600
=========

Droid2600 is a Atari 2600 VCS emulator running on Android devices.

Special support for TV devices such as the Amazon TV Stick has been built in, including great integration for remote controls. Additionally, support for gamepads is a key feature - as classic gaming is't great fun on touch screens.

## Download Droid2600 APK

- [Latest Droid2600 Release](https://github.com/rolandshacks/Droid2600/releases/latest)

## Placing ROM files

Droid2600 supports loading cartridge ROMs as individual files (with extension ".bin", or ".a26") or bundled into a zip archive named
"droid2600.zip", "vcs2600.zip" or "atari2600.zip".

Droid2600 scans for these files in the following directories underneath the app install, or user data folder. Unfortunately,
loading ROM files or archives from the "Download" or "Documents" folder will probably not work in more recent Android versions.

The actual file system of your device might look different, but this would be one working
example location:

```
/data/user/0/org.codewiz.droid2600/games
```

Suggested way to load data to your device is to
bundle all .bin files into one "vcs2600.zip" archive.

## Using touch screen

Using a touch input (on smartphones and tablets) allows a multitude of things:

- left side is used as the virtual stick
- right side is used as the virtual fire button
- upper right corner is used to open the ROM selector
- bottom right corner is used to popup the menu
- upper left corner is used to trigger Console Reset
- upper middle area is used to trigger Console Select

## Using a gamepad

Button usage

- Button A: Fire
- Button L1: Console Reset
- Button R1: Console Select
- Button Start: Open ROM selector
- Button Select or Button Menu: Opening the menu
- Button Thumb-Right: Toggle zoom modes

Special buttons (such as those on a Amazon Fire TV Stick remote control)

- Button Pause: Toggle pause
- Button Fast-Forward: Enable warp mode
- Button Rewind: Disable warp mode

## Settings

Besides the straight forward options, there are some interesting things:

- Warp mode: Run emulation as fast as possible.
- Swap joystick: swap joystick to be used for the emulated port 1 (if swapping is enabled) or port 2 (default)

## Implementation Note

Droid2600 is based on the brilliant Stella multi-platform Atari 2600 VCS emulator.
Stella was originally developed for Linux by Bradford W. Mott, and is currently
maintained by Stephen Anthony. Thanks to the Stella team for the fantastic work!

--

Have fun!

Roland

PS. Contribution is welcome!









































Emulating the Atari 2600 VCS on Android devices.

Some facts:

- Optimized for TV devices (such as the Amazon Fire TV Stick).
- Supports gamepads (preferrable!)
- Uses hardware acceleration for rendering (OpenGL)
- Supports loading from game archives (zip file containing ROM images)
- Convenient auto-scan functionality
- Smart-touch interface for using touch screen borders and corners for special functionality

Download
--------

Download Droid2600 releases at

  https://github.com/rosc77/Droid2600/releases

Wiki
----

Read the Droid2600 wiki at
    
  https://github.com/rosc77/Droid2600/wiki
	
Copyright
---------

```
Copyright 2025 Roland Schabenberger

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

ROM Images
----------

There are no ROM images delivered with Droid2600. But good news is that
there are many sources to download them.

Open Source
-----------

Droid2600 is based on the brilliant Stella multi-platform Atari 2600 VCS emulator.
Stella was originally developed for Linux by Bradford W. Mott, and is currently
maintained by Stephen Anthony.

Thanks to the Stella team for the fantastic work!

See http://stella.sourceforge.net/ for details.
