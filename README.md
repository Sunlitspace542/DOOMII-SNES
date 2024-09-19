# Doom II MAP01+MAP02 SNES Source Code

Here's the source code to my Doom II SNES techdemo.    
``map_sources`` has what I have left of the original Doom Engine format source maps.  
``art_sources`` has the original PNGs used for the title and intermission art replacements.  
``msu1_data`` is the source to the MSU-1 data and PCMs.  

# DOOM-FX
Doom/FX for Super Nintendo with SuperFX GSU2A  

## To assemble:  

``When starting WinUAE, ensure your configuration is loaded before clicking Start, and any changes you make to the configuration are saved!``  

1. Set up an emulated Amiga 4000 in WinUAE. [Follow this tutorial up to 4:09 to set up the A4000.](https://youtu.be/Cqu2NAZ9dgg)  

2. in RAM settings, increase Chip to 8 MB and 32-bit chip to at least 16 MB. (The build process puts the object files to be linked in a RAMdisk, so we need enough free space to link everything)  

3. Install SAS/C  
- Extract the included ``sasc650.zip`` in ``01-prereqs``.  
- Mount the first disk, open it, and run ``Install_SASC_6.50``.  
- Do not touch any of the options, and click "proceed".  
- Ensure it is installing to hard disk and click "proceed".  
- When asked to choose a directory, set the drive to ``system:``, and click "proceed".  
- keep clicking "proceed" until it asks you for the next disk. Mount the disk that it asks you to, and click "proceed".  
- when you see a window reading "the following assigns have been created", click "proceed".  
- Installation Complete!  

4. Install SAS/C 6.55 update  
- Note: this is not necessary to build the game itself. However, if you wish to reassemble the tools (ripdoom/spmus/etc) you will need this.  
- Shut down the emulated Amiga by opening the configuration window (usually it is bound to F12) and clicking "restart".  
- Extract the included ``sc655patch.zip`` somewhere. Mount this as a drive in WinUAE. Note that WinUAE tends to mess with drive names and filepaths when adding or modifying a virtual drive, so ensure all of your virtual drives have the correct names and paths.  
- Start the emulated Amiga again.  
- In workbench, open the drive containing the patch files. Run ``Install_6.55_Patch``.  
- Workbench will ask you to insert disk 1 of SAS/C 6.50, if it is not already mounted. Do so, and click "retry".  
- Do not touch any of the options, and click "proceed". 
- Ensure it is installing to hard disk and click "proceed".  
- When asked to insert the next disk of SAS/C 6.50, do so and click "proceed".  
- When a window appears titled "sc:Read.Me.6.55", press CTRL+C to exit.  
- You will see a window reading "Several header files have been changed". click "yes".  
- Installation Complete! remove the virtual drive containing the 6.55 patch and any SAS/C disk images.  

5. Mount the source directories as virtual drives  
- Download or clone the repo if you haven't already. If you downloaded as a zip, extract it somewhere.  
- Shut down the emulated Amiga by opening the configuration window (usually it is bound to F12) and clicking "restart".  
- Mount the drives as follows:  (do not remove the SYSTEM drive)

| Device | Volume | Path                                      |
|--------|--------|-------------------------------------------|
| HD     | HD     | [path to repo]\HD                         |
| HD2    | HD2    | [path to repo]\HD2                        |
| HD3    | HD3    | [path to repo]\HD3                        |
| ACCESS | ACCESS | [path to repo]\ACCESS                     |

- Start the emulated Amiga again. Save your current configuration if you have not already.

6. Build the game!  
In Workbench:  
go to ``System -> System -> Shell``  
in AmigaShell:  
Change to the source code directory by typing ``hd:source``  
Setup paths: ``execute setuprl`` (this only needs to be done once per session, if you restart WinUAE, you will have to run this again)  
Build the ROM: ``smake rl``  
To build the ROM and write shell output to a log file located in HD/compbi: ``execute buildtolog``

In the HD/compbi directory, there should be a .sfc ROM file present if everything worked successfully.  
