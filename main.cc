// main.cc --- 
// 
// Description: 
// Author: Hongyi Wu(吴鸿毅)
// Email: wuhongyi@qq.com 
// Created: 日 7月  2 22:46:18 2017 (+0800)
// Last-Updated: 二 7月  4 14:32:36 2017 (+0800)
//           By: Hongyi Wu(吴鸿毅)
//     Update #: 15
// URL: http://wuhongyi.cn 

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <cinttypes>
#include <inttypes.h>
#include <cstdint>
#include <stdint.h>
#include <math.h>
#include <ctype.h>
#include <curses.h>
#include "CAENVMElib.h"
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

typedef struct man_par
{
  uint32_t	addr ;		// Address
  uint32_t	data ;          // Data
  ushort		level ;		// Interrupt level
  unsigned char		irqstat;	// IRQ status
  ushort		am ;		// Addressing Mode
  CVDataWidth dtsize ;		// Data Format
  uint32_t	basaddr ;	// Base Address
  uint32_t	blts ;		// Block size for blt (bytes)
  ushort		ncyc ;		// Number of cycles
  ushort		autoinc ;	// Auto increment address
  uint32_t	*buff ;         // Mmemory buffer for blt
} man_par_t ;


/******************************************************************************/
/*                               CON_KBHIT                                    */
/*----------------------------------------------------------------------------*/
/* return:        0  ->  no key pressed                                       */
/*                c  ->  ascii code of pressed key                            */
/*----------------------------------------------------------------------------*/
/* Read the standard input buffer; if it is empty, return 0 else read and     */
/* return the ascii code of the first char in the buffer. A call to this      */
/* function doesn't stop the program if the input buffer is empty.            */
/******************************************************************************/
char con_kbhit()
{
  int i, g;  
  
  nodelay(stdscr, TRUE);
  i = ( ( g = getch() ) == ERR ? 0 : g );
  nodelay(stdscr, FALSE);

  return i;   
}

/******************************************************************************/
/*                               CON_GETCH                                    */
/*----------------------------------------------------------------------------*/
/* return:        c  ->  ascii code of pressed key                            */
/*----------------------------------------------------------------------------*/
/* Get a char from the console without echoing                                */
/* return the character read                                                  */
/******************************************************************************/
int  con_getch(void)
{
  int i;
  
  while( ( i = getch() ) == ERR );
  return i; 
}









/*	
	-----------------------------------------------------------------------------

	Name		:	CaenVmeIrqCheck
	Description :	Interrupt Check 

	Input		:	BHandle		->	Board Id (V1718,V2718)
	man			->	pointer to 'man' structure
	Output		:	None
	Release		:	1.0

	-----------------------------------------------------------------------------
*/

void CaenVmeIrqCheck(int32_t BHandle, man_par_t *man)
{
  CVErrorCodes	ret ;

  printf(" CHECK IRQ\n");

  CAENVME_IRQCheck(BHandle,&man->irqstat);		// Check IRQ Status

  printf(" IRQ status: %02X\n",man->irqstat);
  printf(" Interrupt Level to Acknowledge (0 to exit) : ");  // Get the int level   
  scanf("%x",&man->level); 

  if (man->level == 0)
    {
      return ;
    }

  printf(" Waiting for interrupt ... Press any key to stop.\n");

  while (!(man->irqstat & (1<<(man->level-1))) && !con_kbhit())   // Check Int loop
    CAENVME_IRQCheck(BHandle,&man->irqstat);


  if(man->irqstat & (1<<(man->level-1)))
    {
      CVIRQLevels level;
      switch (man->level) {
      case 1:
	level = cvIRQ1;
	break;
      case 2:
	level = cvIRQ2;
	break;
      case 3:
	level = cvIRQ3;
	break;
      case 4:
	level = cvIRQ4;
	break;
      case 5:
	level = cvIRQ5;
	break;
      case 6:
	level = cvIRQ6;
	break;
      case 7:
	level = cvIRQ7;
	break;
      default:
	level = cvIRQ1;
	break;
      }
      ret = CAENVME_IACKCycle(BHandle,level,&man->data,man->dtsize);
    
      switch (ret)
	{
	case cvSuccess   : printf(" Cycle completed normally\n");							    
	  printf(" Int. Ack. on IRQ%d: StatusID = %0X",man->level,man->data);
	  break ;
	case cvBusError	 : printf(" Bus Error !!!");
	  break ;				   
	case cvCommError : printf(" Communication Error !!!");
	  break ;
	default          : printf(" Unknown Error !!!");
	  break ;
        }
    }
  else
    printf(" Interrupt request IRQ%d not active",man->level);   
}



