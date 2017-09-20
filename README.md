
Markup : ![picture alt]("https://github.com/gokulvasan/MemoryManagementPhaseBasedStressTest/blob/master/MemoryGobbler.jpg")

Author: Gokul Vasan

 MEMORY GOBBLER
===============

Memory management stress test platform.

RANDOM memory load generator:

RANDOM: this defines what all randomized.
Markup:
	1. selection of amount of mappings in a phase. 
	2. selection between file and anon map.
	3. selection of size.
	4. Touch of a particular map.
	5. length of a holding time.
	6. Amount or how many to touch. 

OVERVIEW:
---------

* Applies natural programme like behaviour, i.e. holding and transtion phases.
* Moreover, unlike other benchmark tool, this uses combination of anon and filemap page.
* Sequence of selection between anon and file is made random.
* but, tries to maintain 3:1 ratio between file and anon respectively.
* Touch is random, not sequential, trying to imitate programm's stochastic behavior.
* Applies quasi-stationary behaviour:
	* i.e. distribution of page reference remains constant for a period of time.
	* Period of time is made stochastic and unpredictable.
* Obeys phase based beaviour, i.e. mostly tries to touch recent allocated pages, but sometimes
  tries touching older phase pages.

USAGE:
------

* User have to use the file_gen script which generates a drive of files called test.img.
* redirect the output to files.h which is used by mem_gobbler.c
* Mount the generated drive @ /mnt/test_images for making this tool work.
* Tool uses location /mnt/test_images to get the files for mapping.

TODO:
-----

* Use Brk and Sbrk 
* Option bases phase max set.
* Option to tune anon and file ratio.
* speed scale for allocation rate
