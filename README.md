# applebin
*applebin* is a simple utility for Unix that will convert *AppleSingle* and *AppleDouble*
formatted files into *MacBinary* format.

## Description
Some Macintosh files, such as application programs, contain information in a
resource fork. When transporting these files across non-Macintosh filesystems,
the resource fork data must be encoded somehow, because other Unix and Windows filesystems
do not support resource forks. Back in the 1990s, this encoding was accomplished 
with the *MacBinary* format. In modern times, the newer *AppleSingle* and *AppleDouble*
formats are more commonly used.

## Usage
`applebin` takes one or two filename arguments on the command line. The first is
the name of the `AppleSingle/Double` header file (which probably starts with
`._`), and the second is the name of the data fork file (which is probably the
same as the first file without the `._` prefix).

``` shell
$ applebin ._some-file some-file
# creates some-file.bin
```

## Building
Download and unzip or clone the repository, cd into the applebin folder and run make.

``` shell
$ unzip applebin.zip
$ cd applebin
$ make
```

## Status
### Supported
- File Type and Creator codes
- Resource Forks

### Todo
* Creation and Modification Times
* Finder Info flags
* Locked and Protected flags
* Original Filename

# Technical Miscellany
## AppleDouble Header file

     0   4   magic number x00 05 16 0[07]
     4   4   version x00 02 00 00
     8  16   filler zeros
    24   2   number entries
    26  12   first entry header

### Entry

    0   4   type 1-data 2-rsrc 3-name 9-finfo
    4   4   offset to start
    8   4   length

Type 9 Finder Info (See Inside Macintosh Toolbox Essentials 7-47)

     0  16  ioFlFinderInfo  (starts with type, creator)
    16  16  ioFlXFinderInfo

Type 8 Date/Times
Type 10 Locked/Protected

## MacBinary Header file
MacBinary format 128 byte header

     0  1 zero
     1  1 length of name
     2 63 name of file
    65 4 type
    69 4 creator
    73 1 finder flags (TODO needs reference)
    74 1 zero
    75 2 vpos
    77 2 hpos
    79 2 folder id
    81 1 protected
    82 1 zero
    83 4 data fork length
    87 4 resource fork length
    91 4 creation date
    95 4 last modified
    101 1 finder flags other byte (TODO needs reference)
    122 1 version, starts at 129
    123 1 min-version, starts at 129
    124 4 crc of previous 124 bytes
    128 start of fork

## Resource Fork
See Inside Macintosh: More Mac Toolbox: Resource Manager 1-22

resource fork begins with resource header:

    4 offset to rdata
    4 offset to rmap
    4 length rdata
    4 length rmap

resource map:

    0  16 copy of resource header
    22  2 rfork attributes
    24  2 0x001C (start of map to num types)
    26  2 offset from start of rmap to name list
    28  2 number of types - 1
    30    contains list of resource types (8 bytes each)

resource type list entry:

    4 type
    2 number of this type - 1
    2 offset from beginnig of type list to reference list of this type