/*
  -----------------------------------------------------------------------------

  Name		:	CaenVmeWriteBlt
  Description :	Vme Block Transfer Write  

  Input		:	BHandle		->	Board Id (V1718,V2718)
  man			->	pointer to 'man' structure
  Output		:	None
  Release		:	1.0

  -----------------------------------------------------------------------------
*/

void CaenVmeWriteBlt(int32_t BHandle, man_par_t *man)
{
  int  nb;
  uint32_t      i,incr ;
  CVAddressModifier	am;
  CVErrorCodes  ret,old_ret=cvSuccess;

  if(man->am == cvA16_U)
    {
      printf(" Can't execute a A16 BLT Write Cycle");
      return ;
    }
  if(man->am == cvCR_CSR)
    {
      printf(" Can't execute a CR/CSR BLT Write Cycle");
      return ;
    }

  printf(" First Data [hex] : ") ;
  scanf("%x",&man->data);					// Get data to write 

  printf(" Data Increment [hex] : ") ; // Get increment for data
  scanf("%x",&incr);  
  for(i=0; i<(man->blts/4); i++)				// Fill the data buffer
    man->buff[i] = man->data+incr*i;

  if (man->dtsize == cvD64)
    {
      if (man->am == cvA24_U_DATA)
	am = cvA24_U_MBLT;
      else 
	am = cvA32_U_MBLT;
    }
  else
    {
      if (man->am == cvA24_U_DATA)
	am = cvA24_U_BLT;
      else 
	am = cvA32_U_BLT;
    }

  if(man->ncyc == 0)				// Infinite Loop
    printf(" Running ...    Press any key to stop.");

  for (i=0; ((man->ncyc==0) || (i<man->ncyc)) && !con_kbhit(); i++)
    {
      ret = CAENVME_BLTWriteCycle(BHandle,man->addr,(char *)man->buff,man->blts,am,man->dtsize,&nb);
        		
      if((i==0) || (ret != old_ret))
	{

	  switch (ret)
	    {
	    case cvSuccess   : printf(" Cycle(s) completed normally\n");
	      printf(" Written %u bytes",nb);
	      break ;
	    case cvBusError	 : printf(" Bus Error !!!\n");
	      printf(" Written %u bytes",nb);
	      break ;				   
	    case cvCommError : printf(" Communication Error !!!");
	      break ;
	    default          : printf(" Unknown Error !!!");
	      break ;
	    }
	}
       
      old_ret = ret;
    } 
}


/*
  -----------------------------------------------------------------------------

  Name		:	CaenVmeReadBlt
  Description :	Vme Block Transfer Read  

  Input		:	BHandle		->	Board Id (V1718,V2718)
  man			->	pointer to 'man' structure
  Output		:	None
  Release		:	1.0

  -----------------------------------------------------------------------------
*/

