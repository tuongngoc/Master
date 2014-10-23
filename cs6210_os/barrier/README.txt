Try to make. It will build all

##################################################
#-----------Using OpenMP---------------------    #
##################################################
1) A Sense-reversing centralized barrier
-------------------------------------
./gtmp_counter [NUM_THREADS]

2) MCS Barrier 
------------
distributed tree-based barrier with only local spinning.
./gtmp_mcs 

3) combining tree barrier with optimized wakeup
---------------------------------------
./gtmp_tree

##################################################
#-----------Using MPI---------------------       #
##################################################
4) Dissemination Barrier
-----------------------
 mpirun -n 8 ./dissemination_mpi

5) Tournament Barrier
-------------------
mpirun -n 8 ./tournament_mpi

6)A sense-reversing centralized barrier
mpirun -n 8 ./gtmpi_counter


##################################################
#----------- Note from run PLOT----------------- #
##################################################
sudo apt-get install  gnuplot-x11

*I suspect the mpi_plot.gnu & mp_plot.gnu 
files are created in windows but Linux is case sensitive.
So rename the files otherwise you can't plot the gnu files.

$ mv GTMP_Data.csv GTMP_DATA.csv
$ mv GTMPI_Data.csv GTMPI_DATA.csv

*For some weird design choices, gnuplot would just show the graph and quits immediately. You have to supply the -persist flag

$ gnuplot mp_plot.gnu  -persist
$ gnuplot mpi_plot.gnu  -persist

To save the image file, click the top left "copy to clipboard" icon

Open GIMP > File > Create > From Clipboard
File > Save






