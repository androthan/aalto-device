#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <dlfcn.h>
#include <semaphore.h>
#include <OMX_TI_Common.h>
#include <LCML_DspCodec.h>
#include "scale.h"
#include <utils/Log.h>

#define USN_DLL_NAME "usn.dll64P"
#define VPP_NODE_DLL "vpp_sn.dll64P"
#define NUM_OF_VPP_BUFFERS (1)

#define ALIGNMENT 4096
#define ALIGN(p)  ((void*)((((unsigned long) p + ALIGNMENT - 1) / ALIGNMENT) * ALIGNMENT))

#define LOG_FUNCTION_NAME      ALOGD("%d: %s() ENTER", __LINE__, __FUNCTION__);
#define LOG_FUNCTION_NAME_EXIT ALOGD("%d: %s() EXIT", __LINE__, __FUNCTION__);

static const struct DSP_UUID COMMON_TI_UUID = {
        0x79A3C8B3, 0x95F2, 0x403F, 0x9A, 0x4B, {
        0xCF, 0x80, 0x57, 0x73, 0x05, 0x41
    }
};

typedef struct GPPToVPPInputFrameStatus {

    /* INPUT FRAME */
      
    /* input size*/
    OMX_U32      ulInWidth;          /*  picture buffer width          */ 
    OMX_U32      ulInHeight;         /*  picture buffer height         */ 
    OMX_U32      ulCInOffset;        /* offset of the C frame in the   *
                                    * buffer (equal to zero if there *
                                    * is no C frame)                 */ 
    
    /* PROCESSING PARAMETERS */
    
    /*    crop           */ 
    OMX_U32      ulInXstart;          /*  Hin active start             */ 
    OMX_U32      ulInXsize;           /*  Hin active width             */ 
    OMX_U32      ulInYstart;          /*  Vin active start             */ 
    OMX_U32      ulInYsize;           /* Vin active height             */ 

    /*   zoom            */ 
    OMX_U32      ulZoomFactor;        /*zooming ratio (/1024)          */ 
    OMX_U32      ulZoomLimit;         /* zooming ratio limit (/1024)   */ 
    OMX_U32      ulZoomSpeed;         /* speed of ratio change         */ 

    /*  stabilisation             */ 
    OMX_U32      ulXoffsetFromCenter16;    /*  add 1/16/th accuracy offset */ 
    OMX_U32      ulYoffsetFromCenter16;    /* add 1/16/th accuracy offset  */ 

    /*  gain and contrast             */ 
    OMX_U32      ulContrastType;      /*    Contrast method            */ 
    OMX_U32      ulVideoGain;         /* gain on video (Y and C)       */ 

    /*  effect             */ 
    OMX_U32      ulFrostedGlassOvly;  /*  Frosted glass effect overlay          */ 
    OMX_U32      ulLightChroma;       /*  Light chrominance process             */ 
    OMX_U32      ulLockedRatio;       /*  keep H/V ratio                        */ 
    OMX_U32      ulMirror;            /*  to mirror the picture                 */ 
    OMX_U32      ulRGBRotation;          /*  0, 90, 180, 270 deg.                  */ 
    OMX_U32      ulYUVRotation;          /*  0, 90, 180, 270 deg.                  */ 
  
#ifndef _55_
    OMX_U32	     eIORange;              /*  Video Color Range Conversion */	
    OMX_U32      ulDithering;           /*  dithering                             */
    OMX_U32      ulOutPitch;		 // OutPitch (bytes)
	OMX_U32      ulAlphaRGB;		 // Global A value of an ARGB output
    
#endif
  
}GPPToVPPInputFrameStatus;


/* OUTPPUT BUFFER */

typedef struct GPPToVPPOutputFrameStatus {

    OMX_U32      ulOutWidth;          /*  RGB/YUV picture buffer width           */ 
    OMX_U32      ulOutHeight;         /*  RGB/YUV picture buffer height          */ 
    OMX_U32      ulCOutOffset;        /*  Offset of the C frame in the buffer (equal to 0 if there is no C frame)             */ 
  
}GPPToVPPOutputFrameStatus;



