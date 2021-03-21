# Makefile for Simple Multi-thread program
# COP 4610 Project 1
#
#   Team members:
#   - Marcel Riera
#   - Anthony Sanchez-Ayra
#
# Compiler: gcc 4.5

FILES = Multithread.c		# Input files
OUT = Multithread.out 		# Output name

build: $(FILES)			    # Builds target
	gcc $(FILES) -o $(OUT) -lpthread

clean: 				        # Removes core objeccts
	rm -f *.o core

rebuild: clean build		# Removes core then rebuild
