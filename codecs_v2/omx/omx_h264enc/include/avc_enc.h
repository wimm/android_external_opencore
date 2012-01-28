/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */
#ifndef AVC_ENC_H_INCLUDED
#define AVC_ENC_H_INCLUDED


#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

#ifndef OMX_Component_h
#include "OMX_Component.h"
#endif

#ifdef SLSI_S5P6442
#ifndef __SAMSUNG_SYSLSI_APDEV_MFCLIB_SSBSIPH264ENCODE_H__
#include "SsbSipH264Encode.h"
#endif
#else /* SLSI_S5P6442 */
#ifndef AVCENC_API_H_INCLUDED
#include "avcenc_api.h"
#endif
#endif /* SLSI_S5P6442 */

#ifdef SLSI_S5P6442
// RainAde for Encoding pic type
enum
{
    PIC_TYPE_INTRA = 0,
    PIC_TYPE_INTER
};
#else /* SLSI_S5P6442 */
#ifndef CCRGB24TOYUV420_H_INCLUDED
#include "ccrgb24toyuv420.h"
#endif

#ifndef CCRGB12TOYUV420_H_INCLUDED
#include "ccrgb12toyuv420.h"
#endif

#ifndef CCYUV420SEMITOYUV420_H_INCLUDED
#include "ccyuv420semitoyuv420.h"
#endif

#ifndef OSCL_INT64_UTILS_H_INCLUDED
#include "oscl_int64_utils.h"
#endif
#endif /* SLSI_S5P6442 */

class AvcEncoder_OMX
{
    public:

        AvcEncoder_OMX();
        ~AvcEncoder_OMX();

        OMX_ERRORTYPE AvcEncInit(OMX_VIDEO_PORTDEFINITIONTYPE aInputParam,
                                 OMX_CONFIG_ROTATIONTYPE aInputOrientationType,
                                 OMX_VIDEO_PORTDEFINITIONTYPE aEncodeParam,
                                 OMX_VIDEO_PARAM_AVCTYPE aEncodeAvcParam,
                                 OMX_VIDEO_PARAM_BITRATETYPE aRateControlType,
                                 OMX_VIDEO_PARAM_QUANTIZATIONTYPE aQuantType,
                                 OMX_VIDEO_PARAM_MOTIONVECTORTYPE aSearchRange,
                                 OMX_VIDEO_PARAM_INTRAREFRESHTYPE aIntraRefresh,
                                 OMX_VIDEO_PARAM_VBSMCTYPE aVbsmcType);


#ifdef SLSI_S5P6442
        OMX_BOOL AvcEncodeVideo(OMX_U8*    aOutBuffer,
#else /* SLSI_S5P6442 */
        AVCEnc_Status AvcEncodeVideo(OMX_U8*    aOutBuffer,
#endif /* SLSI_S5P6442 */
                                     OMX_U32*   aOutputLength,
                                     OMX_BOOL*  aBufferOverRun,
                                     OMX_U8**   aOverBufferPointer,
                                     OMX_U8*    aInBuffer,
                                     OMX_U32*   aInBufSize,
                                     OMX_TICKS  aInTimeStamp,
                                     OMX_TICKS* aOutTimeStamp,
                                     OMX_BOOL*  aSyncFlag);

#ifdef SLSI_S5P6442
        OMX_BOOL AvcEncodeSendInput(OMX_U8*    aInBuffer,
                                    OMX_U32*   aInBufSize,
                                    OMX_TICKS  aInTimeStamp);
#else /* SLSI_S5P6442 */
        AVCEnc_Status AvcEncodeSendInput(OMX_U8*    aInBuffer,
                                         OMX_U32*   aInBufSize,
                                         OMX_TICKS  aInTimeStamp);
#endif /* SLSI_S5P6442 */


        OMX_ERRORTYPE AvcEncDeinit();

        OMX_ERRORTYPE AvcRequestIFrame();
        OMX_BOOL AvcUpdateBitRate(OMX_U32 aEncodedBitRate);
        OMX_BOOL AvcUpdateFrameRate(OMX_U32 aEncodeFramerate);
#ifdef SLSI_S5P6442
#else /* SLSI_S5P6442 */
        OMX_BOOL GetSpsPpsHeaderFlag();

        /* for avc encoder lib callback functions */
        int     AVC_DPBAlloc(uint frame_size_in_mbs, uint num_buffers);
        int     AVC_FrameBind(int indx, uint8** yuv);
        void    AVC_FrameUnbind(int indx);


#endif /* SLSI_S5P6442 */

    private:

#ifdef SLSI_S5P6442
	// RainAde : for MFC(avc) encoder
	int m_avcenc_create_flag;
	void *m_avcenc_handle;
	unsigned char * m_avcenc_buffer;

	// RainAde : for composer interface
	int frame_cnt;
	int hdr_size;
	int sps;
	int pps;

#else /* SLSI_S5P6442 */
        void CopyToYUVIn(uint8* YUV, int width, int height, int width_16, int height_16);

        /* RGB->YUV conversion */
        ColorConvertBase *ccRGBtoYUV;

        int     iSrcWidth;
        int     iSrcHeight;
        int     iFrameOrientation;

        OMX_COLOR_FORMATTYPE    iVideoFormat;

        /* variables needed in operation */
        AVCHandle iAvcHandle;
        AVCFrameIO iVidIn;
        uint8*  iYUVIn;
        uint8*  iVideoIn;
        uint8*  iVideoOut;
        uint32  iTimeStamp;
        OMX_TICKS iTimeStamp64;
        OMX_BOOL    iIDR;
        int     iDispOrd;

        uint8*  iDPB;
        bool*   iFrameUsed;
        uint8** iFramePtr;
        int     iNumFrames;

        OMX_BOOL  iInitialized;
        OMX_BOOL  iSpsPpsHeaderFlag;
        OMX_BOOL  iReadyForNextFrame;


#endif /* SLSI_S5P6442 */
};


#endif ///#ifndef AVC_ENC_H_INCLUDED
