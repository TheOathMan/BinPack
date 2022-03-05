# BinPack
BinPack is a command line tool for C and C++ to statically pack binary data into a header file, a library file or to output the binary content of a file to the console.

# Usage:
List files that need to be packed as data array in a single header file or as a static libray. Use minus sign (-) to list options, and equal sign (=) for edit options. Available options are:

### Write Options:

* `-hr` To pack all the data of provided files into a header file (.h). [default]

* `-l64` To pack all the data of provided files into a 64 bit static library.

* `-l32` To pack all the data of provided files into a 32 bit static library.

* `-p` Output data to the console.

* `-pn` Output data to the console natively. (strip c++ integer-suffix and comma)

* `-pc` Output data to the console as Ascll characters.


### Mode Options:

* `-hx` Convert data to hexadecimal literals.

* `-bn` Convert data to binary literals.

* `-j` To align the data or justify all lines.

* `-c` To compress the data. (see source code for more details on how to decompress it

### Edit Options:

* `=out` write the output to a folder. Example: =out C:\Users\Desktop

* `=jl` Edit the justify level end line level. Example: =jl 2

* `=cmf` compress a file.

* `=ucf` decompress a file.


# Examples:

### Example 1:
```
text.txt -j -hx -p
```

This command will output the data of 'text.txt' into the console(-p) in the form of hexadecimal(-hx),
and justify(-j) will be applied to the output.

### Example 2:
```
text.txt data.bin -l64
```

This command will pack the binary data of 'text.txt' and 'data.bin' into a static library file (.lib/.a) which then will be outputted along with a header file that contain pointers to each data within that precompiled binary pachage.

# License
[Unlicense](https://unlicense.org/) public domian. 
I dedicate any and all copyright interest in this software to the
public domain. I make this dedication for the benefit of the public at
large and to the detriment of my heirs and successors. I intend this
dedication to be an overt act of relinquishment in perpetuity of all
present and future rights to this software under copyright law.