void CaenVmeReadBlt(int32_t BHandle, man_par_t *man)
{
  int					nb;
  uint32_t				i,j ;
  CVAddressModifier	am;
  CVErrorCodes		ret,old_ret=cvSuccess;


  if(man->am == cvA16_U)
    {
      printf(" Can't execute a A16 BLT Read Cycle");
      return ;
    }
  if(man->am == cvCR_CSR)
    {
      printf(" Can't execute a CR/CSR BLT Read Cycle");
      return ;
    }

  if (man->dtsize == cvD64)
    {
      if (man->am == cvA24_U_DATA)
	am = cvA24_U_MBLT;
      else 
	am = cvA32_U_MBLT;
    }
  else
    {
      if (man->am == cvA24_U_DATA)
	am = cvA24_U_BLT;
      else 
	am = cvA32_U_BLT;
    }

  if(man->ncyc == 0)				// Infinite Loop
    printf(" Running ...    Press any key to stop.");

  for (i=0; ((man->ncyc==0) || (i<man->ncyc)) && !con_kbhit(); i++)
    {
      for (j=0;j<(man->blts/4);j++)
	man->buff[j]=0;

      ret = CAENVME_BLTReadCycle(BHandle,man->addr,(char *)man->buff,man->blts,am,man->dtsize,&nb);
        
      if((i==0) || (ret != old_ret))
	{
	  switch (ret)
	    {
	    case cvSuccess   : printf(" Cycle(s) completed normally\n");
	      printf(" Read %u bytes",nb);
	      break ;
	    case cvBusError	 : printf(" Bus Error !!!\n");
	      printf(" Read %u bytes",nb);
	      break ;				   
	    case cvCommError : printf(" Communication Error !!!");
	      break ;
	    default          : printf(" Unknown Error !!!");
	      break ;
	    }
	}
      old_ret = ret;
    }      
}


/*
  -----------------------------------------------------------------------------

  Name		:	CaenVmeWrite
  Description :	Vme Write access  

  Input		:	BHandle		->	Board Id (V1718,V2718)
  man			->	pointer to 'man' structure
  Output		:	None
  Release		:	1.0

  -----------------------------------------------------------------------------
*/

void CaenVmeWrite(int32_t BHandle, man_par_t *man)
{
  uint32_t		i ;
  CVErrorCodes	ret,old_ret=cvSuccess;

  if(man->dtsize == cvD64)
    {
      printf(" Can't execute a D64 Write Cycle");
      return ;
    }

  printf(" Write Data [hex] : ") ;
  scanf("%x",&man->data) ;

  if(man->ncyc == 0)				// Infinite Loop
    printf(" Running ...    Press any key to stop.");

  for (i=0; ((man->ncyc==0) || (i<man->ncyc)) && !con_kbhit(); i++)
    {
      ret = CAENVME_WriteCycle(BHandle,man->addr,&man->data,(CVAddressModifier)man->am,man->dtsize);

      if((i==0) || (ret != old_ret))
	{
	  switch (ret)
	    {
	    case cvSuccess   : printf(" Cycle(s) completed normally\n");
	      break ;
	    case cvBusError	 : printf(" Bus Error !!!");
	      break ;				   
	    case cvCommError : printf(" Communication Error !!!");
	      break ;
	    default          : printf(" Unknown Error !!!");
	      break ;
	    }
	}

      old_ret = ret;

      if(man->autoinc)
	{
	  man->addr += man->dtsize;		// Increment address (+1 or +2 or +4) 	
	  printf("%08X]",man->addr);	// Update the screen
	}
    }
}



/*
  -----------------------------------------------------------------------------

  Name		:	CaenVmeRead
  Description :	Vme Read access  

  Input		:	BHandle		->	Board Id (V1718,V2718)
  man			->	pointer to 'man' structure
  Output		:	None
  Release		:	1.0

  -----------------------------------------------------------------------------
*/

