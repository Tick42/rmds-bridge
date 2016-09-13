ECHO OFF
rem simple demo script for windows that uses the default mama.properties file
rem
rem Set arguments %1 to the source name e.g. IDN and %2 to the symbol name, e.g. VOD.L
rem
rem First set the WOMBAT_PATH environment variable to the current directory - this is where mama looks for a mama.properties file by defualt.
set WOMBAT_PATH=%cd%
rem
rem Then pass the source and symbol names as command line arguments to mamalistenc
mamalistenc.exe -B -S %1   -s %2  -tport rmds_sub -m tick42rmds -threads 1 