OMX_HANDLETYPE      pDllHandle;
LCML_DSP_INTERFACE* pLCML;

sem_t   semResized;

//We suspect a problem with semaphores (and possibly all futex-es) so we are going to wait on a pipe
int     pipeResized[2];

#define READ_END    0
#define WRITE_END   1

/* -------------------------------------------------------------------*/
/**
  *  GetLCMLHandle() function will be called to load LCML component 
  *
  * 
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_ErrorUndefined    The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
LCML_DSP_INTERFACE* GetLCMLHandle()
{
    OMX_ERRORTYPE (*fpGetHandle)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    char *error = NULL;
    OMX_ERRORTYPE eError;

#if 1 
    pDllHandle = dlopen("libLCML.so", RTLD_LAZY);
	error = dlerror();
    if (!pDllHandle) {
        ALOGE("dlopen: %s",error);
        goto EXIT;
    }

    fpGetHandle = dlsym (pDllHandle, "GetHandle");
#else

    OMX_ERRORTYPE GetHandle(OMX_HANDLETYPE *hInterface );
    fpGetHandle = GetHandle;

#endif

    eError = (*fpGetHandle)(&pHandle);
    if(eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        ALOGV("eError != OMX_ErrorNone...\n");
        pHandle = NULL;
        goto EXIT;
    }

EXIT:
    return pHandle;
}

OMX_ERRORTYPE Fill_LCMLInitParams(OMX_U16 arr[], LCML_DSP *plcml_Init, int inWidth, int inHeight, int outWidth, int outHeight, int inFmt, int outFmt)
{
    
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf,nIpBufSize,nOpBuf,nOpBufSize;
    char  valueStr[52]; /*Changed length*/                      
    OMX_U32 Input_FrameWidth;    
    OMX_U32 Output_FrameWidth;    
    OMX_U16 OutputRGB_Format;    
    OMX_U16 Input_FrameFormat;    
    OMX_U16 Output_FrameFormat;    
    OMX_U16 Overlay;            
    OMX_U16 Alpha = 0; /*Not implemented at OMX level*/
    OMX_U16 ParamSize = 0;
    char * pcSNArgs = NULL;
    OMX_U8 *pTemp = NULL;
    int index;
    LOG_FUNCTION_NAME
    nIpBuf = 1;
    nIpBufSize = inWidth*inHeight*2;

    nOpBuf = 1;
    nOpBufSize = outWidth*outHeight*2;

    plcml_Init->In_BufInfo.nBuffers      = nIpBuf;
    plcml_Init->In_BufInfo.nSize         = nIpBufSize;
    plcml_Init->In_BufInfo.DataTrMethod  = DMM_METHOD;
    plcml_Init->Out_BufInfo.nBuffers     = nOpBuf;
    plcml_Init->Out_BufInfo.nSize        = nOpBufSize;
    plcml_Init->Out_BufInfo.DataTrMethod = DMM_METHOD;

    plcml_Init->DeviceInfo.TypeofDevice       = 0;
    plcml_Init->DeviceInfo.DspStream          = NULL;
    plcml_Init->NodeInfo.nNumOfDLLs           = 3;
    plcml_Init->NodeInfo.AllUUIDs[0].uuid     = &VPPNODE_TI_UUID;
    strcpy ((char *)plcml_Init->NodeInfo.AllUUIDs[0].DllName, VPP_NODE_DLL);
    plcml_Init->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    plcml_Init->NodeInfo.AllUUIDs[1].uuid     = &VPPNODE_TI_UUID;
    strcpy ((char *)plcml_Init->NodeInfo.AllUUIDs[1].DllName, VPP_NODE_DLL);
    plcml_Init->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    plcml_Init->NodeInfo.AllUUIDs[2].uuid     = (struct DSP_UUID *) &COMMON_TI_UUID;
    strcpy ((char *)plcml_Init->NodeInfo.AllUUIDs[2].DllName, USN_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;
    
    plcml_Init->SegID     = 0;
    plcml_Init->Timeout   = -1;
    plcml_Init->Alignment = 0;
    plcml_Init->Priority = 5;

    plcml_Init->ProfileID = 0;
    /*Main input port */
    arr[0] = 5; /*# of Streams*/
    arr[1] = 0; /*Stream ID*/
    arr[2] = 0; /*Stream based input stream*/
    arr[3] = NUM_OF_VPP_BUFFERS; /*Number of buffers on input stream*/
    /*Overlay input port*/
    arr[4] = 1; /*Stream ID*/
    arr[5] = 0; /*Stream based input stream*/
    arr[6] = NUM_OF_VPP_BUFFERS; /*Number of buffers on input stream*/
    /*RGB output port*/
    arr[7] = 2; /*Stream ID*/
    arr[8] = 0; /*Stream basedoutput stream for RGB data*/
    arr[9] = NUM_OF_VPP_BUFFERS; /*Number of buffers on output stream*/
    /*YUV output port*/
    arr[10] = 3; /*Stream ID*/
    arr[11] = 0; /*Stream based output stream for YUV data*/
    arr[12] = NUM_OF_VPP_BUFFERS; /*Number of buffers on output stream*/
    /*Alpha input port, Not implemented at OMX level*/
    arr[13] = 4; /*Stream ID*/
    arr[14] = 0; /*Stream based input stream*/
    arr[15] = NUM_OF_VPP_BUFFERS; /*Number of buffers on output stream*/


    pcSNArgs = (char *) (arr + 16);


    Input_FrameWidth    = inWidth; //3280;
    Output_FrameWidth   = outWidth; //3280;

    if ( inFmt )
        Input_FrameFormat = VGPOP_E420_IN;
    else
    	Input_FrameFormat   = VGPOP_E422_IN_UY;   //422 interlieved

    OutputRGB_Format    = VGPOP_ERGB_NONE;

    if ( outFmt )
        Output_FrameFormat  = VGPOP_E420_OUT;
    else
    	Output_FrameFormat  = VGPOP_E422_OUT_UY; //422 interlieved
    
    /*for overlay*/
    //Overlay = 0;
    Overlay = 0;

    memset(valueStr, 0, sizeof(valueStr));
//     ALOGV(":%lu:%lu:%u:%u:%u:%d\n",
//     Input_FrameWidth,
//     Output_FrameWidth,
//     OutputRGB_Format,
//     Input_FrameFormat,
//     Output_FrameFormat,
//     Overlay);  
    sprintf(valueStr, ":%lu:%lu:%u:%u:%u:%d:%d\n",
    Input_FrameWidth,
    Output_FrameWidth,
    OutputRGB_Format,
    Input_FrameFormat,
    Output_FrameFormat,
    Overlay,
    Alpha);


    while(valueStr[ParamSize] != '\0'){
        ParamSize++;
    }

    /*Copy VPP parameters */
    pTemp = memcpy(pcSNArgs,valueStr,ParamSize);
    if(pTemp == NULL){
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }

    if ( (ParamSize % 2) != 0) {
        index =(ParamSize+1) >> 1;
    }
    else {
        index = ParamSize >> 1;  /*Divide by 2*/
    }
    index = index + 16;  /*Add 16 to the index in order to point to the correct location*/

    arr[index] = END_OF_CR_PHASE_ARGS;
    plcml_Init->pCrPhArgs = arr;
    LOG_FUNCTION_NAME_EXIT
EXIT:
    return eError;
}

