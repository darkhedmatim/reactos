#include <windows.h>
#include <winnt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "any_op.h"

/* hack should be in misc.h*/


CPU_INT ConvertProcess(FILE *outfp, CPU_INT FromCpuid, CPU_INT ToCpuid)
{
    CPU_INT ret=0;
   CPU_INT eax =-1;
   CPU_INT ebp =-1;
   CPU_INT edx =-1;
   CPU_INT esp =-1;
   CPU_INT regbits=-1;
   CPU_INT HowManyRegInUse = 0;
   CPU_INT RegTableCount[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
   CPU_INT t;
   PMYBrainAnalys pMystart = pStartMyBrainAnalys;
   PMYBrainAnalys pMyend = pMyBrainAnalys;

   PMYBrainAnalys ptmpMystart = pStartMyBrainAnalys;
   PMYBrainAnalys ptmpMyend = pMyBrainAnalys;

   if ( (FromCpuid == IMAGE_FILE_MACHINE_POWERPC) ||
        (FromCpuid == IMAGE_FILE_MACHINE_I386))
    {
        regbits = 32 / 8;
        esp = 1;
        eax = 3;
        edx = 4;
        ebp = 31;
    }

    /* FIXME calc where todo first split */

   /* count how many register we got */
    ptmpMystart = pMystart;
    ptmpMyend = pMyend;
    while (ptmpMystart!=NULL)
    {
        if ((ptmpMystart->type & 2) == 2)
            RegTableCount[ptmpMystart->src]++;

        if ((ptmpMystart->type & 8) == 8)
            RegTableCount[ptmpMystart->dst]++;

        if (ptmpMystart == ptmpMyend)
            ptmpMystart=NULL;
        else
            ptmpMystart = (PMYBrainAnalys) ptmpMystart->ptr_next;
    }

    for (t=0;t<31;t++)
    {
        if (RegTableCount[t]!=0)
        {
            HowManyRegInUse++;
        }
        
    }

    /* switch to the acual converting now */
    switch (ToCpuid)
    {
        case IMAGE_FILE_MACHINE_I386:
             ret = ConvertToIA32Process( outfp, eax, ebp,
                                edx, esp, 
                               pMystart, 
                               pMyend, regbits,
                               HowManyRegInUse);
             if (ret !=0)
             {
                 printf("should not happen contact a devloper, x86 fail\n");
                 return -1;
             }
             break;

        case IMAGE_FILE_MACHINE_POWERPC:
             ret = ConvertToPPCProcess( outfp, eax, ebp,
                                edx, esp, 
                               pMystart, 
                               pMyend, regbits,
                               HowManyRegInUse);
             if (ret !=0)
             {
                 printf("should not happen contact a devloper, x86 fail\n");
                 return -1;
             }
             break;

        default:
            printf("should not happen contact a devloper, unknown fail\n");
            return -1;
    }

    return ret;
}
