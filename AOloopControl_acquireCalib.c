/**
 * @file    AOloopControl_acquireCalib.c
 * @brief   Adaptive Optics Control loop engine acquire calibration
 * 
 * AO engine uses stream data structure
 *  
 * @author  O. Guyon
 * @date    25 Aug 2017
 *
 * 
 * @bug No known bugs.
 * 
 * 
 */



#define _GNU_SOURCE

// uncomment for test print statements to stdout
//#define _PRINT_TEST



/* =============================================================================================== */
/* =============================================================================================== */
/*                                        HEADER FILES                                             */
/* =============================================================================================== */
/* =============================================================================================== */

#include <math.h>
#include <stdint.h>
#include <unistd.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>

#ifdef __MACH__
#include <mach/mach_time.h>
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 0
int clock_gettime(int clk_id, struct mach_timespec *t) {
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    uint64_t time;
    time = mach_absolute_time();
    double nseconds = ((double)time * (double)timebase.numer)/((double)timebase.denom);
    double seconds = ((double)time * (double)timebase.numer)/((double)timebase.denom * 1e9);
    t->tv_sec = seconds;
    t->tv_nsec = nseconds;
    return 0;
}
#else
#include <time.h>
#endif



#include <fitsio.h>

#include "CommandLineInterface/CLIcore.h"
#include "00CORE/00CORE.h"
#include "COREMOD_memory/COREMOD_memory.h"
#include "COREMOD_iofits/COREMOD_iofits.h"
#include "COREMOD_tools/COREMOD_tools.h"
#include "COREMOD_arith/COREMOD_arith.h"
#include "info/info.h"

#include "statistic/statistic.h"

#include "AOloopControl/AOloopControl.h"
#include "AOloopControl_IOtools/AOloopControl_IOtools.h"
#include "AOloopControl_acquireCalib/AOloopControl_acquireCalib.h"


/* =============================================================================================== */
/* =============================================================================================== */
/*                                      DEFINES, MACROS                                            */
/* =============================================================================================== */
/* =============================================================================================== */



# ifdef _OPENMP
# include <omp.h>
#define OMP_NELEMENT_LIMIT 1000000
# endif




/* =============================================================================================== */
/* =============================================================================================== */
/*                                  GLOBAL DATA DECLARATION                                        */
/* =============================================================================================== */
/* =============================================================================================== */

//extern long aoloopcontrol_var.aoconfID_dmRM;
//extern long aoloopcontrol_var.aoconfID_wfsim;
//extern long aoloopcontrol_var.aoconfID_imWFS0;
//extern long aoloopcontrol_var.aoconfID_imWFS1;
//extern long aoloopcontrol_var.aoconfID_imWFS1;
//extern long aoloopcontrol_var.aoconfID_cmd_modesRM;

static int RMACQUISITION = 0;  // toggles to 1 when resp matrix is being acquired
static int INITSTATUS_AOloopControl_acquireCalib = 0;

extern long LOOPNUMBER; // current loop index
extern int AOloopcontrol_meminit; // declared in AOloopControl_compTools.c


/* =============================================================================================== */
/*                                     MAIN DATA STRUCTURES                                        */
/* =============================================================================================== */

extern AOLOOPCONTROL_CONF *AOconf; // declared in AOloopControl.c
extern AOloopControl_var aoloopcontrol_var;





/* =============================================================================================== */
/* =============================================================================================== */
/** @name AOloopControl_acquireCalib - 1. ACQUIRING CALIBRATION
 *  Measure system response */
/* =============================================================================================== */
/* =============================================================================================== */

/** @brief CLI function for AOloopControl_RespMatrix_Fast */
int_fast8_t AOloopControl_acquireCalib_RespMatrix_Fast_cli() {
    if(CLI_checkarg(1,4)+CLI_checkarg(2,4)+CLI_checkarg(3,4)+CLI_checkarg(4,2)+CLI_checkarg(5,1)+CLI_checkarg(6,1)+CLI_checkarg(7,1)+CLI_checkarg(8,3)==0) {
        AOloopControl_acquireCalib_RespMatrix_Fast(data.cmdargtoken[1].val.string, data.cmdargtoken[2].val.string, data.cmdargtoken[3].val.string, data.cmdargtoken[4].val.numl, data.cmdargtoken[5].val.numf, data.cmdargtoken[6].val.numf, data.cmdargtoken[7].val.numf, data.cmdargtoken[8].val.string);
        return 0;
    }
    else return 1;
}

/** @brief CLI function for AOloopControl_Measure_WFSrespC */
int_fast8_t AOloopControl_acquireCalib_Measure_WFSrespC_cli() {
    if(CLI_checkarg(1,2)+CLI_checkarg(2,2)+CLI_checkarg(3,2)+CLI_checkarg(4,2)+CLI_checkarg(5,4)+CLI_checkarg(6,5)+CLI_checkarg(7,2)+CLI_checkarg(8,2)+CLI_checkarg(9,2)+CLI_checkarg(10,2)==0) {
        AOloopControl_acquireCalib_Measure_WFSrespC(LOOPNUMBER, data.cmdargtoken[1].val.numl, data.cmdargtoken[2].val.numl, data.cmdargtoken[3].val.numl, data.cmdargtoken[4].val.numl, data.cmdargtoken[5].val.string, data.cmdargtoken[6].val.string, data.cmdargtoken[7].val.numl, data.cmdargtoken[8].val.numl, data.cmdargtoken[9].val.numl, data.cmdargtoken[10].val.numl);
        return 0;
    }
    else return 1;
}

/** @brief CLI function for AOloopControl_Measure_WFS_linResponse */
int_fast8_t AOloopControl_acquireCalib_Measure_WFS_linResponse_cli() {
    if(CLI_checkarg(1,1)+CLI_checkarg(2,2)+CLI_checkarg(3,2)+CLI_checkarg(4,2)+CLI_checkarg(5,2)+CLI_checkarg(6,4)+CLI_checkarg(7,5)+CLI_checkarg(8,5)+CLI_checkarg(9,2)+CLI_checkarg(10,2)+CLI_checkarg(11,2)+CLI_checkarg(12,2)==0) {
        AOloopControl_acquireCalib_Measure_WFS_linResponse(LOOPNUMBER, data.cmdargtoken[1].val.numf, data.cmdargtoken[2].val.numl, data.cmdargtoken[3].val.numl, data.cmdargtoken[4].val.numl, data.cmdargtoken[5].val.numl, data.cmdargtoken[6].val.string, data.cmdargtoken[7].val.string, data.cmdargtoken[8].val.string, data.cmdargtoken[9].val.numl, data.cmdargtoken[10].val.numl, data.cmdargtoken[11].val.numl, data.cmdargtoken[12].val.numl);
        return 0;
    }
    else return 1;
}

/** @brief CLI function for AOloopControl_Measure_zonalRM */
int_fast8_t AOloopControl_acquireCalib_Measure_zonalRM_cli() {
    if(CLI_checkarg(1,1)+CLI_checkarg(2,2)+CLI_checkarg(3,2)+CLI_checkarg(4,2)+CLI_checkarg(5,2)+CLI_checkarg(6,3)+CLI_checkarg(7,3)+CLI_checkarg(8,3)+CLI_checkarg(9,3)+CLI_checkarg(10,2)+CLI_checkarg(11,2)+CLI_checkarg(12,2)+CLI_checkarg(13,2)==0) {
        AOloopControl_acquireCalib_Measure_zonalRM(LOOPNUMBER, data.cmdargtoken[1].val.numf, data.cmdargtoken[2].val.numl, data.cmdargtoken[3].val.numl, data.cmdargtoken[4].val.numl, data.cmdargtoken[5].val.numl, data.cmdargtoken[6].val.string, data.cmdargtoken[7].val.string, data.cmdargtoken[8].val.string, data.cmdargtoken[9].val.string, data.cmdargtoken[10].val.numl, data.cmdargtoken[11].val.numl, data.cmdargtoken[12].val.numl, data.cmdargtoken[13].val.numl);
        return 0;
    }
    else return 1;
}

/** @brief CLI function for AOloopControl_Measure_Resp_Matrix */
int_fast8_t AOloopControl_acquireCalib_Measure_Resp_Matrix_cli() {
    if(CLI_checkarg(1,2)+CLI_checkarg(2,1)+CLI_checkarg(3,2)+CLI_checkarg(4,2)+CLI_checkarg(5,2)==0) {
        AOloopControl_acquireCalib_Measure_Resp_Matrix(LOOPNUMBER, data.cmdargtoken[1].val.numl, data.cmdargtoken[2].val.numf, data.cmdargtoken[3].val.numl, data.cmdargtoken[4].val.numl, data.cmdargtoken[5].val.numl);
        return 0;
    }
    else return 1;
}




















/* =============================================================================================== */
/* =============================================================================================== */
/*                                    FUNCTIONS SOURCE CODE                                        */
/* =============================================================================================== */
/* =============================================================================================== */
/** @name AOloopControl_IOtools functions */

void __attribute__ ((constructor)) libinit_AOloopControl_acquireCalib()
{
	if( INITSTATUS_AOloopControl_acquireCalib == 0)
	{
		init_AOloopControl_acquireCalib();
		RegisterModule(__FILE__, "cacao", "AO loop control acquire calibration");
		INITSTATUS_AOloopControl_acquireCalib = 1;
	}
}
            