int scale_process(void* inBuffer, int inWidth, int inHeight, void* outBuffer, int outWidth, int outHeight, int rotation, int fmt, float zoom)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 w,h,zfactor;
    double aspect_ratio;
	OMX_S32 bufferLen;

//    GPPToVPPInputFrameStatus    PrevIpFrameStatus;
//   GPPToVPPOutputFrameStatus   PrevOpYUVFrameStatus;

    GPPToVPPInputFrameStatus*   pPrevIpFrameStatus; //= &PrevIpFrameStatus;
    GPPToVPPOutputFrameStatus*  pPrevOpYUVFrameStatus; //= &PrevOpYUVFrameStatus;
	
    OMX_MALLOC_SIZE_DSPALIGN(pPrevIpFrameStatus, sizeof(GPPToVPPInputFrameStatus), OMX_U8);
    OMX_MALLOC_SIZE_DSPALIGN(pPrevOpYUVFrameStatus, sizeof(GPPToVPPOutputFrameStatus), OMX_U8);    
	
    pPrevIpFrameStatus->ulInWidth             = inWidth;
    pPrevIpFrameStatus->ulInHeight            = inHeight;
    pPrevIpFrameStatus->ulCInOffset           = 0; /*w * 220;*/  /* offset of the C frame in the   *
                                                                    * buffer (equal to zero if there *
                                                                    * is no C frame)                 */
    /* crop */
    pPrevIpFrameStatus->ulInXstart            = 136;
    pPrevIpFrameStatus->ulInXsize             = 368; /*176 Default value for StdCompRoleTest */
    pPrevIpFrameStatus->ulInYstart            = 0;          
    pPrevIpFrameStatus->ulInYsize             = inHeight; /* 220 Default value for StdCompRoleTest*/
    
    /* zoom*/
    pPrevIpFrameStatus->ulZoomFactor          = zoom*1024; //(outWidth * 1024) / inWidth;
    pPrevIpFrameStatus->ulZoomLimit           = zoom*1024; //(outWidth * 1024) / inWidth;
    pPrevIpFrameStatus->ulZoomSpeed           = 0;
    
    pPrevIpFrameStatus->ulFrostedGlassOvly    = OMX_FALSE;        
    pPrevIpFrameStatus->ulLightChroma         = OMX_TRUE;          
    pPrevIpFrameStatus->ulLockedRatio         = OMX_FALSE;          
    pPrevIpFrameStatus->ulMirror              = OMX_FALSE;      
    pPrevIpFrameStatus->ulRGBRotation         = 0;    
    pPrevIpFrameStatus->ulYUVRotation         = rotation;
    
    pPrevIpFrameStatus->ulContrastType        = 0;  
    pPrevIpFrameStatus->ulVideoGain           = 64;   /*Video Gain (contrast) in VGPOP ranges from 0 to 127, being 64 = Gain 1 (no contrast)*/
    
    pPrevIpFrameStatus->ulXoffsetFromCenter16 = 0;        
    pPrevIpFrameStatus->ulYoffsetFromCenter16 = 0;        
    pPrevIpFrameStatus->ulOutPitch            = 0;  /*Not supported at OMX level*/
    pPrevIpFrameStatus->ulAlphaRGB            = 0; /*Not supported at OMX level*/
   
    /*Init pComponentPrivate->pPrevOpYUVFrameStatus */
    pPrevOpYUVFrameStatus->ulOutWidth         = outWidth;
    pPrevOpYUVFrameStatus->ulOutHeight        = outHeight;
    if( fmt )
	{
        pPrevOpYUVFrameStatus->ulCOutOffset   = outWidth*outHeight; /*  Offset of the C frame in the buffer */
		bufferLen = (outWidth*outHeight) + ((outWidth*outHeight)>> 1);
	}
    else
	{
    	pPrevOpYUVFrameStatus->ulCOutOffset       = 0;/*  Offset of the C frame in the buffer */
		bufferLen = (outWidth*outHeight)*2;
    }  


