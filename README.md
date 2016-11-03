# SMBD_Converter
A tool for converting files between Super Monkey Ball 2 (SMB2) and Super Monkey Ball Deluxe (SMBD) (xbox version).

## Current Support
### Raw LZ
Mostly supported. st201 and st002 are converted perfectly. More testing needs to be done for missing offsets/values.
### TPL
Only converts from SMBD to SMB2. Works for uncompressed textures. Needs refactoring.
### GMA
Only for SMBD to SMB2. Incomplete and needs refactoring

##Usage
### Non-Command Line
Just drag the file to be converted on the executable
### Command Line
Add the file path as the command line parameter. Future support will include passing multiple files at once.