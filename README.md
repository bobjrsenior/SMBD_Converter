# SMBD_Converter
A tool for converting files between Super Monkey Ball 2 (SMB2) and Super Monkey Ball Deluxe (SMBD) (xbox version).

## Current Support
### Raw LZ
Mostly supported. st201 and st002 are converted perfectly. More testing needs to be done for missing offsets/values.
### TPL
Experimental conversion from SMB2 to SMBD (gma will need to be implemented before testing due to slight texture differences in equivilent levels).
### GMA
Only for SMBD to SMB2. Incomplete and needs refactoring

##Usage
### Non-Command Line
Just drag the file to be converted on the executable
### Command Line
Add the file path as the command line parameter. Future support will include passing multiple files at once.

## SMB2 Specifications
### Raw LZ
https://craftedcart.github.io/SMBLevelWorkshop/documentation/index.html?page=lzFormat2
### GMA
http://bin.smwcentral.net/u/21732/GMATPL_V2.txt
### TPL
http://bin.smwcentral.net/u/21732/GMATPL_V2.txt

## SMBD Specifications
### Raw LZ
All known specs are in the SMB2 Raw LZ spec file. The only difference is everything is little endian instead of big endian
### GMA
TODO
### TPL
TODO