int_fast8_t init_AOloopControl_acquireCalib()
{

/* =============================================================================================== */
/* =============================================================================================== */
/** @name AOloopControl_acquireCalib - 1. ACQUIRING CALIBRATION                                                 */
/* =============================================================================================== */
/* =============================================================================================== */
  

    RegisterCLIcommand("aolacqresp", __FILE__, AOloopControl_acquireCalib_Measure_Resp_Matrix_cli, "acquire AO response matrix and WFS reference", "<ave# [long]> <ampl [float]> <nbloop [long]> <frameDelay [long]> <NBiter [long]>", "aolacqresp 50 0.1 5 2", "intAOloopControl_acquireCalib_ Measure_Resp_Matrix(long loop, long NbAve, float amp, long nbloop, long fDelay, long NBiter)");

    RegisterCLIcommand("aolmRMfast", __FILE__, AOloopControl_acquireCalib_RespMatrix_Fast_cli, "acquire fast modal response matrix", "<modes> <dm RM stream> <WFS stream> <sem trigger> <hardware latency [s]> <loop frequ [Hz]> <ampl [um]> <outname>", "aolmRMfast DMmodes aol0_dmRM aol0_wfsim 4 0.00112 2000.0 0.03 rm000", "long AOloopControl_acquireCalib_RespMatrix_Fast(char *DMmodes_name, char *dmRM_name, char *imWFS_name, long semtrig, float HardwareLag, float loopfrequ, float ampl, char *outname)");

    RegisterCLIcommand("aolmeasWFSrespC",__FILE__, AOloopControl_acquireCalib_Measure_WFSrespC_cli, "measure WFS resp to DM patterns", "<delay frames [long]> <DMcommand delay us [long]> <nb frames per position [long]> <nb frames excluded [long]> <input DM patter cube [string]> <output response [string]> <normalize flag> <AOinitMode> <NBcycle>", "aolmeasWFSrespC 2 135 20 0 dmmodes wfsresp 1 0 5", "long AOloopControl_acquireCalib_Measure_WFSrespC(long loop, long delayfr, long delayRM1us, long NBave, long NBexcl, char *IDpokeC_name, char *IDoutC_name, int normalize, int AOinitMode, long NBcycle, int SequInitMode);");

    RegisterCLIcommand("aolmeaslWFSrespC",__FILE__, AOloopControl_acquireCalib_Measure_WFS_linResponse_cli, "measure linear WFS response to DM patterns", "<ampl [um]> <delay frames [long]> <DMcommand delay us [long]> <nb frames per position [long]> <nb frames excluded [long]> <input DM patter cube [string]> <output response [string]> <output reference [string]> <normalize flag> <AOinitMode> <NBcycle>", "aolmeasWFSrespC 0.05 2 135 20 0 dmmodes wfsresp wfsref 1 0 5", "long AOloopControl_acquireCalib_Measure_WFS_linResponse(long loop, float ampl, long delayfr, long delayRM1us, long NBave, long NBexcl, char *IDpokeC_name, char *IDrespC_name, char *IDwfsref_name, int normalize, int AOinitMode, long NBcycle, long NBinnerCycle)");

    RegisterCLIcommand("aolmeaszrm",__FILE__, AOloopControl_acquireCalib_Measure_zonalRM_cli, "measure zonal resp mat, WFS ref, DM and WFS response maps", "<ampl [float]> <delay frames [long]> <DMcommand delay us [long]> <nb frames per position [long]> <nb frames excluded [long]> <output image [string]> <output WFS ref [string]>  <output WFS response map [string]>  <output DM response map [string]> <mode> <normalize flag> <AOinitMode> <NBcycle>", "aolmeaszrm 0.05 2 135 20 zrm wfsref wfsmap dmmap 1 0 0 0", "long AOloopControl_acquireCalib_Measure_zonalRM(long loop, double ampl, long delayfr, long delayRM1us, long NBave, long NBexcl, char *zrespm_name, char *WFSref_name, char *WFSmap_name, char *DMmap_name, long mode, int normalize, int AOinitMode, long NBcycle)");




    // add atexit functions here
    // atexit((void*) myfunc);

    return 0;
}












/* =============================================================================================== */
/* =============================================================================================== */
/** @name AOloopControl - 4. ACQUIRING CALIBRATION                                                 */
/* =============================================================================================== */
/* =============================================================================================== */



/**
 * ## Purpose
 * 
 * Acquire WFS response to a series of DM patterns.\n
 * 
 * 
 * ## Arguments
 * 
 * @param[in]  loop            Loop index
 * @param[in]  delayfr         Integer delay [frame]
 * @param[in]  delayRM1us      Fractional delay [us]
 * @param[in]  NBave           Number of frames averaged per DM state
 * @param[in]  NBexcl          Number of frames excluded
 * @param[in]  IDpokeC_name    Poke pattern
 * @param[out] IDoutC_name     Output cube
 * @param[in]  normalize       Normalize flag
 * @param[in]  AOinitMode      AO structure initialization flag
 * @param[in]  NBcycle         Number of cycles averaged (outer)
 * @param[in]  SequInitMode    Sequence initialization mode (0 or 1)
 * 
 * AOinitMode = 0:  create AO shared mem struct
 * AOinitMode = 1:  connect only to AO shared mem struct
 * 
 * INPUT : DMpoke_name : set of DM patterns
 * OUTPUT : WFSmap_name : WFS response maps
 * 
 * USR1 signal will stop acquisition immediately
 * USR2 signal completes current cycles and stops acquisition
 * 
 * @return IDoutC
 * 
 */

