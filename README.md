 MEMORY GOBBLER
===============

![picture alt](https://github.com/gokulvasan/MemoryManagementPhaseBasedStressTest/blob/master/MemoryGobbler.jpg "Memory gobbler")

Author: Gokul Vasan

Memory management stress test platform.

RANDOM memory load generator:

**RANDOM:** The list defines what is randomized.

	1. selection of amount of mappings in a phase. 
	2. selection between file and anon map.
	3. selection of size of mapping.
	4. Touch of a particular map.
	5. length of a holding time.
	6. Amount or how many to touch. 

OVERVIEW:
---------
* The tool is built with Autoconf, enabling auto-build systems like Open Emebedded portable.
* Applies natural programme like behaviour, i.e. holding and transtion phases.
* Moreover, unlike other benchmark tool, this uses combination of anon and filemap page.
* Sequence of selection between anon and file is made random.
* but, tries to maintain 3:1 ratio between file and anon respectively.
* Touch is random, not sequential, trying to imitate programm's stochastic behavior.
* Applies quasi-stationary behaviour:
	* i.e. distribution of page reference remains constant for a period of time.
	* Period of the constant distribution time is made stochastic and unpredictable.
* Obeys phase based beaviour, i.e. mostly tries to touch recent allocated pages, but sometimes
  tries touching older phase pages.
* Runs till SIGKILL, due to OOM Reaper or SIGSEGV, due to out of swap space.

USAGE:
------
**STEP 1: Generating array of list of files for testing**

* User have to use the *file_gen/file_gen.sh* script provided with the repository to generate a drive of files.
* redirect the output of file_gen.sh to files.h which is used by mem_gobbler.c. 
	* Redirection command: *./file_gen/file_gen.sh > files.h*
	* The files.h is auto generated list configured accordance to the memgobbler for stochastic access.
* Mount the generated drive, i.e., file_gen/test.img @ /media/test_images to make the drive of files accessable to the tool.
* P.S. Tool uses location /media/test_images to get the files for mapping.

**STEP 2: Building the tool**
* Once the files.h is generated in the build directory, now we can build the memory gobbler. 
* In the shell run the following in sequence:
	*  ./autogen.sh
	* ./configure
 	*  make
* This will produce binary *memgobble* that is ready for use.

**STEP 3: Running the tool**
* ***memgobble***
* ***memgobble [options]***
* ***List of options:***
* -h                   : Display usage
* -v                   : Switch verbose off
* -p [locality count]  : Maximum locality the system will run
* -l [upper limit]     : Set the upper bound of alloc request
* -e [Execution time]  : Execution time of each locality in Ms
* -M [pages]           : Maximum allocation in pages per phase
* -s [speed]           : Speed at which memory is gobbled
	* speed: 0-n, Higher the value faster the memory exhaustion
* -t [access-type]     : Access type of the locality
	* access-type:
		* 0: Fix memory access pattern
		* 1: Stride memory access pattern
		* 2: Sequential memory access pattern
		* 3: Repeat memory access pattern
		* 4: Random memory access pattern
* -V [File name]       : Vector of localities

TODO:
-----
* Use Brk and Sbrk 
* Option to tune anon and file ratio.

**WARNING:** Memory stress tool that could slow down the system.
