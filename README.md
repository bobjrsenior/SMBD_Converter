# SMBD_Converter

A tool for converting files between Super Monkey Ball 2 (SMB2) and Super Monkey Ball Deluxe (SMBD) (xbox version).

## Current Support

### Raw LZ

All standard SMB2 stages can be converted, but not all minigame stages. See: https://pastebin.com/VAgP37F5

Deluxe levels still need some work. See: https://pastebin.com/3rwWEt25

### TPL

Experimental conversion from SMB2 to SMBD (gma will need to be implemented before testing due to slight texture differences in equivilent levels).

Don't use this. Use GxModelViewer (GxUtils) instead.

### GMA

Only for SMBD to SMB2. Incomplete and needs refactoring

Don't use this. Use GxModelViewer (GxUtils) instead.

## Usage

### Non-Command Line

Just drag the file to be converted on the executable

### Command Line

Add the file path as the command line parameter. Future support will include passing multiple files at once.

## SMB2 Specifications

### Raw LZ

https://craftedcart.github.io/SMBLevelWorkshop/documentation/index.html?page=lzFormat2

### GMA

https://craftedcart.github.io/SMBLevelWorkshop/documentation/index.html?page=gmaFormat

### TPL

https://craftedcart.github.io/SMBLevelWorkshop/documentation/index.html?page=tplFormat12

## SMBD Specifications

### Raw LZ

All known specs are in the SMB2 Raw LZ spec file. The only difference is everything is little endian instead of big endian

### GMA

All known specs are in the SMB2 GMA spec file. The only difference is everything is little endian instead of big endian

### TPL

https://craftedcart.github.io/SMBLevelWorkshop/documentation/index.html?page=tplFormatDx