void CaenVmeRead(int32_t BHandle, man_par_t *man)
{
  uint32_t		i,old_data=0 ;
  CVErrorCodes	ret,old_ret=cvSuccess;

  if(man->dtsize == cvD64)
    {
      printf(" Can't execute a D64 Read Cycle");
      return ;
    }

  if(man->ncyc == 0)				// Infinite Loop
    printf(" Running ...    Press any key to stop.");

  for (i=0; ((man->ncyc==0) || (i<man->ncyc)) && !con_kbhit(); i++)
    {
      ret = CAENVME_ReadCycle(BHandle,man->addr,&man->data,(CVAddressModifier)man->am,man->dtsize);

      if((i==0) || (ret != old_ret))
	{
	  switch (ret)
	    {
	    case cvSuccess   : printf(" Cycle(s) completed normally\n");
	      if((i==0) || (old_data != man->data))  
		{                    
		  if( man->dtsize == cvD32 )
		    printf(" Data Read : %08X",man->data);
		  if( man->dtsize == cvD16 )
		    printf(" Data Read : %04X",man->data & 0xffff);
		  if( man->dtsize == cvD8 )
		    printf(" Data Read : %02X",man->data & 0xff);
		}
	      break ;
	    case cvBusError	 : printf(" Bus Error !!!");
	      break ;				   
	    case cvCommError : printf(" Communication Error !!!");
	      break ;
	    default          : printf(" Unknown Error !!!");
	      break ;
	    }
	}

      old_data = man->data;
      old_ret = ret;

      if(man->autoinc)
	{
	  man->addr += man->dtsize;                       // Increment address (+1 or +2 or +4) 	
	  printf("%08X]",man->addr);	// Update the screen
	}
    }
    
}



/*
  -----------------------------------------------------------------------------

  Name		:	ViewReadBltData
  Description :	Display the content of 'man->buff' buffer  

  Input		:	man	 ->	pointer to 'man' structure
  Output		:	None
  Release		:	1.0

  -----------------------------------------------------------------------------
*/

void ViewReadBltData(man_par_t *man)
{
  ushort		i,j,line, page=0, gotow, dtsize ;
  uint32_t	ndata;
  uint32_t	*d32;
  ushort		*d16;
  unsigned char		*d8;
  char		key = 0;
  char		msg[80];
  FILE		*fsave;

  d32 = man->buff ;
  d16 = (ushort *)man->buff ;
  d8  = (unsigned char *)man->buff ;

  dtsize = man->dtsize ;

  while( key != 'Q' )				// Loop. Exit if 'Q' is pressed  
    {
      ndata = man->blts / dtsize;

      printf("\n VIEW BUFFER\n\n") ; 
      printf(" Num.    Addr     Hex         Dec \n\n");
    
      // Write a page 
      for( line=0, i=page * 16; (line<16) && (i<ndata); line++, i++)
	{
	  if( dtsize == cvD32 || dtsize == cvD64)             
	    printf(" %05u   %04X     %08X    %-10d \n",i,i*4,d32[i],d32[i]);
                          
	  if( dtsize == cvD16) 
	    printf(" %05u   %04X     %04X        %-6d \n",i,i*2,d16[i],d16[i]);

	  if( dtsize == cvD8)                             
            printf(" %05u   %04X     %02X          %-4d \n",i,i,d8[i],d8[i]);                   
        }
	
      // Print the line menu
      printf("\n[Q] Quit  [D] Data_size  [S] Save  [G] Goto");
      if( page != 0 )
        printf("  [P] Previous");
      if( i != ndata )
        printf("  [N] Next");

      key=toupper(con_getch()) ;              // Wait for command

      switch (key)
	{
	case 'N' :	if(i<ndata)             // Next page 
	    page++;
	  break ;
	case 'P' :	if(page>0)              // Previous page 
	    page--;
	  break ;     
	case 'D' :	dtsize = dtsize * 2;    // Change data size (8,16,32) 
	  if(dtsize > 4) 
	    dtsize = 1;
	  page = 0;
	  break ;
	case 'G' :      printf("Insert data number (dec) : ") ; // Go to data 
	  scanf("%d",(ushort *)&gotow) ;

	  if(gotow>ndata)
	    {
	      printf(" Invalid data number ");
	    }
	  else
	    page=gotow/16;
	  break ;
     
	case 'S' :      
	  printf(" File Name : ") ; // Save buffer to file 
	  if (scanf("%s",msg) == EOF)
	    break ;
      
	  if((fsave=fopen(msg,"w")) == NULL)
	    {
	      printf(" Can't open file ");
	    }
	  else
	    {
	      for(j=0;j<ndata;j++)
		{
		  if( dtsize == cvD32 || dtsize == cvD64)  
		    fprintf(fsave,"%05u\t%08X\t%-10d\n",j,d32[j],d32[j]);
		  if( dtsize == cvD16) 
		    fprintf(fsave,"%05u\t%04X\t%-6d\n",j,d16[j],d16[j]);
		  if( dtsize == cvD8)  
		    fprintf(fsave,"%05u\t%02X\t%-4d\n",j,d8[j],d8[j]);
		}

	      fclose(fsave);
	    }
	  break ;
	default	 :  break ;
   
	}
    } 
}