//	ALOGV("11111111111111111111111111111111111111111111111\n");
    eError = LCML_QueueBuffer(pLCML->pCodecinterfacehandle,
                              EMMCodecInputBuffer,
                              inBuffer,
                              inWidth * inHeight * 2,
                              inWidth * inHeight * 2,
                              (void*)pPrevIpFrameStatus,
                              sizeof(GPPToVPPInputFrameStatus),
                              NULL);
    if (eError != OMX_ErrorNone) {
        ALOGE("Camera Component: Error 0x%X While sending the input buffer to Codec\n",eError);
        goto OMX_CAMERA_BAIL_CMD;
    }

//	ALOGV("222222222222222222222222222222222222222222222222\n");
    eError = LCML_QueueBuffer(pLCML->pCodecinterfacehandle,
                              EMMCodecStream3,
                              outBuffer,
                              bufferLen,
                              0,
                              (void*)pPrevOpYUVFrameStatus,
                              sizeof(GPPToVPPOutputFrameStatus),  
                              NULL);
    if (eError != OMX_ErrorNone) {
        ALOGE("Camera Component: Error 0x%X While sending the output buffer to Codec\n",eError);
        goto OMX_CAMERA_BAIL_CMD;
    }

//	ALOGV("3333333333333333333333333333333333333333333333333\n");
    sem_wait( &semResized );

    char ch;
    read(pipeResized[READ_END], &ch, 1);

	ALOGV("444444444444444444444444444444444444444444444444444444\n");
    return 0;

    
