# Error Correction

## Description

This repository contains a multi-process, multi-threaded program that simulates the corruption and recovery of files.

The recovery is done with a parity check. 

## How to use it

1. Clone this repository
2. Run `make` in the downloaded directory.
3. Run `./error_correction.out -f <path_to_file> -s <server_to_delete>`
    * `<path_to_file>` can be the path to any file with reading permission.
        * Using an example file: `Examples/example.txt`
    * `<server_to_delete>` can be any number between 0 and 2, indicating the server that will delete 
    part of the file; the part that the program will try to recover. By default, this
    has a value of `0`.
4. Inspect the newly created files: 
    * `<file_name>_broken.<file_ext>`: This file will contain the resulting file after 
    the indicated part was deleted but before it was attempted to recover. 
        * Using the example above: `Examples/example_broken.txt`
    * `<file_name>_recovered.<file_ext>`: This file will contain the resulting file after
    recovering the part that was deleted.
        * Using the example above: `Examples/example_recovered.txt`
    * You will see one of the two messages when the program finishes running: 
        * `File successfully recovered... They are the same!`
        * `Hmmm, this wasn't recovered correctly... Tough one!`

## How it works

The uploaded file is separated, bit by bit, into the number of servers specified. Keep in mind that in this implementation, 
we can only recover the file if only __one__ of the servers' information is lost.
Also, the servers are simulated by different processes, communicating with sockets.


We then create a parity file, which will help us recover the whole file if part of it is missing. After creating the parity file, 
the user will be asked to input the server which its information will be lost.

After that server looses its bits, the file will be put together again. The server that its information is lost will send 
only `0`s as their bits. A file with this information will be written with the text `_broken` added before the extension 
as its name.

Then, the recovery process will start, which checks for `0`s in the lost file that should be `1`s. After this is finished, 
the recovered file will be written with the text `_recovered` added before the extension as its name. The files are also 
compared in the program to see if the recovery was successful.



## Why is this useful?

Imagine that we separate a `3MB` file into three different servers. Normally, you would need to store a copy of the file in 
another server to make sure you have access to the file in case the server where it is stored is lost. 

If this is the case, then you would need to store the entire `3MB` file in another server, adding up to a total of 
`3MB + 3MB = 6 MB`. Now, if one of those servers is lost, then you could still recover the file.

In this implementation, the algorithm used needs a total storage of:

`Storage = f + 2*(f/s)`

Where `f`: File size

`s`: amount of servers

So lets say that we are using 3 servers in this implementation. Then we would need to store `1MB` in each server plus 
an additional `1MB` in 2 of the three servers. This additional information is the parity file. Since we are only ensuring 
the data is recovered if one of the servers is lost, if two servers contain this file then we will always have access to it.

So, if 3 servers are used, the amount of memory needed is 

`Storage = 3 + 2*(3/3) = 5MB`

Using this algorithm provides redundancy for a file using `17%` less storage than the 'raw' approach for 3 servers.

If we increase the amount of servers, then the parity file becomes smaller, making the storage difference even larger.

## Sources

What is a Parity Bit? (2017, October 17). Retrieved April 20, 2019, from https://www.computerhope.com/jargon/p/paritybi.htm

Barr, M. (2018, November 14). CRC Series, Part 2: CRC Mathematics and Theory. Retrieved April 20, 2019, from https://barrgroup.com/Embedded-Systems/How-To/CRC-Math-Theory

Initial server creation based from code authored by Gilberto Echeverria - ITESM CSF.

## Author

Axel Zuchovicki - ITESM CSF