long AOloopControl_acquireCalib_Measure_WFSrespC(
    long        loop,
    long        delayfr,
    long        delayRM1us,
    long        NBave,
    long        NBexcl,
    const char *IDpokeC_name,
    const char *IDoutC_name,
    int         normalize,
    int         AOinitMode,
    long        NBcycle,
    int         SequInitMode
)
{
    char fname[200];
    char name[200];
    char command[200];
    uint32_t *sizearray;
    long IDoutC;

    long NBiter = 10000; // runs until USR1 signal received
    long iter;
    int r;
    long IDpokeC;
    long NBpoke;
    long framesize;
    char *ptr0; // source
    float *arrayf;
    int RT_priority = 80; //any number from 0-99
    struct sched_param schedpar;
    int ret;
    long imcnt;
    long ii;

    long imcntmax;

    FILE *fp;

    PROCESSINFO *processinfo;

    if((data.processinfo==1)&&(data.processinfoActive==0))
    {
        // CREATE PROCESSINFO ENTRY
        // see processtools.c in module CommandLineInterface for details
        //

        char pinfoname[200];  // short name for the processinfo instance
        // avoid spaces, name should be human-readable


        sprintf(pinfoname, "aol%ld-acqRM", loop);
        processinfo = processinfo_shm_create(pinfoname, 0);
        processinfo->loopstat = 0; // loop initialization
        strcpy(processinfo->source_FUNCTION, __FUNCTION__);
        strcpy(processinfo->source_FILE,     __FILE__);
        processinfo->source_LINE = __LINE__;

        char msgstring[200];
        sprintf(msgstring, "delay=%ld+%ldus ave=%ld excl=%ld cyc=%ld", delayfr, delayRM1us, NBave, NBexcl, NBcycle);
        processinfo_WriteMessage(processinfo, msgstring);
        data.processinfoActive = 1;
    }

    // CATCH SIGNALS

    if (sigaction(SIGTERM, &data.sigact, NULL) == -1)
        printf("\ncan't catch SIGTERM\n");

    if (sigaction(SIGINT, &data.sigact, NULL) == -1)
        printf("\ncan't catch SIGINT\n");

    if (sigaction(SIGABRT, &data.sigact, NULL) == -1)
        printf("\ncan't catch SIGABRT\n");

    if (sigaction(SIGBUS, &data.sigact, NULL) == -1)
        printf("\ncan't catch SIGBUS\n");

    if (sigaction(SIGSEGV, &data.sigact, NULL) == -1)
        printf("\ncan't catch SIGSEGV\n");

    if (sigaction(SIGHUP, &data.sigact, NULL) == -1)
        printf("\ncan't catch SIGHUP\n");

    if (sigaction(SIGPIPE, &data.sigact, NULL) == -1)
        printf("\ncan't catch SIGPIPE\n");


    /** ## DETAILS, STEPS */


    /**
     * Process is given high priority to minimize timing jitter
     *
     */

    schedpar.sched_priority = RT_priority;
#ifndef __MACH__
    sched_setscheduler(0, SCHED_FIFO, &schedpar);
#endif


    /**
     * If NBcycle is set to zero, then the process should run in an infinite loop.
     * The process will then run until receiving USR1.
     *
     */
    if(NBcycle < 1)
        NBiter = LONG_MAX; // runs until USR1 signal received
    else
        NBiter = NBcycle;




    sizearray = (uint32_t*) malloc(sizeof(uint32_t)*3);


    printf("INITIALIZE MEMORY (mode %d, meminit = %d)....\n", AOinitMode, AOloopcontrol_meminit);
    fflush(stdout);

    if(AOloopcontrol_meminit==0)
        AOloopControl_InitializeMemory(AOinitMode);

    AOloopControl_loadconfigure(LOOPNUMBER, 1, 2);






    printf("Importing DM response matrix channel shared memory ...\n");
    fflush(stdout);
    aoloopcontrol_var.aoconfID_dmRM = read_sharedmem_image(AOconf[loop].DMctrl.dmRMname);

    printf("Importing WFS camera image shared memory ... \n");
    fflush(stdout);
    aoloopcontrol_var.aoconfID_wfsim = read_sharedmem_image(AOconf[loop].WFSim.WFSname);




    /**
     * A temporary array is created and initialized to hold the WFS response to each poke mode.
     *
     */
    IDpokeC = image_ID(IDpokeC_name);
    NBpoke = data.image[IDpokeC].md[0].size[2];
    sizearray[0] = AOconf[loop].WFSim.sizexWFS;
    sizearray[1] = AOconf[loop].WFSim.sizeyWFS;
    sizearray[2] = NBpoke;
    IDoutC = create_3Dimage_ID(IDoutC_name, sizearray[0], sizearray[1], sizearray[2]);

    uint_fast16_t PokeIndex;   // Mode to be poked
    for(PokeIndex = 0; PokeIndex < NBpoke; PokeIndex++)
        for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
            data.image[IDoutC].array.F[PokeIndex*AOconf[loop].WFSim.sizeWFS+ii] = 0.0;


    /**
     * WFS frames will arrive in aol_imWFS1RM
     *
     */
    if(sprintf(name, "aol%ld_imWFS1RM", loop) < 1)
        printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");
    aoloopcontrol_var.aoconfID_imWFS1 = create_image_ID(name, 2, sizearray, _DATATYPE_FLOAT, 1, 0);




    /**
     * A temporary array is created to hold the DM command
     */
    arrayf = (float*) malloc(sizeof(float)*AOconf[loop].DMctrl.sizeDM);
    for(ii=0; ii<AOconf[loop].DMctrl.sizeDM; ii++)
        arrayf[ii] = 0.0;








    iter = 0;


    ptr0 = (char*) data.image[IDpokeC].array.F;
    framesize = sizeof(float)*AOconf[loop].DMctrl.sizexDM*AOconf[loop].DMctrl.sizeyDM;

    printf("STARTING response measurement...\n");
    fflush(stdout);

    list_image_ID();

    /**
     * Memory is allocated to arrays
     */
    imcntmax          = (4+delayfr+(NBave+NBexcl)*NBpoke)*NBiter + 4;

    long *array_iter;
    array_iter        = (long*) malloc(sizeof(long)*imcntmax);  // Cycle number

    int  *array_poke;
    array_poke        = (int*)  malloc(sizeof(int) *imcntmax);  // Did we poke DM during this time interval ?

    int  *array_accum;
    array_accum       = (int*)  malloc(sizeof(int) *imcntmax);  // Does frame count toward accumulated signal ?

    long *array_kk;
    array_kk          = (long*) malloc(sizeof(long)*imcntmax);  // frame index within poke mode acquisition

    // frame counter within poke mode acquisition, starts negative
    // becomes positive when accumulating signal
    long *array_kk1;
    array_kk1         = (long*) malloc(sizeof(long)*imcntmax);

    long *array_PokeIndex;
    array_PokeIndex   = (long*) malloc(sizeof(long)*imcntmax);  // Poke mode being measured

    long *array_PokeIndex1;
    array_PokeIndex1  = (long*) malloc(sizeof(long)*imcntmax);  // Current poke mode on DM


    long *array_PokeIndexMapped;
    array_PokeIndexMapped   = (long*) malloc(sizeof(long)*imcntmax);  // Poke mode being measured, index in poke cube

    long *array_PokeIndex1Mapped;
    array_PokeIndex1Mapped  = (long*) malloc(sizeof(long)*imcntmax);  // Current poke mode on DM, index in poke cube




    /**
     * Poke sequence defines the sequence of mode poked
     *
     */
    long *array_PokeSequ;
    array_PokeSequ    = (long*) malloc(sizeof(long)*NBpoke);

    for(PokeIndex = 0; PokeIndex < NBpoke; PokeIndex++)
        array_PokeSequ[PokeIndex] = PokeIndex;
    if(SequInitMode > 0) // swap pairs every 4 indices
    {
        for(PokeIndex = 0; PokeIndex < NBpoke-1; PokeIndex += 4)
        {
            int index0 = PokeIndex;
            int index1 = PokeIndex + SequInitMode;

            while(index1 > NBpoke-1)
                index1 -= NBpoke;

            // swap sequence pairs
            long tmpPokeMode;
            tmpPokeMode = array_PokeSequ[index0];
            array_PokeSequ[index0] = array_PokeSequ[index1];
            array_PokeSequ[index1] = tmpPokeMode;
        }
    }


    int permut_offset = 0;


    for(imcnt=0; imcnt<imcntmax; imcnt++)
    {
        array_poke[imcnt]  = 0;
        array_accum[imcnt] = 0;
    }

    imcnt = 0;


    int loopOK = 1;
    if(data.processinfo==1)
        processinfo->loopstat = 1;  // Notify processinfo that we are entering loop
    long loopcnt = 0;

    /**
     * The outermost loop increments the measurement cycle.
     * Signal is to be averaged among cycles. Each measurement cycle repeats the same sequence.
     *
     */

    while(loopOK==1)
    {
        if(data.processinfo==1)
        {
            while(processinfo->CTRLval == 1)  // pause
                usleep(50);

            if(processinfo->CTRLval == 2) // single iteration
                processinfo->CTRLval = 1;

            if(processinfo->CTRLval == 3) // exit loop
            {
                loopOK = 0;
            }
        }


        printf("NBpoke=%ld # %3ld/%3ld (%6ld/%6ld)\n", NBpoke, iter, NBiter, imcnt, imcntmax);
        fflush(stdout);

      /*  if(data.processinfo==1)
        {
            char msgstring[200];
            sprintf(msgstring, "NBpoke=%ld # %3ld/%3ld (%6ld/%6ld)", NBpoke, iter, NBiter, imcnt, imcntmax);
            processinfo_WriteMessage(processinfo, msgstring);
        }*/


        // RE-ORDER POKE SEQUENCE
        // adjacent pairs are swapped
        permut_offset ++;
        if(permut_offset == 2)
            permut_offset = 0;

        for(PokeIndex = permut_offset; PokeIndex < NBpoke; PokeIndex += 2)
        {
            int index0 = PokeIndex;
            int index1 = PokeIndex+1;

            if(index1 > NBpoke-1)
                index1 -= NBpoke;

            // swap sequence pairs
            long tmpPokeMode;
            tmpPokeMode = array_PokeSequ[index0];
            array_PokeSequ[index0] = array_PokeSequ[index1];
            array_PokeSequ[index1] = tmpPokeMode;
        }



        // INITIALIZE WITH FIRST POKE

        long kk  = 0;
        long kk1 = 0;

        uint_fast16_t PokeIndex  = 0;    // Poked mode index
        uint_fast16_t PokeIndex1 = 0;

        uint_fast16_t PokeIndexMapped;   // Poked mode index in original poke cube
        uint_fast16_t PokeIndex1Mapped;

        PokeIndexMapped  = array_PokeSequ[PokeIndex];
        PokeIndex1Mapped = array_PokeSequ[PokeIndex1];

        if((PokeIndex1Mapped<0)||(PokeIndex1Mapped>NBpoke-1))
        {
            printf("ERROR: PokeIndex1Mapped = %ld is outside range 0 - %ld\n", PokeIndex1Mapped, NBpoke);
            exit(0);
        }

        if(data.processinfo==1)//TEST
        {
            char msgstring[200];
            sprintf(msgstring, "poke = %ld", PokeIndex1Mapped);
            processinfo_WriteMessage(processinfo, msgstring);
        }

        usleep(delayRM1us);
        data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 1;
        memcpy ((void*) data.image[aoloopcontrol_var.aoconfID_dmRM].array.F, (void*) (ptr0 + PokeIndex1Mapped*framesize), sizeof(float)*AOconf[loop].DMctrl.sizeDM);
        COREMOD_MEMORY_image_set_sempost_byID(aoloopcontrol_var.aoconfID_dmRM, -1);
        data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].cnt1 = PokeIndex1Mapped;
        data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].cnt0++;
        data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 0;
        AOconf[loop].aorun.DMupdatecnt ++;
        array_poke[imcnt] = 1;




        // WAIT FOR LOOP DELAY, PRIMING

        array_iter[imcnt]             = iter;
        array_kk[imcnt]               = kk;
        array_kk1[imcnt]              = kk1;
        array_PokeIndex[imcnt]        = PokeIndex;
        array_PokeIndex1[imcnt]       = PokeIndex1;
        array_PokeIndexMapped[imcnt]  = PokeIndexMapped;
        array_PokeIndex1Mapped[imcnt] = PokeIndex1Mapped;
        imcnt ++;

        Read_cam_frame(loop, 1, normalize, 0, 0);
        COREMOD_MEMORY_image_set_sempost_byID(aoloopcontrol_var.aoconfID_dmRM, -1);
        data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].cnt0++;

        // read delayfr frames
        for(kk=0; kk<delayfr; kk++)
        {
            array_iter[imcnt]             = iter;
            array_kk[imcnt]               = kk;
            array_kk1[imcnt]              = kk1;
            array_PokeIndex[imcnt]        = PokeIndex;
            array_PokeIndex1[imcnt]       = PokeIndex1;
            array_PokeIndexMapped[imcnt]  = PokeIndexMapped;
            array_PokeIndex1Mapped[imcnt] = PokeIndex1Mapped;
            imcnt ++;

            Read_cam_frame(loop, 1, normalize, 0, 0);

            kk1++;
            if(kk1==NBave)
            {
                kk1 = -NBexcl;
                PokeIndex1++;

                if(PokeIndex1>NBpoke-1)
                    PokeIndex1 -= NBpoke;

                PokeIndex1Mapped  = array_PokeSequ[PokeIndex1];

                if((PokeIndex1Mapped<0)||(PokeIndex1Mapped>NBpoke-1))
                {
                    printf("ERROR: PokeIndex1Mapped = %ld is outside range 0 - %ld\n", PokeIndex1Mapped, NBpoke);
                    exit(0);
                }

        if(data.processinfo==1)//TEST
        {
            char msgstring[200];
            sprintf(msgstring, "poke = %ld", PokeIndex1Mapped);
            processinfo_WriteMessage(processinfo, msgstring);
        }

                // POKE
                usleep(delayRM1us);
                data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 1;
                //memcpy (data.image[aoloopcontrol_var.aoconfID_dmRM].array.F, ptr0 + PokeIndex1Mapped*framesize, sizeof(float)*AOconf[loop].DMctrl.sizeDM); //TEST
                COREMOD_MEMORY_image_set_sempost_byID(aoloopcontrol_var.aoconfID_dmRM, -1);
                data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].cnt1 = PokeIndex1Mapped;
                data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].cnt0++;
                data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 0;
                AOconf[loop].aorun.DMupdatecnt ++;
                array_poke[imcnt] = 1;
            }
        }




        /**
         * First inner loop increment poke mode
         *
         */
        while ((PokeIndex < NBpoke)&&(data.signal_USR1==0))
        {
            // INTEGRATION

            for(kk=0; kk<NBave+NBexcl; kk++)
            {
                array_iter[imcnt]             = iter;
                array_kk[imcnt]               = kk;
                array_kk1[imcnt]              = kk1;
                array_PokeIndex[imcnt]        = PokeIndex;
                array_PokeIndex1[imcnt]       = PokeIndex1;
                array_PokeIndexMapped[imcnt]  = PokeIndexMapped;
                array_PokeIndex1Mapped[imcnt] = PokeIndex1Mapped;
                imcnt ++;

                Read_cam_frame(loop, 1, normalize, 0, 0);


                if(kk<NBave)
                {
                    for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                        data.image[IDoutC].array.F[PokeIndexMapped*AOconf[loop].WFSim.sizeWFS+ii] += data.image[aoloopcontrol_var.aoconfID_imWFS1].array.F[ii];
                    array_accum[imcnt] = 1;
                }
                kk1++;
                if(kk1==NBave)
                {
                    kk1 = -NBexcl;
                    PokeIndex1++;

                    if(PokeIndex1>NBpoke-1)
                        PokeIndex1 -= NBpoke;

                    PokeIndex1Mapped  = array_PokeSequ[PokeIndex1];

                    if((PokeIndex1Mapped<0)||(PokeIndex1Mapped>NBpoke-1))
                    {
                        printf("ERROR: PokeIndex1Mapped = %ld is outside range 0 - %ld\n", PokeIndex1Mapped, NBpoke);
                        exit(0);
                    }

                    usleep(delayRM1us);
                    data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 1;
                    //memcpy (data.image[aoloopcontrol_var.aoconfID_dmRM].array.F, ptr0 + PokeIndex1Mapped*framesize, sizeof(float)*AOconf[loop].DMctrl.sizeDM); //TEST
                    COREMOD_MEMORY_image_set_sempost_byID(aoloopcontrol_var.aoconfID_dmRM, -1);
                    data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].cnt1 = PokeIndex1Mapped;
                    data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].cnt0++;
                    data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 0;
                    AOconf[loop].aorun.DMupdatecnt ++;
                    array_poke[imcnt] = 1;
                }
            }

            PokeIndex++;
            PokeIndexMapped  = array_PokeSequ[PokeIndex];
        }


        for(ii=0; ii<AOconf[loop].DMctrl.sizeDM; ii++)
            arrayf[ii] = 0.0;

        // zero DM channel

        usleep(delayRM1us);
        data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 1;
        memcpy ((void*) (data.image[aoloopcontrol_var.aoconfID_dmRM].array.F) , (void*) (arrayf), sizeof(float)*AOconf[loop].DMctrl.sizeDM); 
        COREMOD_MEMORY_image_set_sempost_byID(aoloopcontrol_var.aoconfID_dmRM, -1);
        data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].cnt1 = 0;
        data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].cnt0++;
        data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 0;
        AOconf[loop].aorun.DMupdatecnt ++;
        array_poke[imcnt] = 1;

        iter++;




        // process signals

        if(data.signal_TERM == 1) {
            loopOK = 0;
            if(data.processinfo==1)
                processinfo_SIGexit(processinfo, SIGTERM);
        }

        if(data.signal_INT == 1) {
            loopOK = 0;
            if(data.processinfo==1)
                processinfo_SIGexit(processinfo, SIGINT);
        }

        if(data.signal_ABRT == 1) {
            loopOK = 0;
            if(data.processinfo==1)
                processinfo_SIGexit(processinfo, SIGABRT);
        }

        if(data.signal_BUS == 1) {
            loopOK = 0;
            if(data.processinfo==1)
                processinfo_SIGexit(processinfo, SIGBUS);
        }

        if(data.signal_SEGV == 1) {
            loopOK = 0;
            if(data.processinfo==1)
                processinfo_SIGexit(processinfo, SIGSEGV);
        }

        if(data.signal_HUP == 1) {
            loopOK = 0;
            if(data.processinfo==1)
                processinfo_SIGexit(processinfo, SIGHUP);
        }

        if(data.signal_PIPE == 1) {
            loopOK = 0;
            if(data.processinfo==1)
                processinfo_SIGexit(processinfo, SIGPIPE);
        }

        loopcnt++;
        if(data.processinfo==1)
            processinfo->loopcnt = loopcnt;



        if(iter==NBiter)
            loopOK = 0;
        if(data.signal_USR1==1)
            loopOK = 0;
        if(data.signal_USR2==1)
            loopOK = 0;
    } // end of iteration loop

    free(arrayf);

    free(sizearray);

    if(data.processinfo==1)
        processinfo_cleanExit(processinfo);


    for(PokeIndex = 0; PokeIndex < NBpoke; PokeIndex++)
        for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
            data.image[IDoutC].array.F[PokeIndex*AOconf[loop].WFSim.sizeWFS+ii] /= NBave*iter;


    // print poke log
    fp = fopen("RMpokelog.txt", "w");
    for(imcnt=0; imcnt<imcntmax; imcnt++)
    {
        fprintf(fp, "%6ld %3ld    %1d %1d     %6ld  %6ld     %4ld %4ld   %4ld %4ld     %3ld %3ld %3ld\n",
                imcnt,
                array_iter[imcnt],
                array_poke[imcnt],
                array_accum[imcnt],
                array_kk[imcnt],
                array_kk1[imcnt],
                array_PokeIndex[imcnt],
                array_PokeIndex1[imcnt],
                array_PokeIndexMapped[imcnt],
                array_PokeIndex1Mapped[imcnt],
                NBpoke,
                NBexcl,
                NBave
               );
    }
    fclose(fp);


    free(array_iter);
    free(array_accum);
    free(array_poke);
    free(array_kk);
    free(array_kk1);
    free(array_PokeIndex);
    free(array_PokeIndex1);
    free(array_PokeIndexMapped);
    free(array_PokeIndex1Mapped);
    free(array_PokeSequ);

    return(IDoutC);
}











