# CCTS Assignment : Implementing BOCC and FOCC algorithms
=========================================================

Submitted by: Supreet Singh (CS16BTECH11038)

These algorithms have been toaken from the book Transaction Information Systems (by Vossen)

## How to compile

**Note:** This requires C++17 and lpthread.

Simply run the following command in the source directory (executable generated is named ccts):

```bash
$ ./build.sh
```

This will generate an executable of the name **ccts**.

Alternatively, you can do the same using cmake (CMakeLists.txt file has been provided).


## Executing

Simply run:

```bash
$ ./ccts
```

in a directory with the `inp-params.txt` file. 

The input parameters are: N (number of threads), M (number of different data items), numTrans (each thread commits these
number of transactions), constVal (random num generation for writing), λ (random num generation for sleeping).

## Output

The program will create log files for each of the following algorithms:

* BOCC-CTA
* FOCC-CTA
* FOCC-OTA

Also, the average delay to commit (in ms) and the average number of aborts before committing will be output
to stdout. 