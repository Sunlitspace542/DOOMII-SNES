# Importing Custom Levels

1. Make your level in Doom Builder. Make sure the format is ``Doom: Doom (Doom format)``.  

2. Copy your level to ``02-extra-tools/idbsp`` and start DOSBox. ``idbsp`` is needed to reduce the size of the ``BLOCKMAP`` lump so the level has a better chance of fitting into the ROM.  

3. Run ``buildwad [name of your wad]``. For example, for a wad named ``e1m1.wad``, the command would be ``buildwad e1m1``.  

4. When it asks if you want to overwrite, press Y and press ENTER. After this, you can close DOSBox.  

5. Copy the WAD from the ``idbsp`` directory to ``HD2/DOOM/1.666/WAD`` and rename it to ``doom.wad``.  

6. In AmigaShell, set up the paths if you haven't already (from ``HD:source``, run ``execute setuprl``).  

7. Switch to ``HD2:`` and execute the command ``execute rl:make/ripwad``.  

8. Go to ``HD2/DOOMDATA`` and copy the folder with your level's map number to ``HD2/DOOM/1.666/WAD.DATA/LEVELS``. 

9. Convert the level from Doom format to Reality Engine format by switching to ``rl:binaries`` and running ``execute rl:make/level eX mY`` where X is the episode number of your level, and Y is the map number. If you see any error messages about ``touch``, disregard them.  

10. Switch back to ``HD:source`` and run ``smake rl`` to build the game.  

# Troubleshooting

## "Can't FIT section" error
Try disabling some levels in ``HD/source/rage.i``.  
You can also enable ``useE1M1ONLY`` if you're only replacing the first map and don't care about any others.  

## The emulator freezes when I try to convert a level!
You may be using engine features in your level that ``RIPDOOM`` does not support. Usually this is due to Doom II level features such as fast doors. Replace them and it should work fine.  