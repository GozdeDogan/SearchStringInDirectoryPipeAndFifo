all: 
	  clear
	  gcc 131044019_Gozde_Dogan_HW3.c restart.c -o withPipeandFIFO
clean:
	  rm *.o withPipeandFIFO 