OMX_CAMERA_BAIL_CMD:
    return -1;
}


OMX_ERRORTYPE ZoomCallback (TUsnCodecEvent event,void * args [10])
{
//    ALOGV("Callback event=%d\n",event);

    if( event == EMMCodecBufferProcessed )
    {
        if( (int)args[0] == EMMCodecInputBuffer )
        {
            ALOGV("\n\nImage processed.\n\n\n");


            sem_post( &semResized );
            
            write(pipeResized[WRITE_END], "Q", 1);
            ALOGV("\n\nImage processed semaphore posted.\n\n\n");            
        }
    }
    return OMX_ErrorNone;
}

int scale_init(int inWidth, int inHeight, int outWidth, int outHeight, int inFmt, int outFmt)
{
    LCML_CALLBACKTYPE   cb;
    OMX_ERRORTYPE       err;
    LCML_DSP*           pLcmlDsp;
    OMX_U16             array[100];

    char p[32] = "damedesuStr";
    LOG_FUNCTION_NAME
    pLCML = GetLCMLHandle();

    if( pLCML == NULL )
    {
        ALOGE("Cannot get LCML handle.\n");
        return -1;
    }

    cb.LCML_Callback = ZoomCallback;

    pLcmlDsp = pLCML->dspCodec;

    err = Fill_LCMLInitParams( array, pLcmlDsp, inWidth, inHeight, outWidth, outHeight, inFmt, outFmt);
    
    err = LCML_InitMMCodec( pLCML->pCodecinterfacehandle, p, &pLCML, (void*)p, &cb );
    if( err != OMX_ErrorNone )
    {
        ALOGE("LCML_InitMMCodec error = 0x%08x", err);
        return -1;
    }

    err = LCML_ControlCodec( pLCML->pCodecinterfacehandle, EMMCodecControlStart, "damedesuStr" );
    if( err != OMX_ErrorNone )
    {
        ALOGE("LCML_ControlCodec(EMMCodecControlStart) error = 0x%08x\n", err);
        return -1;
    }

    sem_init( &semResized, 0 , 0 );
    pipe(pipeResized);
    LOG_FUNCTION_NAME_EXIT
    return err;
}

int scale_deinit()
{
    OMX_ERRORTYPE err;

    err = LCML_ControlCodec( pLCML->pCodecinterfacehandle, MMCodecControlStop, NULL );
    if( err != OMX_ErrorNone )
    {
        ALOGV("LCML_ControlCodec(MMCodecControlStop) error=0x%08x\n", err);
        return -1;
    }

    err = LCML_ControlCodec( pLCML->pCodecinterfacehandle, EMMCodecControlDestroy, NULL );
    if( err != OMX_ErrorNone )
    {
        ALOGV("LCML_ControlCodec(MMCodecControlStop) error=0x%08x\n", err);
        return -1;
    }
    sem_destroy(&semResized);
    
    close(pipeResized[READ_END]);
    close(pipeResized[WRITE_END]);    
	
    return 0;
}