/**
 * ## Purpose
 *
 *  Measure the WFS linear response to a set of DM patterns
 *
 * This function creates positive and negative maps which are sent by AOloopControl_acquireCalib_Measure_WFSrespC() \n
 * Bleeding from one pattern to the next due to limited DM time response is canceled by using two patterns \n
 *
 *
 * pattern a sequence : +- +- +- +- ... \n
 * pattern b sequence : +- -+ +- -+ ... \n
 *
 */
long AOloopControl_acquireCalib_Measure_WFS_linResponse(
    long        loop,
    float       ampl,
    long        delayfr,
    long        delayRM1us,
    long        NBave,
    long        NBexcl,
    const char *IDpokeC_name,
    const char *IDrespC_name,
    const char *IDwfsref_name,
    int         normalize,
    int         AOinitMode,
    long        NBcycle,
    long        NBinnerCycle
)
{
    long IDrespC;
    long IDpokeC;
    long dmxsize, dmysize, dmxysize;
    long wfsxsize, wfsysize, wfsxysize;
    long NBpoke, NBpoke2;
    long IDpokeC2a, IDpokeC2b; // poke sequences a, b used to remove time bleeding effects in linear regime
    long IDwfsresp2a, IDwfsresp2b;
    long poke, act, pix;
    long IDwfsref;

    int *pokesign;
    int pokesigntmp;
    char *ptra;
    char *ptrb;
    char *ptra0;
    char *ptrb0;


    long NBinnerCycleC;
    if(NBinnerCycle < 1)
        NBinnerCycleC = 1;
    else
        NBinnerCycleC = NBinnerCycle;



    IDpokeC = image_ID(IDpokeC_name);
    dmxsize = data.image[IDpokeC].md[0].size[0];
    dmysize = data.image[IDpokeC].md[0].size[1];
    dmxysize = dmxsize*dmysize;
    NBpoke = data.image[IDpokeC].md[0].size[2];

    NBpoke2 = 2*NBpoke*NBinnerCycleC + 4; // add zero frame before and after


    IDpokeC2a = create_3Dimage_ID("dmpokeC2a", dmxsize, dmysize, NBpoke2);
    IDpokeC2b = create_3Dimage_ID("dmpokeC2b", dmxsize, dmysize, NBpoke2);




    // set start and end frames to zero

    for(act=0; act<dmxysize; act++)
    {
        data.image[IDpokeC2a].array.F[act] = 0.0;
        data.image[IDpokeC2a].array.F[dmxysize + act] = 0.0;
        data.image[IDpokeC2a].array.F[dmxysize*(2*NBpoke*NBinnerCycleC+2) + act] = 0.0;
        data.image[IDpokeC2a].array.F[dmxysize*(2*NBpoke*NBinnerCycleC+3) + act] = 0.0;
    }

    ptra0 = (char*) data.image[IDpokeC2a].array.F;
    ptrb0 = (char*) data.image[IDpokeC2b].array.F;
    memcpy((void *) ptrb0, (void *) ptra0, sizeof(float)*dmxysize);

    ptrb = ptrb0 + sizeof(float)*dmxysize;
    memcpy((void *) ptrb, (void *) ptra0, sizeof(float)*dmxysize);

    ptrb = ptrb0 + sizeof(float)*dmxysize*(2*NBpoke*NBinnerCycleC+2);
    memcpy((void *) ptrb, (void *) ptra0, sizeof(float)*dmxysize);

    ptrb = ptrb0 + sizeof(float)*dmxysize*(2*NBpoke*NBinnerCycleC+2);
    memcpy((void *) ptrb, (void *) ptra0, sizeof(float)*dmxysize);






    pokesign = (int*) malloc(sizeof(int)*NBpoke);

    pokesigntmp = 1;

    int pokeindex;
    pokeindex = 2; // accounts for first two zero pokes

    for(poke=0; poke<NBpoke; poke++)
    {
        int innercycle;
        for(innercycle=0; innercycle < NBinnerCycleC; innercycle++)
        {
            // note
            // old incices were 2*poke+2 and 2*poke+3

            for(act=0; act<dmxysize; act++)
                data.image[IDpokeC2a].array.F[dmxysize*(pokeindex) + act]   =  ampl*data.image[IDpokeC].array.F[dmxysize*poke+act];
            for(act=0; act<dmxysize; act++)
                data.image[IDpokeC2a].array.F[dmxysize*(pokeindex+1) + act] = -ampl*data.image[IDpokeC].array.F[dmxysize*poke+act];


            // swap one pair out of two in cube IDpokeC2b
            pokesign[poke] = pokesigntmp;
            if(pokesign[poke]==1)  // do not swap
            {
                ptra = ptra0 + sizeof(float)*dmxysize*(pokeindex);
                ptrb = ptrb0 + sizeof(float)*dmxysize*(pokeindex);
                memcpy((void *) ptrb, (void *) ptra, sizeof(float)*dmxysize);

                ptra = ptra0 + sizeof(float)*dmxysize*(pokeindex+1);
                ptrb = ptrb0 + sizeof(float)*dmxysize*(pokeindex+1);
                memcpy((void *) ptrb, (void *) ptra, sizeof(float)*dmxysize);
            }
            else  // do swap
            {
                ptra = ptra0 + sizeof(float)*dmxysize*(pokeindex);
                ptrb = ptrb0 + sizeof(float)*dmxysize*(pokeindex+1);
                memcpy((void *) ptrb, (void *) ptra, sizeof(float)*dmxysize);

                ptra = ptra0 + sizeof(float)*dmxysize*(pokeindex+1);
                ptrb = ptrb0 + sizeof(float)*dmxysize*(pokeindex);
                memcpy((void *) ptrb, (void *) ptra, sizeof(float)*dmxysize);
            }

            pokeindex += 2;
        }

        if(pokesign[poke]==1)
            pokesigntmp = -1;
        else
            pokesigntmp = 1;
        //pokesigntmp = 1; // no inversion
    }



    save_fits("dmpokeC2a", "!tmp/test_dmpokeC2a.fits");
    save_fits("dmpokeC2b", "!tmp/test_dmpokeC2b.fits");


    printf("NBpoke = %ld\n", NBpoke);
    fflush(stdout);

    // Positive direction sequence
    AOloopControl_acquireCalib_Measure_WFSrespC(loop, delayfr, delayRM1us, NBave, NBexcl, "dmpokeC2a", "wfsresp2a", normalize, AOinitMode, (long) (NBcycle/2), 0);

    // Negative direction sequence
    AOloopControl_acquireCalib_Measure_WFSrespC(loop, delayfr, delayRM1us, NBave, NBexcl, "dmpokeC2b", "wfsresp2b", normalize, AOinitMode, (long) (NBcycle/2), 0);

    printf("STEP done\n");
    fflush(stdout);

    //	save_fits("wfsresp2", "!tmp/test_wfsresp2.fits");


    // process data cube
    IDwfsresp2a = image_ID("wfsresp2a");
    IDwfsresp2b = image_ID("wfsresp2b");
    wfsxsize = data.image[IDwfsresp2a].md[0].size[0];
    wfsysize = data.image[IDwfsresp2a].md[0].size[1];
    wfsxysize = wfsxsize*wfsysize;
    IDrespC = create_3Dimage_ID(IDrespC_name, wfsxsize, wfsysize, NBpoke);

    IDwfsref = create_2Dimage_ID(IDwfsref_name, wfsxsize, wfsysize);

    pokeindex = 2;
    for(poke=0; poke<NBpoke; poke++)
    {
        int innercycle;
        for(innercycle=0; innercycle < NBinnerCycleC; innercycle++)
        {
            // Sum response
            for(pix=0; pix<wfsxysize; pix++)
            {
                data.image[IDrespC].array.F[wfsxysize*poke + pix] += 0.5*(data.image[IDwfsresp2a].array.F[wfsxysize*(pokeindex) + pix] - data.image[IDwfsresp2a].array.F[wfsxysize*(pokeindex+1) + pix])/2.0/ampl/NBinnerCycleC;
                data.image[IDrespC].array.F[wfsxysize*poke + pix] += 0.5*pokesign[poke]*(data.image[IDwfsresp2b].array.F[wfsxysize*(pokeindex) + pix] - data.image[IDwfsresp2b].array.F[wfsxysize*(pokeindex+1) + pix])/2.0/ampl/NBinnerCycleC;
            }

            // Sum reference
            for(pix=0; pix<wfsxysize; pix++)
            {
                data.image[IDwfsref].array.F[pix] += (data.image[IDwfsresp2a].array.F[wfsxysize*(pokeindex) + pix] + data.image[IDwfsresp2a].array.F[wfsxysize*(pokeindex+1) + pix])/(2*NBpoke)/NBinnerCycleC;
                data.image[IDwfsref].array.F[pix] += (data.image[IDwfsresp2b].array.F[wfsxysize*(pokeindex) + pix] + data.image[IDwfsresp2b].array.F[wfsxysize*(pokeindex+1) + pix])/(2*NBpoke)/NBinnerCycleC;
            }

            pokeindex += 2;
        }

    }

    free(pokesign);


    return(IDrespC);
}




