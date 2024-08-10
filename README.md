# This works only on Proton 9.0 and higher
This is a small server plugin which hooks some function in lsteamclient.dll and makes InitiateGameConnection work. Please note that this is hacky as hell and might break in the future.

## How to install/use
1. You need a CMake compatible compiler C such as Visual Studio Build Tools
2. Just run the bat script.
3. Copy the resulting .dll file to one of the following
- SDK 2006
  - .../steamapps/common/Source SDK Base/bin/
- SDK 2007
  - The directory that contains gameinfo.txt
4. If you want the plugin to load automatically, create a vdf file with any name in <your_game_directory>/addons/ structured as follows
```
"Plugin"
{
        "file"  "authplugin"
}
```
