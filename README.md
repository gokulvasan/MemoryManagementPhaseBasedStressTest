 MEMORY GOBBLER
===============

![picture alt](https://github.com/gokulvasan/MemoryManagementPhaseBasedStressTest/blob/master/MemoryGobbler.jpg "Memory gobbler")

Author: Gokul Vasan

Memory management stress test platform.

Need and Philosophy:
--------------------
* Almost all of the memory management benchmarking tool never imitates a real programme behaviour.
* This tool attempts to generate stress test on the memory mangement, but imitating program behaviour.
* Every Program imitates [memory locality](https://en.wikipedia.org/wiki/Locality_of_reference "locality of reference wiki page") at varied [degrees]( https://dl.acm.org/citation.cfm?id=360227 "Characteristics of Program Localities"). This tool does imitate that.
* From the various studies and from the [”Principle of locality”](https://dl.acm.org/citation.cfm?id=1070856) of program’s behaviour from memory access perspective we can infer that:
	* P1. Its the tendency of a programme to cluster the references of pages to small set of the resident pages for extended intervals.
	* P2. There exists a strong Relation between the near future and near past cluster of reference pages, i.e., The set tends to overlap.
	* P3. There exists a feeble or nearly no relation between distant future and distant past references of pages.
	* P4. Pages are accessed in random exhibiting a stochastic behaviour.
	* P5. The cluster references tends to slowly move away from one active set to another, i.e., They exhibit a quasi stationary behaviour, manifesting a quasi time series model.
* When these inferences have to be implemented into a test programme, then the implementation needs to adhere to following properties:
	* Allocation of pages have to be stochastic.
	* Majority of the allocation have to happen during transitions.
	* The program’s tendency to access the pages have to be in LRU approach, but should also stochastically access least recently or least frequently touched pages.
	* Programme should exhibit phase behaviour, i.e., allocation should not happen continuously, but rather there should be a pausation behaviour in allocation request. This pausation should be stochastic.

* Memgobbler imitates **randomness** of a program by adhering to the above rules.

**RANDOM:** The list defines what is randomized.

	1. selection of amount of mappings in a locality. 
	2. selection between file and anon map.
	3. selection of size of mapping.
	4. Touch of a particular map.
	5. length of a holding time.
	6. Amount or how many to touch. 

Overview Of Implementation:
---------------------------
* The tool is built with Autoconf, making the tool auto-build systems like Open Emebedded friendly.
* Generates a sequence of jobs and each job is a locality.
* It is suffice to define Locality with just: [holding(in phase) and transtion state](http://ieeexplore.ieee.org/document/1702696/).
* Tool uses combination of anon and filemap page.
* Sequence of selection between anon and file is made random.
* but, tries to maintain 3:1 ratio between file and anon respectively.
* Touch can be determined to be completly random or could be explicitly specified.
* Applies quasi-stationary behaviour:
	* i.e. distribution of page reference remains constant for a period of time.
	* Period of the constant distribution time is made stochastic and unpredictable.
* Obeys phased beaviour, i.e. mostly tries to touch recent allocated pages, but sometimes
  tries touching older phase pages.
* Runs 2 different mode:
	* Controlled mode : Use args to generate controlled behaviour. or
	* Rogue mode : completly random Runs till SIGKILL due to OOM Reaper.

Usage:
------
**STEP 1: Generating array of list of files for testing**

* User have to use the *file_gen/file_gen.sh* script provided with the repository to generate a drive of files.
* redirect the output of file_gen.sh to files.h which is used by mem_gobbler.c. 
	* Redirection command: *./file_gen/file_gen.sh > files.h*
	* The files.h is auto generated list configured accordance to the memgobbler for stochastic access.
* Mount the generated drive, i.e., file_gen/test.img @ /media/test_images to make the drive of files accessable to the tool.
* P.S. Tool hardcodes the mount location of test.img @ /media/test_images to get the files for mmap-ping.

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