/** Measures zonal response matrix
 * -> collapses it to DM response map and WFS response map
 * (both maps show amplitude of actuator effect on WFS)
 *
 * mode :
 *  0: compute WFSmap and DMmap
 *  1: compute WFSmap, DMmap, WFSmask and DMmask  -> images wfsmask and dmmask
 * NOTE can take custom poke matrix (loaded in image name RMpokeCube)
 *
 * ASYNC = 1  -> record ALL frames and assemble the RM off-line
 *
 * AOinitMode = 0:  create AO shared mem struct
 * AOinitMode = 1:  connect only to AO shared mem struct
 *  */

long AOloopControl_acquireCalib_Measure_zonalRM(long loop, double ampl, long delayfr, long delayRM1us, long NBave, long NBexcl, const char *zrespm_name, const char *WFSref0_name, const char *WFSmap_name, const char *DMmap_name, long mode, int normalize, int AOinitMode, long NBcycle)
{
    long ID_WFSmap, ID_WFSref0, ID_WFSref2, ID_DMmap, IDmapcube, IDzrespm, IDzrespmn, ID_WFSref0n,  ID_WFSref2n;
    long act, j, ii, kk;
    double value;
    float *arrayf;
    char fname[200];
    char name[200];
    char command[200];
    long IDpos, IDneg;
    float tot, v1, rms;
    uint32_t *sizearray;

    long NBiter = LONG_MAX; // runs until USR1 signal received
    long iter;
    float *arraypix;
    long i;
    long istart, iend, icnt;
    long cntn;
    double tmpv;

    long ID_WFSmask, ID_DMmask;
    float lim;
    double total;
    int r;
    long IDzrespfp, IDzrespfm;
    long IDpokeC;
    long NBpoke;

    int RT_priority = 80; //any number from 0-99
    struct sched_param schedpar;
    int ret;
    long act2;

    long *actarray;
    long poke, poke1, poke2;




    schedpar.sched_priority = RT_priority;
#ifndef __MACH__
    sched_setscheduler(0, SCHED_FIFO, &schedpar);
#endif


    if(NBcycle < 1)
        NBiter = LONG_MAX; // runs until USR1 signal received
    else
        NBiter = NBcycle;



    arraypix = (float*) malloc(sizeof(float)*NBiter);
    sizearray = (uint32_t*) malloc(sizeof(uint32_t)*3);





    printf("INITIALIZE MEMORY (mode %d)....\n", AOinitMode);
    fflush(stdout);



    if(AOloopcontrol_meminit==0)
        AOloopControl_InitializeMemory(AOinitMode);
    fflush(stdout);



    //  sprintf(fname, "./conf/AOloop.conf");

    printf("LOAD/CONFIGURE loop ...\n");
    fflush(stdout);


    AOloopControl_loadconfigure(LOOPNUMBER, 1, 2);



    printf("Importing DM response matrix channel shared memory ...\n");
    fflush(stdout);
    aoloopcontrol_var.aoconfID_dmRM = read_sharedmem_image(AOconf[loop].DMctrl.dmRMname);



    printf("Importing WFS camera image shared memory ... \n");
    fflush(stdout);
    aoloopcontrol_var.aoconfID_wfsim = read_sharedmem_image(AOconf[loop].WFSim.WFSname);



    if(sprintf(name, "aol%ld_imWFS1RM", loop) < 1)
        printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");

    sizearray[0] = AOconf[loop].WFSim.sizexWFS;
    sizearray[1] = AOconf[loop].WFSim.sizeyWFS;
    printf("WFS size = %ld %ld\n", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS);
    fflush(stdout);
    aoloopcontrol_var.aoconfID_imWFS1 = create_image_ID(name, 2, sizearray, _DATATYPE_FLOAT, 1, 0);


    arrayf = (float*) malloc(sizeof(float)*AOconf[loop].DMctrl.sizeDM);

    sizearray[0] = AOconf[loop].DMctrl.sizexDM;
    sizearray[1] = AOconf[loop].DMctrl.sizeyDM;
    ID_DMmap = create_image_ID(DMmap_name, 2, sizearray, _DATATYPE_FLOAT, 1, 5);


    IDpos = create_2Dimage_ID("wfsposim", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS);
    IDneg = create_2Dimage_ID("wfsnegim", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS);



    IDpokeC = image_ID("RMpokeCube");
    if(IDpokeC==-1)
    {
        IDpokeC = create_3Dimage_ID("RMpokeCube", AOconf[loop].DMctrl.sizexDM, AOconf[loop].DMctrl.sizeyDM, AOconf[loop].DMctrl.sizexDM*AOconf[loop].DMctrl.sizeyDM);
        for(act=0; act<AOconf[loop].DMctrl.sizexDM*AOconf[loop].DMctrl.sizeyDM; act++)
        {
            for(ii=0; ii<AOconf[loop].DMctrl.sizexDM*AOconf[loop].DMctrl.sizeyDM; ii++)
                data.image[IDpokeC].array.F[act*AOconf[loop].DMctrl.sizexDM*AOconf[loop].DMctrl.sizeyDM+ii] = 0.0;
            data.image[IDpokeC].array.F[act*AOconf[loop].DMctrl.sizexDM*AOconf[loop].DMctrl.sizeyDM+act] = 1.0;
        }
        //        save_fits("RMpokeCube", "!./conf/RMpokeCube.fits");
        save_fits("RMpokeCube", "!./conf/zRMpokeCube.fits");

        NBpoke = data.image[IDpokeC].md[0].size[2];
    }
    else
    {
        //NBpoke = AOconf[loop].DMctrl.sizeDM;
        NBpoke = data.image[IDpokeC].md[0].size[2];
    }

    //    save_fits("RMpokeCube", "!./conf/test1_RMpokeCube.fits");

    if(sprintf(command, "echo \"%ld\" > RM_NBpoke.txt\n", NBpoke) < 1)
        printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");
    if(system(command) != 0)
        printERROR(__FILE__, __func__, __LINE__, "system() returns non-zero value");

    if(sprintf(command, "echo \"%ld\" > test_RM_NBpoke.txt\n", NBpoke) < 1)
        printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");
    if(system(command) != 0)
        printERROR(__FILE__, __func__, __LINE__, "system() returns non-zero value");

    //    sleep(10);

    sizearray[0] = AOconf[loop].WFSim.sizexWFS;
    sizearray[1] = AOconf[loop].WFSim.sizeyWFS;
    sizearray[2] = NBpoke; //AOconf[loop].DMctrl.sizeDM;

    actarray = (long*) malloc(sizeof(long)*NBpoke);

    ID_WFSmap = create_image_ID(WFSmap_name, 2, sizearray, _DATATYPE_FLOAT, 1, 5);
    ID_WFSref0 = create_image_ID("tmpwfsref0", 2, sizearray, _DATATYPE_FLOAT, 1, 5);
    ID_WFSref2 = create_image_ID("tmpwfsref2", 2, sizearray, _DATATYPE_FLOAT, 1, 5);
    ID_WFSref0n = create_image_ID(WFSref0_name, 2, sizearray, _DATATYPE_FLOAT, 1, 5);
    ID_WFSref2n = create_image_ID("tmpwfsimrms", 2, sizearray, _DATATYPE_FLOAT, 1, 5);
    IDzrespm = create_image_ID("zrespm", 3, sizearray, _DATATYPE_FLOAT, 0, 5); // Zonal response matrix
    IDzrespmn = create_image_ID(zrespm_name, 3, sizearray, _DATATYPE_FLOAT, 0, 5); // Zonal response matrix normalized

    IDzrespfp = create_image_ID("zrespfp", 3, sizearray, _DATATYPE_FLOAT, 0, 5); // positive poke image
    IDzrespfm = create_image_ID("zrespfm", 3, sizearray, _DATATYPE_FLOAT, 0, 5); // negative poke image

    if(mode>0)
    {
        sizearray[0] = AOconf[loop].WFSim.sizexWFS;
        sizearray[1] = AOconf[loop].WFSim.sizeyWFS;
        ID_WFSmask = create_image_ID("wfsmask", 2, sizearray, _DATATYPE_FLOAT, 1, 5);

        sizearray[0] = AOconf[loop].DMctrl.sizexDM;
        sizearray[1] = AOconf[loop].DMctrl.sizeyDM;
        ID_DMmask = create_image_ID("dmmask", 2, sizearray, _DATATYPE_FLOAT, 1, 5);
    }


    cntn = 0;
    iter = 0;


    printf("Clearing directory files\n");
    fflush(stdout);

    //    for(iter=0; iter<NBiter; iter++)
    if(system("mkdir -p zresptmp") != 0)
        printERROR(__FILE__, __func__, __LINE__, "system() returns non-zero value");

    if(system("rm ./zresptmp/LO*.fits") != 0)
        printERROR(__FILE__, __func__, __LINE__, "system() returns non-zero value");

    if(sprintf(command, "echo %ld > ./zresptmp/%s_nbiter.txt", iter, zrespm_name) < 1)
        printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");

    if(system(command) != 0)
        printERROR(__FILE__, __func__, __LINE__, "system() returns non-zero value");


    printf("STARTING RM...\n");
    fflush(stdout);

    while((iter<NBiter)&&(data.signal_USR1==0))
    {
        printf("iteration # %8ld    \n", iter);
        fflush(stdout);


        // permut actarray
        for(poke=0; poke<NBpoke; poke++)
            actarray[poke] = poke;

        for(poke=0; poke<NBpoke; poke++)
        {
            poke1 = (long) (ran1()*NBpoke);
            if(poke1>=NBpoke)
                poke1 = NBpoke-1;
            if(poke!=poke1)
            {
                poke2 = actarray[poke1];
                actarray[poke1] = actarray[poke];
                actarray[poke] = poke2;
            }
        }



        for(poke=0; poke<NBpoke; poke++)
            for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                data.image[IDzrespm].array.F[poke*AOconf[loop].WFSim.sizeWFS+ii] = 0.0;


        act = 0;

        long kk1 = 0;
        int PokeSign = 1;
        long act1 = 0;



        // initialize with first positive poke
        for(j=0; j<AOconf[loop].DMctrl.sizeDM; j++)
            arrayf[j] = ampl*data.image[IDpokeC].array.F[actarray[act1]*AOconf[loop].DMctrl.sizeDM+j];


        usleep(delayRM1us);
        data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 1;
        memcpy (data.image[aoloopcontrol_var.aoconfID_dmRM].array.F, arrayf, sizeof(float)*AOconf[loop].DMctrl.sizeDM);
        data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].cnt0++;
        data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 0;
        AOconf[loop].aorun.DMupdatecnt ++;



        // WAIT FOR LOOP DELAY, PRIMING
        Read_cam_frame(loop, 1, normalize, 0, 1);

        // read delayfr frames
        for(kk=0; kk<delayfr; kk++)
        {
            Read_cam_frame(loop, 1, normalize, 0, 0);
            kk1++;
            if(kk1==NBave)
            {
                kk1 = -NBexcl;
                if(PokeSign==1)
                    PokeSign = -1;
                else
                {
                    act1++;
                    PokeSign = 1;
                }

                if(act1>NBpoke-1)
                    act1 = NBpoke-1;
                // POKE
                for(j=0; j<AOconf[loop].DMctrl.sizeDM; j++)
                    arrayf[j] = ampl*PokeSign*data.image[IDpokeC].array.F[actarray[act1]*AOconf[loop].DMctrl.sizeDM+j];

                usleep(delayRM1us);
                data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 1;
                memcpy (data.image[aoloopcontrol_var.aoconfID_dmRM].array.F, arrayf, sizeof(float)*AOconf[loop].DMctrl.sizeDM);
                data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].cnt0++;
                data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 0;
                AOconf[loop].aorun.DMupdatecnt ++;
            }
        }





        while ((act < NBpoke)&&(data.signal_USR1==0))
        {
            //	printf("act = %6ld   NBpoke = %6ld\n", act, NBpoke);
            //	fflush(stdout);
            for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
            {
                data.image[IDpos].array.F[ii] = 0.0;
                data.image[IDneg].array.F[ii] = 0.0;
            }

            // POSITIVE INTEGRATION
            //  printf("POSITIVE INTEGRATION\n");
            //  fflush(stdout);
            for(kk=0; kk<NBave+NBexcl; kk++)
            {
                Read_cam_frame(loop, 1, normalize, 0, 0);
                if(kk<NBave)
                    for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                        data.image[IDpos].array.F[ii] += data.image[aoloopcontrol_var.aoconfID_imWFS1].array.F[ii];
                kk1++;
                if(kk1==NBave)
                {
                    kk1 = -NBexcl;
                    if(PokeSign==1)
                        PokeSign = -1;
                    else
                    {
                        act1++;
                        PokeSign = 1;
                    }
                    if(act1>NBpoke-1)
                        act1 = NBpoke-1;
                    // POKE
                    for(j=0; j<AOconf[loop].DMctrl.sizeDM; j++)
                        arrayf[j] = ampl*PokeSign*data.image[IDpokeC].array.F[actarray[act1]*AOconf[loop].DMctrl.sizeDM+j];

                    usleep(delayRM1us);
                    data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 1;
                    memcpy (data.image[aoloopcontrol_var.aoconfID_dmRM].array.F, arrayf, sizeof(float)*AOconf[loop].DMctrl.sizeDM);
                    data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].cnt0++;
                    data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 0;
                    AOconf[loop].aorun.DMupdatecnt ++;
                }
            }


            for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
            {
                data.image[IDzrespm].array.F[actarray[act]*AOconf[loop].WFSim.sizeWFS+ii] += data.image[IDpos].array.F[ii];
                data.image[IDzrespfp].array.F[actarray[act]*AOconf[loop].WFSim.sizeWFS+ii] = data.image[IDpos].array.F[ii];
                data.image[ID_WFSref0].array.F[ii] += data.image[IDpos].array.F[ii];
                data.image[ID_WFSref2].array.F[ii] += data.image[IDpos].array.F[ii]*data.image[IDpos].array.F[ii];
            }

            // NEGATIVE INTEGRATION
            //   printf("NEGATIVE INTEGRATION\n");
            //   fflush(stdout);
            for(kk=0; kk<NBave+NBexcl; kk++)
            {
                Read_cam_frame(loop, 1, normalize, 0, 0);
                if(kk<NBave)
                    for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                        data.image[IDneg].array.F[ii] += data.image[aoloopcontrol_var.aoconfID_imWFS1].array.F[ii];
                kk1++;
                if(kk1==NBave)
                {
                    kk1 = -NBexcl;
                    if(PokeSign==1)
                        PokeSign = -1;
                    else
                    {
                        act1++;
                        PokeSign = 1;
                    }
                    if(act1>NBpoke-1)
                        act1 = NBpoke-1;
                    // POKE
                    for(j=0; j<AOconf[loop].DMctrl.sizeDM; j++)
                        arrayf[j] = ampl*PokeSign*data.image[IDpokeC].array.F[actarray[act1]*AOconf[loop].DMctrl.sizeDM+j];

                    usleep(delayRM1us);
                    data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 1;
                    memcpy (data.image[aoloopcontrol_var.aoconfID_dmRM].array.F, arrayf, sizeof(float)*AOconf[loop].DMctrl.sizeDM);
                    data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].cnt0++;
                    data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 0;
                    AOconf[loop].aorun.DMupdatecnt ++;
                }
            }

            for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
            {
                data.image[IDzrespm].array.F[actarray[act]*AOconf[loop].WFSim.sizeWFS+ii] -= data.image[IDneg].array.F[ii];
                data.image[IDzrespfm].array.F[actarray[act]*AOconf[loop].WFSim.sizeWFS+ii] = data.image[IDneg].array.F[ii];
                data.image[ID_WFSref0].array.F[ii] += data.image[IDneg].array.F[ii];
                data.image[ID_WFSref2].array.F[ii] += data.image[IDneg].array.F[ii] * data.image[IDneg].array.F[ii];
            }

            act++;
        }
        cntn = 2*NBave; // Number of images


        for(j=0; j<AOconf[loop].DMctrl.sizeDM; j++)
            arrayf[j] = 0.0;

        usleep(delayRM1us);
        data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 1;
        memcpy (data.image[aoloopcontrol_var.aoconfID_dmRM].array.F, arrayf, sizeof(float)*AOconf[loop].DMctrl.sizeDM);
        data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].cnt0++;
        data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 0;
        AOconf[loop].aorun.DMupdatecnt ++;


        if(data.signal_USR1==0) // keep looping
        {
            for(act=0; act<NBpoke; act++)
                for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                    data.image[IDzrespmn].array.F[act*AOconf[loop].WFSim.sizeWFS+ii] = data.image[IDzrespm].array.F[actarray[act]*AOconf[loop].WFSim.sizeWFS+ii]/ampl/cntn;
            if(sprintf(fname, "!./zresptmp/%s_%03ld.fits", zrespm_name, iter) < 1)
                printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");
            save_fits(zrespm_name, fname);

            for(act=0; act<NBpoke; act++)
                for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                {
                    data.image[IDzrespfp].array.F[act*AOconf[loop].WFSim.sizeWFS+ii] /= NBave;
                    data.image[IDzrespfm].array.F[act*AOconf[loop].WFSim.sizeWFS+ii] /= NBave;
                }

            if(sprintf(fname, "!./zresptmp/%s_pos_%03ld.fits", zrespm_name, iter) < 1)
                printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");

            save_fits("zrespfp", fname);

            if(sprintf(fname, "!./zresptmp/%s_neg_%03ld.fits", zrespm_name, iter) < 1)
                printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");

            save_fits("zrespfm", fname);

            total = 0.0;
            for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
            {
                data.image[ID_WFSref2n].array.F[ii] = sqrt((data.image[ID_WFSref2].array.F[ii] - data.image[ID_WFSref0].array.F[ii]*data.image[ID_WFSref0].array.F[ii])/NBave/cntn);
                data.image[ID_WFSref0n].array.F[ii] = data.image[ID_WFSref0].array.F[ii]/NBave/cntn;
                total += data.image[ID_WFSref0n].array.F[ii];
            }



            if(normalize==1)
            {
                for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                {
                    data.image[ID_WFSref0n].array.F[ii] /= total;
                    data.image[ID_WFSref2n].array.F[ii] /= total;
                }
            }
            else
            {
                for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                {
                    data.image[ID_WFSref0n].array.F[ii] /= NBave;
                    data.image[ID_WFSref2n].array.F[ii] /= NBave;
                }
            }

            if(sprintf(fname, "!./zresptmp/%s_%03ld.fits", WFSref0_name, iter) < 1)
                printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");

            save_fits(WFSref0_name, fname);

            if(sprintf(fname, "!./zresptmp/wfsimRMS.fits") < 1)
                printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");

            save_fits("wfsimrms", fname);


            if(mode!=3)
            {
                for(poke=0; poke<NBpoke; poke++)
                {
                    rms = 0.0;
                    for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                    {
                        tmpv = data.image[IDzrespmn].array.F[poke*AOconf[loop].WFSim.sizeWFS+ii];
                        rms += tmpv*tmpv;
                    }
                    data.image[ID_DMmap].array.F[act] = rms;
                }

                if(sprintf(fname, "!./zresptmp/%s_%03ld.fits", DMmap_name, iter) < 1)
                    printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");

                save_fits(DMmap_name, fname);


                for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                {
                    rms = 0.0;
                    for(poke=0; poke<NBpoke; poke++)
                    {
                        tmpv = data.image[IDzrespmn].array.F[poke*AOconf[loop].WFSim.sizeWFS+ii];
                        rms += tmpv*tmpv;
                    }
                    data.image[ID_WFSmap].array.F[ii] = rms;
                }

                if(sprintf(fname, "!./zresptmp/%s_%03ld.fits", zrespm_name, iter) < 1)
                    printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");

                save_fits(WFSmap_name, fname);

                if(mode>0) // compute WFSmask and DMmask
                {
                    // WFSmask : select pixels >40% of 85-percentile
                    lim = 0.4*img_percentile(WFSmap_name, 0.7);
                    for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                    {
                        if(data.image[ID_WFSmap].array.F[ii]<lim)
                            data.image[ID_WFSmask].array.F[ii] = 0.0;
                        else
                            data.image[ID_WFSmask].array.F[ii] = 1.0;
                    }

                    // DMmask: select pixels >10% of 50-percentile
                    lim = 0.1*img_percentile(DMmap_name, 0.5);
                    for(act=0; act<AOconf[loop].DMctrl.sizeDM; act++)
                    {
                        if(data.image[ID_DMmap].array.F[act]<lim)
                            data.image[ID_DMmask].array.F[act] = 0.0;
                        else
                            data.image[ID_DMmask].array.F[act] = 1.0;
                    }
                }
            }
            iter++;
            if(sprintf(command, "echo %ld > ./zresptmp/%s_nbiter.txt", iter, zrespm_name) < 1)
                printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");

            if(system(command) != 0)
                printERROR(__FILE__, __func__, __LINE__, "system() returns non-zero value");
        }
    } // end of iteration loop

    free(arrayf);
    free(sizearray);
    free(arraypix);

    free(actarray);

    delete_image_ID("tmpwfsref0");


    return(ID_WFSmap);
}