/*	
	-----------------------------------------------------------------------------

	Name		:	CaenVmeManual
	Description :	V1718 & v2718 manual controller  

	Input		:	BHandle		->	Board Id (V1718,V2718)
	first_call	->	flag for default settings
	Output		:	None
	Release		:	1.0

	-----------------------------------------------------------------------------
*/

void CaenVmeManual(int32_t BHandle, short first_call)
{	
  static  man_par_t	man;
  char  key,dis_main_menu;
  uint32_t      value;

  if (first_call)
    {								// Default Value 
      man.addr = 0xEE000000 ;      
      man.level = 1 ;           
      man.am = cvA32_U_DATA ;            
      man.dtsize = cvD16 ;        
      man.basaddr = 0xEE000000 ; 
      man.blts = 256 ;          
      man.ncyc = 1 ;            
      man.autoinc = 0 ;  
	
      // Allocate 32K for the software buffer containing data for blt 
      man.buff = (uint32_t *)malloc(16*1024*1024);
      if (man.buff == NULL)
	{
	  printf(" !!! Error. Can't allocate memory for BLT buffer. ");
	  exit(-1);
	}
    }

  for (;;)
    {
      dis_main_menu = 0 ;

      printf("\n     CAEN VME Manual Controller \n\n") ;

      printf(" R - READ\n") ;
      printf(" W - WRITE\n") ;
      printf(" B - BLOCK TRANSFER READ\n") ;
      printf(" T - BLOCK TRANSFER WRITE\n") ;
      printf(" I - CHECK INTERRUPT\n") ;
      printf(" 1 - ADDRESS                  [%08"PRIX32"]\n",man.addr) ;
      printf(" 2 - BASE ADDRESS             [%08"PRIX32"]\n",man.basaddr) ;
      printf(" 3 - DATA FORMAT              [") ; 
      if (man.dtsize == cvD8)
        printf("D8]\n") ; 
      if (man.dtsize == cvD16)
        printf("D16]\n") ; 
      if (man.dtsize == cvD32)
        printf("D32]\n") ; 
      if (man.dtsize == cvD64)
        printf("D64]\n") ; 
           
      printf(" 4 - ADDRESSING MODE          [") ;  
      if (man.am == cvA16_U)
        printf("A16]\n") ; 
      if (man.am == cvA24_U_DATA)
        printf("A124]\n") ; 
      if (man.am == cvA32_U_DATA)
        printf("A32]\n") ; 
      if (man.am == cvCR_CSR)
        printf("CR/CSR]\n") ; 
     
      printf(" 5 - BLOCK TRANSFER SIZE      [%"PRIu32"]\n",man.blts) ;
      printf(" 6 - AUTO INCREMENT ADDRESS   [") ; 
      if (man.autoinc)
        printf("ON] \n") ;
      else
        printf("OFF]\n") ;

      printf(" 7 - NUMBER OF CYCLES         [") ;
      if (man.ncyc)
        printf("%d]\n",man.ncyc) ; 
      else
        printf("Infinite\n") ;
		   		         
      printf(" 8 - VIEW BLT DATA\n") ;          
      printf(" F - FRONT PANEL I/O\n") ; 
      printf(" Q - QUIT MANUAL CONTROLLER\n") ; 

      do
	{
	  key = toupper(con_getch()) ;

	  switch (key)  
	    {
	    case  'R' :	CaenVmeRead(BHandle, &man) ;
	      break ;	

	    case  'W' : CaenVmeWrite(BHandle, &man) ; 
	      break ;

	    case  'B' : CaenVmeReadBlt(BHandle, &man) ; 
	      break ;

	    case  'T' : CaenVmeWriteBlt(BHandle, &man) ;
	      break ;

	    case  'I' : CaenVmeIrqCheck(BHandle, &man) ;
	      break ;

	    case  '1' : printf(" Please enter new Address : ") ;

	      if (scanf("%"SCNx32,&value) != EOF)
		man.addr = man.basaddr + value ;

	      printf("%08"PRIX32,man.addr) ;

	      break ;

	    case  '2' : printf(" Please enter new Base Address : ") ;

	      if (scanf("%"SCNx32,&value) != EOF)
		{
		  man.addr -= man.basaddr ;   
		  man.basaddr = value ;
		  man.addr += man.basaddr ;
		}

	      printf("%08"PRIX32,man.basaddr) ;
	      printf("%08"PRIX32,man.addr) ;
	      break ;

	    case  '3' : 

	      switch(man.dtsize)
		{
		case cvD8  : man.dtsize = cvD16 ;
		  printf("D16]\n") ;
		  break ;	
		case cvD16 : man.dtsize = cvD32 ;
		  printf("D32]\n") ;
		  break ;	
		case cvD32 : man.dtsize = cvD64 ;
		  printf("D64]\n") ;
		  break ;	
		case cvD64 : man.dtsize = cvD8 ;
		  printf("D8] \n") ;
		  break ;	

		case cvD16_swapped :
		case cvD32_swapped :
		case cvD64_swapped :
		default :
		  break;
		}

	      break ;

	    case  '4' : 
	      switch(man.am)
		{
		case cvA16_U	  : man.am = cvA24_U_DATA ;
		  printf("A24]\n") ;
		  break ;	
		case cvA24_U_DATA : man.am = cvA32_U_DATA ;
		  printf("A32]\n") ;
		  break ;	
		case cvA32_U_DATA : man.am = cvCR_CSR ;
		  printf("CR/SCR]\n") ;
		  break ;	
		case cvCR_CSR	  : man.am = cvA16_U ;
		  printf("A16]   \n") ;
		  break ;	
		}

	      break ;

	    case  '5' : printf(" Please enter Block Transfer Size : ") ;
	      if (scanf("%"SCNu32,&value) != EOF)
		man.blts = value ;
	      printf("%"PRIu32"]        \n",man.blts) ;	
	      break ;

	    case  '6' : 

	      man.autoinc ^= 1 ;
	      if (man.autoinc)
	        printf("ON] \n") ;
	      else
	        printf("OFF]\n") ;
	      break ;

	    case  '7' : printf(" Please enter Number of Cycles : ") ;
					
	      if (scanf("%"SCNu32,&value) != EOF)
		man.ncyc = (ushort)value ;
	      if (man.ncyc)
	        printf("%"PRIu32"]       \n",man.ncyc) ;
	      else
	        printf("Infinite]\n",man.ncyc) ;
	      break ;

	    case  '8' :	ViewReadBltData(&man);
	      dis_main_menu = 1;	// Display Main Menu
	      break ;

	    case  'Q' : free(man.buff);		// Release the memory buffer for BLT 
	      return ;			
	    default   : break ;
	    }
	}
      while (!dis_main_menu) ;
    }										// End 'for(;;)' 
}



//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

int main(int argc, char *argv[])
{
  CVBoardTypes  VMEBoard;
  short         Link;
  short         Device;
  int32_t       BHandle;

  Device = 0;
  Link = 0;
  VMEBoard = cvV2718;

  // Initialize the Board
  if( CAENVME_Init(VMEBoard, Link, Device, &BHandle) != cvSuccess ) 
    {
      printf("\n\n Error opening the device\n");
      exit(1);
    }

  CaenVmeManual(BHandle,1) ;
  

  CAENVME_End(BHandle);
  
  return 0;
}

// 
// main.cc ends here
