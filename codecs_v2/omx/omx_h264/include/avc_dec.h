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
#ifndef AVC_DEC_H_INCLUDED
#define AVC_DEC_H_INCLUDED

#ifndef OMX_Component_h
#include "OMX_Component.h"
#endif

#ifdef SLSI_S5P6442
#else /* SLSI_S5P6442 */
#ifndef _AVCDEC_API_H_
#include "avcdec_api.h"
#endif
#endif /* SLSI_S5P6442 */

#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

#ifdef SLSI_S5P6442
#ifndef __SAMSUNG_SYSLSI_APDEV_MFCLIB_SSBSIPH264DECODE_H__
#include "SsbSipH264Decode.h"
#endif

//#define MFC_FPS	// for MFC' performance test
#ifdef MFC_FPS
#include <sys/time.h>
#endif

#define CROP

#define LOG_NDEBUG 0
#define LOG_TAG "OMXAVC_MFC"
#include <utils/Log.h>

#define AVCD_TIMESTAMP_ARRAY_SIZE 17
#else /* SLSI_S5P6442 */
#define AVC_DEC_TIMESTAMP_ARRAY_SIZE 17

class AVCCleanupObject_OMX
{
        AVCHandle* ipavcHandle;

    public:
        AVCCleanupObject_OMX(AVCHandle* aAvcHandle = NULL)
        {
            ipavcHandle = aAvcHandle;
        }

        //! Use destructor to do all the clean up work
        ~AVCCleanupObject_OMX();
};
#endif /* SLSI_S5P6442 */



class AvcDecoder_OMX
{
    public:
#ifdef SLSI_S5P6442
#ifdef MFC_FPS
        struct timeval  start, stop;
        unsigned int    time;
        int             frame_cnt;
        int             decode_cnt;
        int             need_cnt;

        unsigned int measureTime(struct timeval* start, struct timeval* stop);
#endif
        AvcDecoder_OMX() { iAvcActiveFlag = OMX_FALSE; };
#else /* SLSI_S5P6442 */
        AvcDecoder_OMX()
        {
            CurrInputTimestamp = 0;
            pDpbBuffer = NULL;
            FrameSize = 0;
            iAvcActiveFlag = OMX_FALSE;
            oscl_memset(DisplayTimestampArray, 0, sizeof(OMX_TICKS)*AVC_DEC_TIMESTAMP_ARRAY_SIZE);
        };
#endif /* SLSI_S5P6442 */

        ~AvcDecoder_OMX() { };

#ifdef SLSI_S5P6442
        OMX_ERRORTYPE   AvcDecInit_OMX();
        OMX_ERRORTYPE   AvcDecDeinit_OMX();
        OMX_BOOL        AvcDecodeVideo_OMX(OMX_U8** aOutBuf, OMX_U32* aOutBufSize, OMX_TICKS* aOutTimestamp,
                                           OMX_U8** aInBuf, OMX_U32* aInBufSize, OMX_TICKS* aInTimestamp,
                                           OMX_PARAM_PORTDEFINITIONTYPE* aPortParam,
                                           OMX_S32* iFrameCount, OMX_BOOL aMarkerFlag,
                                           OMX_BOOL *aResizeFlag, OMX_BOOL MultiSliceFlag);
        OMX_BOOL        GetYuv(OMX_U8** aOutBuf, OMX_U32* aOutBufSize, OMX_TICKS* aOutTimestamp);

#else /* SLSI_S5P6442 */
        AVCCleanupObject_OMX*   pCleanObject;
        AVCHandle       AvcHandle;
        AVCDecSPSInfo   SeqInfo;
        uint32          FrameSize;
        uint8*          pDpbBuffer;
        OMX_TICKS       DisplayTimestampArray[AVC_DEC_TIMESTAMP_ARRAY_SIZE];
        OMX_TICKS       CurrInputTimestamp;
        OMX_U32         InputBytesConsumed;
#endif /* SLSI_S5P6442 */
        OMX_BOOL        iAvcActiveFlag;

#ifdef SLSI_S5P6442
    private:
        OMX_BOOL        ResetTimestamp(void);
        OMX_BOOL        AddTimestamp(OMX_TICKS* time);
        OMX_BOOL        GetTimestamp(OMX_TICKS* time);

        int             mfc_create    (void);
        int             mfc_dec_slice (unsigned char* data, unsigned int size, OMX_BOOL MultiSliceFlag);
        unsigned char*  mfc_get_yuv   (unsigned int* out_size);
        int             mfc_flag_video_frame(unsigned char* data, int size);

    private:
        OMX_S32         iDisplay_Width, iDisplay_Height;

        OMX_TICKS       m_time_queue[AVCD_TIMESTAMP_ARRAY_SIZE];
        int             m_time_queue_start;
        int             m_time_queue_end;

        void*           m_mfc_handle;
        unsigned int    m_mfc_buffer_size;
        unsigned char*  m_mfc_buffer_base;
        unsigned char*  m_mfc_buffer_now;
        int             m_mfc_flag_info_out;
        int             m_mfc_flag_create;

#ifdef CROP
	// RainAde : to support Crop
	int			m_mfc_flag_crop;
	unsigned int	m_crop_left_offset, m_crop_right_offset;
	unsigned int	m_crop_top_offset, m_crop_bottom_offset;		
	unsigned char* m_cropped_data_virt;
#endif /*CROP*/

        OMX_U8          delimiter_h264[4];
        OMX_U32         m_min_stride;
        OMX_U32         m_min_sliceheight;
#else /* SLSI_S5P6442 */
        OMX_ERRORTYPE AvcDecInit_OMX();

        OMX_BOOL AvcDecodeVideo_OMX(OMX_U8* aOutBuffer, OMX_U32* aOutputLength,
                                    OMX_U8** aInputBuf, OMX_U32* aInBufSize,
                                    OMX_PARAM_PORTDEFINITIONTYPE* aPortParam,
                                    OMX_S32* iFrameCount, OMX_BOOL aMarkerFlag,
                                    OMX_TICKS* aOutTimestamp,
                                    OMX_BOOL *aResizeFlag);

        OMX_ERRORTYPE AvcDecDeinit_OMX();

        OMX_BOOL InitializeVideoDecode_OMX();

        OMX_BOOL FlushOutput_OMX(OMX_U8* aOutBuffer, OMX_U32* aOutputLength, OMX_TICKS* aOutTimestamp, OMX_S32 OldWidth, OMX_S32 OldHeight);

        AVCDec_Status GetNextFullNAL_OMX(uint8** aNalBuffer, int32* aNalSize, OMX_U8* aInputBuf, OMX_U32* aInBufSize);

        static int32 AllocateBuffer_OMX(void* aUserData, int32 i, uint8** aYuvBuffer);

        static int32 ActivateSPS_OMX(void* aUserData, uint aSizeInMbs, uint aNumBuffers);

        int32 NSAllocateBuffer_OMX(void* aUserData, int32 i, uint8** aYuvBuffer);

        int32 NSActivateSPS_OMX(void* aUserData, uint aSizeInMbs, uint aNumBuffers);

        void ResetDecoder(); // for repositioning
#endif /* SLSI_S5P6442 */

};

typedef class AvcDecoder_OMX AvcDecoder_OMX;

#endif  //#ifndef AVC_DEC_H_INCLUDED