/** measures response matrix AND reference */
// scan delay up to fDelay

int_fast8_t AOloopControl_acquireCalib_Measure_Resp_Matrix(long loop, long NbAve, float amp, long nbloop, long fDelay, long NBiter)
{
    long NBloops;
    long kloop;
    long delayus = 10000; // delay in us
    long ii, i, imax;
    int Verbose = 0;
    long k1, k, k2;
    char fname[200];
    char name0[200];
    char name[200];

    long kk;
    long RespMatNBframes;
    long IDrmc;
    long kc;

    long IDeigenmodes;

    long double RMsig;
    long double RMsigold;
    long kc0;
    FILE *fp;
    long NBexcl = 2; // number of frames excluded between DM mode changes
    long kc0min, kc0max;
    long IDrmtest;
    int vOK;

    long iter;
    long IDrmi;
    float beta = 0.0;
    float gain = 0.0001;
    long IDrmcumul;
    long IDrefi;
    long IDrefcumul;

    uint32_t *sizearray;

    long IDrespM;
    long IDwfsref0;

    long IDoptsignal; // optical signal for each mode, cumulative
    long IDoptsignaln; // optical signal for each mode, normalize
    long IDmcoeff; // multiplicative gain to amplify low-oder modes
    long IDoptcnt;
    double rmsval;
    char signame[200];

    double normcoeff, normcoeffcnt;


    int AdjustAmplitude = 0;
    char command[2000];

    float valave;
    long IDrmc1;



    RMACQUISITION = 1;


    printf("ACQUIRE RESPONSE MATRIX - loop = %ld, NbAve = %ld, amp = %f, nbloop = %ld, fDelay = %ld, NBiter = %ld\n", loop, NbAve, amp, nbloop, fDelay, NBiter);

    sizearray = (uint32_t*) malloc(sizeof(uint32_t)*3);



    if(AOloopcontrol_meminit==0)
        AOloopControl_InitializeMemory(0);
    //   sprintf(fname, "./conf/AOloop.conf");
    AOloopControl_loadconfigure(LOOPNUMBER, 1, 10);


    // create output
    IDwfsref0 = create_2Dimage_ID("refwfsacq", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS);
    IDrespM = create_3Dimage_ID("respmacq", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS, AOconf[loop].AOpmodecoeffs.NBDMmodes);




    IDoptsignal = create_2Dimage_ID("optsig", AOconf[loop].AOpmodecoeffs.NBDMmodes, 1);
    IDoptsignaln = create_2Dimage_ID("optsign", AOconf[loop].AOpmodecoeffs.NBDMmodes, 1);
    IDmcoeff = create_2Dimage_ID("mcoeff", AOconf[loop].AOpmodecoeffs.NBDMmodes, 1);
    IDoptcnt = create_2Dimage_ID("optsigcnt", AOconf[loop].AOpmodecoeffs.NBDMmodes, 1);

    for(k=0; k<AOconf[loop].AOpmodecoeffs.NBDMmodes; k++)
    {
        data.image[IDoptcnt].array.F[k] = 0.0;
        data.image[IDoptsignal].array.F[k] = 0.0;
        data.image[IDoptsignaln].array.F[k] = 0.0;
        data.image[IDmcoeff].array.F[k] = 1.0;
    }


    RespMatNBframes = 2*AOconf[loop].AOpmodecoeffs.NBDMmodes*NbAve;  // *nbloop
    printf("%ld frames total\n", RespMatNBframes);
    fflush(stdout);

    IDrmc = create_3Dimage_ID("RMcube", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS, RespMatNBframes); // this is the main cube





    IDrmi = create_3Dimage_ID("RMiter", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS, AOconf[loop].AOpmodecoeffs.NBDMmodes);    // Response matrix for 1 iteration
    IDrmcumul = create_3Dimage_ID("RMcumul", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS, AOconf[loop].AOpmodecoeffs.NBDMmodes);  // Cumulative Response matrix

    IDrefi = create_2Dimage_ID("REFiter", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS);
    IDrefcumul = create_2Dimage_ID("REFcumul", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS);



    /// local arrays for image acquision
    //	aoloopcontrol_var.aoconfID_wfsim = create_2Dimage_ID("RMwfs", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS);
    aoloopcontrol_var.aoconfID_imWFS0 = create_2Dimage_ID("RMwfs0", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS);
    aoloopcontrol_var.aoconfID_imWFS1 = create_2Dimage_ID("RMwfs1", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS);
    aoloopcontrol_var.aoconfID_imWFS1 = create_2Dimage_ID("RMwfs2", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS);


    aoloopcontrol_var.aoconfID_cmd_modesRM = create_2Dimage_ID("RMmodesloc", AOconf[loop].AOpmodecoeffs.NBDMmodes, 1);


    for(iter=0; iter<NBiter; iter++)
    {
        if (file_exists("stopRM.txt"))
        {
            if(system("rm stopRM.txt") != 0)
                printERROR(__FILE__, __func__, __LINE__, "system() returns non-zero value");

            iter = NBiter;
        }
        else
        {
            NBloops = nbloop;


            // initialize reference to zero
            for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                data.image[IDrefi].array.F[ii] = 0.0;

            for(ii=0; ii<AOconf[loop].WFSim.sizeWFS*RespMatNBframes; ii++)
                data.image[IDrmc].array.F[ii] = 0.0;


            //            printf("\n");
            //            printf("Testing (in measure_resp_matrix function) :,  NBloops = %ld, NBmode = %ld\n",  NBloops, AOconf[loop].AOpmodecoeffs.NBDMmodes);
            //            fflush(stdout);
            //            sleep(1);

            for(k2 = 0; k2 < AOconf[loop].AOpmodecoeffs.NBDMmodes; k2++)
                data.image[aoloopcontrol_var.aoconfID_cmd_modesRM].array.F[k2] = 0.0;



            // set DM to last mode, neg
            k1 = AOconf[loop].AOpmodecoeffs.NBDMmodes-1;
            data.image[aoloopcontrol_var.aoconfID_cmd_modesRM].array.F[k1] = -amp*data.image[IDmcoeff].array.F[k1];
            set_DM_modesRM(loop);


            usleep(delayus);

            for (kloop = 0; kloop < NBloops; kloop++)
            {
                kc = 0;
                if(Verbose)
                {
                    printf("\n Loop %ld / %ld (%f)\n", kloop, NBloops, amp);
                    fflush(stdout);
                }


                for(k1 = 0; k1 < AOconf[loop].AOpmodecoeffs.NBDMmodes; k1++)
                {
                    for(k2 = 0; k2 < AOconf[loop].AOpmodecoeffs.NBDMmodes; k2++)
                        data.image[aoloopcontrol_var.aoconfID_cmd_modesRM].array.F[k2] = 0.0;

                    // positive
                    data.image[aoloopcontrol_var.aoconfID_cmd_modesRM].array.F[k1] = amp*data.image[IDmcoeff].array.F[k1];
                    set_DM_modesRM(loop);




                    for(kk=0; kk<NbAve; kk++)
                    {
                        Read_cam_frame(loop, 1, 1, 0, 0);


                        for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                        {
                            data.image[IDrefi].array.F[ii] += data.image[aoloopcontrol_var.aoconfID_imWFS1].array.F[ii];
                            data.image[IDrmc].array.F[kc*AOconf[loop].WFSim.sizeWFS+ii] += data.image[aoloopcontrol_var.aoconfID_imWFS1].array.F[ii];
                        }
                        kc++;
                    }


                    // negative
                    data.image[aoloopcontrol_var.aoconfID_cmd_modesRM].array.F[k1] = 0.0-amp*data.image[IDmcoeff].array.F[k1];
                    set_DM_modesRM(loop);



                    for(kk=0; kk<NbAve; kk++)
                    {
                        Read_cam_frame(loop, 1, 1, 0, 0);

                        for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                        {
                            data.image[IDrefi].array.F[ii] += data.image[aoloopcontrol_var.aoconfID_imWFS1].array.F[ii];
                            data.image[IDrmc].array.F[kc*AOconf[loop].WFSim.sizeWFS+ii] += data.image[aoloopcontrol_var.aoconfID_imWFS1].array.F[ii];
                        }
                        kc++;
                    }
                }
            }

            for(ii=0; ii<AOconf[loop].WFSim.sizeWFS*RespMatNBframes; ii++)
                data.image[IDrmc].array.F[ii] /= NBloops;


            // set DM to zero
            for(k2 = 0; k2 < AOconf[loop].AOpmodecoeffs.NBDMmodes; k2++)
                data.image[aoloopcontrol_var.aoconfID_cmd_modesRM].array.F[k2] = 0.0;
            set_DM_modesRM(loop);

            for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                data.image[IDrefi].array.F[ii] /= RespMatNBframes*NBloops;



            // SAVE RMCUBE
            //    save_fits("RMcube", "!RMcube.fits");

            // remove average
            if(1)
            {
                IDrmc1 = create_3Dimage_ID("RMcube1", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS, RespMatNBframes); // this is the main cube, average removed

                for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                {
                    valave = 0.0;
                    for(kc=0; kc<RespMatNBframes; kc++)
                        valave += data.image[IDrmc].array.F[kc*AOconf[loop].WFSim.sizeWFS+ii];
                    valave /= RespMatNBframes;
                    for(kc=0; kc<RespMatNBframes; kc++)
                        data.image[IDrmc1].array.F[kc*AOconf[loop].WFSim.sizeWFS+ii] = data.image[IDrmc].array.F[kc*AOconf[loop].WFSim.sizeWFS+ii] - valave;
                }
                save_fits("RMcube1", "!RMcube1.fits");
            }




            IDrmtest = create_3Dimage_ID("rmtest", AOconf[loop].WFSim.sizexWFS, AOconf[loop].WFSim.sizeyWFS, AOconf[loop].AOpmodecoeffs.NBDMmodes);


            kc0 = fDelay;

            // initialize RM to zero
            for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                for(k=0; k<AOconf[loop].AOpmodecoeffs.NBDMmodes; k++)
                    data.image[IDrmtest].array.F[k*AOconf[loop].WFSim.sizeWFS+ii] = 0.0;

            // initialize reference to zero
            kc = kc0;

            for(k1 = 0; k1 < AOconf[loop].AOpmodecoeffs.NBDMmodes; k1++)
            {
                // positive
                kc += NBexcl;
                if(kc > data.image[IDrmc].md[0].size[2]-1)
                    kc -= data.image[IDrmc].md[0].size[2];
                for(kk=NBexcl; kk<NbAve-NBexcl; kk++)
                {
                    for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                    {
                        data.image[IDrmtest].array.F[k1*AOconf[loop].WFSim.sizeWFS+ii] += data.image[IDrmc].array.F[kc*AOconf[loop].WFSim.sizeWFS+ii];
                        //     data.image[IDrmc].array.F[kc*AOconf[loop].WFSim.sizeWFS+ii] += 1.0;
                    }
                    kc++;
                    if(kc > data.image[IDrmc].md[0].size[2]-1)
                        kc -= data.image[IDrmc].md[0].size[2];
                }
                kc+=NBexcl;
                if(kc > data.image[IDrmc].md[0].size[2]-1)
                    kc -= data.image[IDrmc].md[0].size[2];

                // negative
                kc+=NBexcl;
                if(kc > data.image[IDrmc].md[0].size[2]-1)
                    kc -= data.image[IDrmc].md[0].size[2];
                for(kk=NBexcl; kk<NbAve-NBexcl; kk++)
                {
                    for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                    {
                        data.image[IDrmtest].array.F[k1*AOconf[loop].WFSim.sizeWFS+ii] -= data.image[IDrmc].array.F[kc*AOconf[loop].WFSim.sizeWFS+ii];
                        //  data.image[IDrmc].array.F[kc*AOconf[loop].WFSim.sizeWFS+ii] -= 1.0;
                    }
                    kc++;
                    if(kc > data.image[IDrmc].md[0].size[2]-1)
                        kc -= data.image[IDrmc].md[0].size[2];
                }
                kc+=NBexcl;
                if(kc > data.image[IDrmc].md[0].size[2]-1)
                    kc -= data.image[IDrmc].md[0].size[2];
            }

            //  save_fits("RMcube", "!RMcube2.fits");
            //  exit(0);
            for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                for(k1=0; k1<AOconf[loop].AOpmodecoeffs.NBDMmodes; k1++)
                    data.image[IDrmi].array.F[k1*AOconf[loop].WFSim.sizeWFS+ii] = data.image[IDrmtest].array.F[k1*AOconf[loop].WFSim.sizeWFS+ii];


            //        save_fl_fits("rmtest", "!rmtest.fits");
            delete_image_ID("rmtest");




            printf("%ld %ld  %ld  %ld\n", IDrefcumul, IDrmcumul, IDwfsref0, IDrespM);


            beta = (1.0-gain)*beta + gain;
            for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
            {
                data.image[IDrefcumul].array.F[ii] = (1.0-gain)*data.image[IDrefcumul].array.F[ii] + gain*data.image[IDrefi].array.F[ii];

                data.image[IDwfsref0].array.F[ii] = data.image[IDrefcumul].array.F[ii]/beta;



                for(k1=0; k1<AOconf[loop].AOpmodecoeffs.NBDMmodes; k1++)
                {
                    data.image[IDrmcumul].array.F[k1*AOconf[loop].WFSim.sizeWFS+ii] = (1.0-gain)*data.image[IDrmcumul].array.F[k1*AOconf[loop].WFSim.sizeWFS+ii] + gain*data.image[IDrmi].array.F[k1*AOconf[loop].WFSim.sizeWFS+ii];
                    data.image[IDrespM].array.F[k1*AOconf[loop].WFSim.sizeWFS+ii] = data.image[IDrmcumul].array.F[k1*AOconf[loop].WFSim.sizeWFS+ii]/beta;
                }
            }

            for(k1=0; k1<AOconf[loop].AOpmodecoeffs.NBDMmodes; k1++)
            {
                rmsval = 0.0;
                for(ii=0; ii<AOconf[loop].WFSim.sizeWFS; ii++)
                    rmsval += data.image[IDrespM].array.F[k1*AOconf[loop].WFSim.sizeWFS+ii]*data.image[IDrespM].array.F[k1*AOconf[loop].WFSim.sizeWFS+ii];

                data.image[IDoptsignal].array.F[k1] += rmsval;
                data.image[IDoptcnt].array.F[k1] += 1.0;

                data.image[IDoptsignaln].array.F[k1] = data.image[IDoptsignal].array.F[k1]/data.image[IDoptcnt].array.F[k1];
            }
            save_fits("optsignaln","!./tmp/RM_optsign.fits");

            if(sprintf(signame, "./tmp/RM_optsign_%06ld.txt", iter) < 1)
                printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");

            normcoeff = 0.0;
            normcoeffcnt = 0.0;
            for(k1=AOconf[loop].AOpmodecoeffs.NBDMmodes/2; k1<AOconf[loop].AOpmodecoeffs.NBDMmodes; k1++)
            {
                normcoeff += data.image[IDoptsignaln].array.F[k1];
                normcoeffcnt += 1.0;
            }
            normcoeff /= normcoeffcnt;



            if(AdjustAmplitude==1)
                for(k1=0; k1<AOconf[loop].AOpmodecoeffs.NBDMmodes; k1++)
                {
                    data.image[IDmcoeff].array.F[k1] = 0.8*data.image[IDmcoeff].array.F[k1] + 0.2/(data.image[IDoptsignaln].array.F[k1]/normcoeff);
                    if(data.image[IDmcoeff].array.F[k1]>5.0)
                        data.image[IDmcoeff].array.F[k1] = 5.0;
                }

            fp = fopen(signame, "w");
            for(k1=0; k1<AOconf[loop].AOpmodecoeffs.NBDMmodes; k1++)
                fprintf(fp, "%ld  %g  %g  %g\n", k1, data.image[IDoptsignaln].array.F[k1], data.image[IDoptcnt].array.F[k1], data.image[IDmcoeff].array.F[k1]*amp);
            fclose(fp);
            if(system("cp ./tmp/RM_outsign%06ld.txt ./tmp/RM_outsign.txt") != 0)
                printERROR(__FILE__, __func__, __LINE__, "system() returns non-zero value");

            save_fits("refwfsacq", "!./tmp/refwfs.fits");
            save_fits("respmacq", "!./tmp/respM.fits");
        }
    }


    fp = fopen("./tmp/rmparams.txt", "w");
    fprintf(fp, "%5ld       NbAve: number of WFS frames per averaging\n", NbAve);
    fprintf(fp, "%f	        amp: nominal DM amplitude (RMS)\n", amp);
    fprintf(fp, "%ld        iter: number of iterations\n", iter);
    fprintf(fp, "%ld        nbloop: number of loops per iteration\n", nbloop);
    fprintf(fp, "%ld        fDelay: delay number of frames\n", fDelay);
    fclose(fp);



    printf("Done\n");
    free(sizearray);


    return(0);
}






