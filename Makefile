########################################################################
#                                                                      
#              --- CAEN SpA - Computing Division ---                   
#                                                                      
#   CAENVMElib Software Project                                        
#                                                                      
#   Created  :  March 2004      (Rel. 1.0)                                             
#                                                                      
#   Auth: S. Coluccini                                                 
#                                                                      
########################################################################

EXE	=	caen
EXEWU   =       pku

CC	=	gcc

COPTS	=	-fPIC -DLINUX -Wall 
#COPTS	=	-g -fPIC -DLINUX -Wall 

FLAGS	=	-Wall -s
#FLAGS	=	-Wall

DEPLIBS	=       -l CAENVME -l ncurses -lc -lm

LIBS	=	

INCLUDEDIR =	-I.

OBJS	=	CAENVMEDemoMain.o CAENVMEDemoVme.o console.o

INCLUDES =      console.h  # ../include/CAENVMElib.h ../include/CAENVMEtypes.h ../include/CAENVMEoslib.h

#########################################################################

all	:	$(EXE) $(EXEWU)

clean	:
		/bin/rm -f $(OBJS) $(EXE) $(EXEWU)

$(EXE)	:	$(OBJS)
		/bin/rm -f $(EXE)
		$(CC) $(FLAGS) -o $(EXE) $(OBJS) $(DEPLIBS)

$(EXEWU) :     

$(OBJS)	:	$(INCLUDES) Makefile

%.o	:	%.c
		$(CC) $(COPTS) $(INCLUDEDIR) -c -o $@ $<

