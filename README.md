# Nanxidactyl

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

MainFile: nanxiSim 
Belongs in: cd [path to Nanxidactyl]/source/tools/18742

Note: 
If we want to change name of file, go to makefile.rules in the 18742 directory and change the test tool roots to include the file name. 

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TO BUILD THE CACHE SIMULATOR:  

cd [path to Nanxidactyl]/source/tools/18742
make obj-intel64/nanxiSim.so TARGET=intel64

OR 

make all TARGET=intel64

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TO RUN PIN: 

Example: 
../../../../pin-3.18-98332-gaebd7b1e6-gcc-linux/pin -t obj-intel64/nanxiSim.so -o nanxiSim.log -- /bin/ls

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Other: 
I kept some of the cpp and c files in 18742 because it seems to be a linux thing, so I didn't remove it. 

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


READ HERE: 
compile matrix mult from MatrixMultiply: make
compile pin from source/tools/18742: make obj-intel64/testpin.so TARGET=intel64
run pin from source/tools/18742: ../../../pin_linux -t obj-intel64/testpin.so -- ../../../MatrixMultiply/a.out