//
// Measure fast Modal response matrix
//
// HardwareLag [s]
//
// ampl [um]
//

long AOloopControl_acquireCalib_RespMatrix_Fast(const char *DMmodes_name, const char *dmRM_name, const char *imWFS_name, long semtrig, float HardwareLag, float loopfrequ, float ampl, const char *outname)
{
    long IDout;
    long IDmodes;
    long IDmodes1; // muplitiples by ampl, + and -
    long IDdmRM;
    long IDwfs;
    long IDbuff;
    long ii, kk;

    long HardwareLag_int;
    float HardwareLag_frac;
    float WFSperiod;

    long NBmodes;
    long dmxsize, dmysize, dmxysize, wfsxsize, wfsysize, wfsxysize;
    long twait;

    int RT_priority = 80; //any number from 0-99
    struct sched_param schedpar;

    char *ptr0;
    long dmframesize;
    long wfsframesize;
    char *ptrs0;
    char *ptrs1;




    WFSperiod = 1.0/loopfrequ;
    HardwareLag_int = (long) (HardwareLag/WFSperiod);
    HardwareLag_frac = HardwareLag - WFSperiod*HardwareLag_int; // [s]

    twait = (long) (1.0e6 * ( (0.5*WFSperiod) - HardwareLag_frac ) );



    IDmodes = image_ID(DMmodes_name);
    dmxsize = data.image[IDmodes].md[0].size[0];
    dmysize = data.image[IDmodes].md[0].size[1];
    NBmodes = data.image[IDmodes].md[0].size[2];
    dmxysize = dmxsize*dmysize;




    IDmodes1 = image_ID("_tmpmodes");
    if(IDmodes1 == -1)
        IDmodes1 = create_3Dimage_ID("_tmpmodes", dmxsize, dmysize, 2*NBmodes);

    for(kk=0; kk<NBmodes; kk++)
    {
        for(ii=0; ii<dmxysize; ii++)
        {
            data.image[IDmodes1].array.F[2*kk*dmxysize+ii] =  ampl * data.image[IDmodes].array.F[kk*dmxysize+ii];
            data.image[IDmodes1].array.F[(2*kk+1)*dmxysize+ii] =  -ampl * data.image[IDmodes].array.F[kk*dmxysize+ii];
        }
    }



    IDdmRM = image_ID(dmRM_name);

    IDwfs = image_ID(imWFS_name);
    wfsxsize = data.image[IDwfs].md[0].size[0];
    wfsysize = data.image[IDwfs].md[0].size[1];
    wfsxysize = wfsxsize*wfsysize;

    IDbuff = image_ID("RMbuff");
    if(IDbuff == -1)
        IDbuff = create_3Dimage_ID("RMbuff", wfsxsize, wfsysize, 2*NBmodes + HardwareLag_int + 1);

    dmframesize = sizeof(float)*dmxysize;
    wfsframesize = sizeof(float)*wfsxysize;

    schedpar.sched_priority = RT_priority;
#ifndef __MACH__
    int r;

    r = seteuid(data.euid); //This goes up to maximum privileges
    sched_setscheduler(0, SCHED_FIFO, &schedpar); //other option is SCHED_RR, might be faster
    r = seteuid(data.ruid);//Go back to normal privileges
#endif

    ptr0 = (char*) data.image[IDmodes1].array.F;
    ptrs0 = (char*) data.image[IDbuff].array.F;

    // flush semaphore
    while(sem_trywait(data.image[IDwfs].semptr[semtrig])==0) {}


    for(kk=0; kk<NBmodes; kk++)
    {
        char *ptr1;

        sem_wait(data.image[IDwfs].semptr[semtrig]);
        ptrs1 = ptrs0 + wfsxysize*(2*kk);
        memcpy(ptrs1, data.image[IDwfs].array.F, wfsframesize);
        usleep(twait);

        // apply positive mode poke
        ptr1 = ptr0 + (2*kk)*dmframesize;
        data.image[IDdmRM].md[0].write = 1;
        memcpy(data.image[IDdmRM].array.F, ptr1, dmframesize);
        COREMOD_MEMORY_image_set_sempost_byID(IDdmRM, -1);
        data.image[IDdmRM].md[0].cnt0++;
        data.image[IDdmRM].md[0].write = 0;



        sem_wait(data.image[IDwfs].semptr[semtrig]);
        ptrs1 = ptrs0 + wfsxysize*(2*kk+1);
        memcpy(ptrs1, data.image[IDwfs].array.F, wfsframesize);
        usleep(twait);

        // apply negative mode poke
        ptr1 = ptr0 + (2*kk+1)*dmframesize;
        data.image[IDdmRM].md[0].write = 1;
        memcpy(data.image[IDdmRM].array.F, ptr1, dmframesize);
        COREMOD_MEMORY_image_set_sempost_byID(IDdmRM, -1);
        data.image[IDdmRM].md[0].cnt0++;
        data.image[IDdmRM].md[0].write = 0;
    }

    for(kk=0; kk<HardwareLag_int + 1; kk++)
    {
        sem_wait(data.image[IDwfs].semptr[semtrig]);
        ptrs1 = ptrs0 + wfsxysize*(2*NBmodes+kk);
        memcpy(ptrs1, data.image[IDwfs].array.F, wfsframesize);
        usleep(twait);

        // apply zero poke
        data.image[IDdmRM].md[0].write = 1;
        memset(data.image[IDdmRM].array.F, 0, dmframesize);
        COREMOD_MEMORY_image_set_sempost_byID(IDdmRM, -1);
        data.image[IDdmRM].md[0].cnt0++;
        data.image[IDdmRM].md[0].write = 0;
    }


    IDout = create_3Dimage_ID(outname, wfsxsize, wfsysize, NBmodes);
    for(kk=0; kk<NBmodes; kk++)
    {
        long buffindex;

        buffindex = 2*kk + HardwareLag_int;
        for(ii=0; ii<wfsxysize; ii++)
        {
            data.image[IDout].array.F[kk*wfsxysize + ii] = ( data.image[IDbuff].array.F[(buffindex)*wfsxysize + ii] - data.image[IDbuff].array.F[(buffindex+1)*wfsxysize + ii] ) / ampl;
        }

    }

    return(IDout);
}


























