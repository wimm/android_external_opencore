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
#include "oscl_types.h"
#include "avc_dec.h"
#ifdef SLSI_S5P6442
/* Initialization routine */
OMX_ERRORTYPE AvcDecoder_OMX::AvcDecInit_OMX()
{
#ifdef MFC_FPS
	time = 0;
	frame_cnt = 0;
	decode_cnt = 0;
	need_cnt = 0;
#endif

    iDisplay_Width = 0;
    iDisplay_Height = 0;
	iAvcActiveFlag = OMX_FALSE;

	delimiter_h264[0] = 0x00;
	delimiter_h264[1] = 0x00;
	delimiter_h264[2] = 0x00;
	delimiter_h264[3] = 0x01;

	if(ResetTimestamp() == OMX_FALSE)
	{
		LOGE("ResetTimestamp fail\n");
		return OMX_ErrorUndefined;
	}
	
	if(mfc_create() < 0)
	{
		LOGE("mfc_create fail\n");
		return OMX_ErrorUndefined;
	}

    return OMX_ErrorNone;
}

int AvcDecoder_OMX::mfc_create(void)
{
	if(m_mfc_flag_create == 1)
	{
		LOGE("mfc_create aleady done \n");
		return 0;
	}

	m_mfc_handle = SsbSipH264DecodeInit();
	if (m_mfc_handle == NULL)
	{
		LOGE("SsbSipH264DecodeInit Failed.\n");
		return -1;
	}

	m_mfc_buffer_base = (unsigned char*)SsbSipH264DecodeGetInBuf(m_mfc_handle, 0);

	m_mfc_buffer_now    = m_mfc_buffer_base;
	m_mfc_buffer_size   = 0;	
	m_mfc_flag_info_out = 0;

#ifdef CROP
	// RainAde : to support Crop
	m_mfc_flag_crop = 0;
	m_crop_top_offset = 0;
	m_crop_bottom_offset	 = 0;
	m_crop_left_offset = 0;
	m_crop_right_offset = 0;
	m_cropped_data_virt = NULL;
#endif /*CROP*/

	m_mfc_flag_create   = 1;
					
    return 0;
}

/*Decode routine */
/* yj: Added MultiSliceFlag for multi-slice decoding */
OMX_BOOL AvcDecoder_OMX::AvcDecodeVideo_OMX(
							OMX_U8** 	aOutBuf, 
							OMX_U32* 	aOutBufSize,
							OMX_TICKS* 	aOutTimestamp,
        					OMX_U8** 	aInBuf, 
							OMX_U32* 	aInBufSize,
							OMX_TICKS*	aInTimestamp,
        					OMX_PARAM_PORTDEFINITIONTYPE* aPortParam,
        					OMX_S32* 	iFrameCount, 
							OMX_BOOL 	aMarkerFlag, 
							OMX_BOOL 	*aResizeFlag,
							OMX_BOOL	MultiSliceFlag)
{
    OMX_BOOL Status = OMX_TRUE;
	OMX_S32 OldWidth, OldHeight;
	int ret = 0;

	unsigned char*	pNalBuffer	= *aInBuf;
	int 			NalSize		= *aInBufSize;

    OldWidth = aPortParam->format.video.nFrameWidth;
    OldHeight = aPortParam->format.video.nFrameHeight;

	// RainAde for thumbnail
	//*aInBufSize  = 0;
    *aResizeFlag = OMX_FALSE;

#ifdef MFC_FPS
	gettimeofday(&start, NULL);
	decode_cnt++;
#endif

	ret = mfc_dec_slice(pNalBuffer, NalSize, MultiSliceFlag);

	switch(ret)
	{
		case -1 :
		{
			Status = OMX_FALSE;
			LOGE("mfc_dec_slice fail\n");
			break;
		}
		case 0:
		{
#ifdef MFC_FPS
			need_cnt++;
#endif
			Status = OMX_TRUE;
	        // RainAde for thumbnail
			*aInBufSize  = 0;
			break;
		}
		case 1 :
		{
#ifdef MFC_FPS
			gettimeofday(&stop, NULL);
			time += measureTime(&start, &stop);
			frame_cnt++;
#endif

    		if (iAvcActiveFlag == OMX_FALSE || ((iDisplay_Width != OldWidth) || (iDisplay_Height != OldHeight)))
			{
				LOGV("iDisplay_Width : %d\n", (int)iDisplay_Width);
				LOGV("iDisplay_Height: %d\n", (int)iDisplay_Height);

				if(iDisplay_Width > 720)
				{
					Status = OMX_FALSE;
					break;
				}
					
				iAvcActiveFlag = OMX_TRUE;
				aPortParam->format.video.nFrameWidth  = iDisplay_Width;
				aPortParam->format.video.nFrameHeight = iDisplay_Height;
        		aPortParam->format.video.nStride = m_min_stride;
        		aPortParam->format.video.nSliceHeight = m_min_sliceheight;

				// finally, compute the new minimum buffer size.
				// Decoder components always output YUV420 format
        		aPortParam->nBufferSize = (m_min_sliceheight * m_min_stride * 3) >> 1;

				if ((iDisplay_Width != OldWidth) || (iDisplay_Height != OldHeight))
					*aResizeFlag = OMX_TRUE;

				return OMX_TRUE;
			}
				
			if(AddTimestamp(aInTimestamp) == OMX_FALSE)
			{
				LOGE("AddTimestamp fail\n");
			    return OMX_FALSE;
			}
			
			if(GetYuv(aOutBuf, aOutBufSize, aOutTimestamp) == OMX_FALSE)
			{
				Status = OMX_FALSE;
				break;
			}

			(*iFrameCount)++;
							
			Status = OMX_TRUE;
	        // RainAde for thumbnail
			*aInBufSize  = 0;
			break;
		}
		default :
		{
			Status = OMX_FALSE;
			LOGE("UnExpected Operation\n");
			break;
		}
	}

	if(Status == OMX_FALSE)
	{
		*iFrameCount = 0;
		*aOutBufSize = 0;

		return OMX_FALSE;
	}

	return OMX_TRUE;
}

OMX_BOOL AvcDecoder_OMX::GetYuv(OMX_U8** aOutBuf, OMX_U32* aOutBufSize, OMX_TICKS* aOutTimestamp)
{
	unsigned char*  out_data_virt = NULL;
	unsigned int    out_size      = 0;
				
	out_data_virt = mfc_get_yuv(&out_size);
				
#ifdef CROP
	// RainAde : to support Crop
	if(m_mfc_flag_crop == 1)
	{
		int tmp_idx = 0;

		for(tmp_idx = 0; tmp_idx < iDisplay_Height; tmp_idx++)
		{
			memcpy(m_cropped_data_virt + m_min_stride*tmp_idx, 
					out_data_virt + m_min_stride*(m_crop_top_offset+tmp_idx)+m_crop_left_offset, 
					iDisplay_Width);	// Y
		}
		for(tmp_idx = 0; tmp_idx < iDisplay_Height/2; tmp_idx++)
		{
			memcpy(m_cropped_data_virt + m_min_stride*m_min_sliceheight + m_min_stride/2*tmp_idx, 
					out_data_virt        + m_min_stride*m_min_sliceheight + m_min_stride/2*(m_crop_top_offset/2+tmp_idx) + m_crop_left_offset/2, 
					iDisplay_Width/2);	// Cb
			
			memcpy(m_cropped_data_virt + m_min_stride*m_min_sliceheight + m_min_stride*m_min_sliceheight/4 + m_min_stride/2*tmp_idx, 
					out_data_virt       + m_min_stride*m_min_sliceheight + m_min_stride*m_min_sliceheight/4 + m_min_stride/2*(m_crop_top_offset/2+tmp_idx) + m_crop_left_offset/2, 
					iDisplay_Width/2);			
		}

	out_data_virt = m_cropped_data_virt;
		
	}
#endif /*CROP*/

	if(out_data_virt)
	{
		*aOutBuf     = out_data_virt;
		*aOutBufSize = out_size;
				
		if(GetTimestamp(aOutTimestamp) == OMX_FALSE)
			LOGE("GetTimestamp fail\n");
	}
	else
		return OMX_FALSE;

	return OMX_TRUE;
}
				
OMX_BOOL AvcDecoder_OMX::ResetTimestamp(void)
{
	memset(m_time_queue, 0, sizeof(OMX_TICKS)*AVCD_TIMESTAMP_ARRAY_SIZE);

	m_time_queue_start = 0;
	m_time_queue_end   = 0;
	
	return OMX_TRUE;
}

OMX_BOOL AvcDecoder_OMX::AddTimestamp(OMX_TICKS* timeStamp)
{
	m_time_queue[m_time_queue_end] = *timeStamp;

	// RainAde : changed for bug fix -> index should not be over AVCD_TIMESTAMP_ARRAY_SIZE
	if(AVCD_TIMESTAMP_ARRAY_SIZE-1 <= m_time_queue_end)
		m_time_queue_end = 0;
	else
		m_time_queue_end++;

	if(m_time_queue_end == m_time_queue_start)
	{
		LOGE("TIME QUEUE IS OVERFLOWED\n");
		return OMX_FALSE;
	}

	return OMX_TRUE;
}

OMX_BOOL AvcDecoder_OMX::GetTimestamp(OMX_TICKS* timeStamp)
{
	int index = 0;

	if(m_time_queue_end == m_time_queue_start)
	{
		LOGE("TIME QUEUE IS UNDERFLOWED m_time_queue_end : %d / m_time_queue_start : %d\n", m_time_queue_end, m_time_queue_start);
		return OMX_FALSE;
	}
	else
		index = m_time_queue_start;

	// RainAde : changed for bug fix -> index should not be over AVCD_TIMESTAMP_ARRAY_SIZE
	if(AVCD_TIMESTAMP_ARRAY_SIZE-1 <= m_time_queue_start)
		m_time_queue_start = 0;
	else
		m_time_queue_start++;

	// RainAde : changed for bug fix -> data is not delivered properly
	*timeStamp = m_time_queue[index];

	return OMX_TRUE;
}

int AvcDecoder_OMX::mfc_dec_slice(unsigned char* data, unsigned int size, OMX_BOOL MultiSliceFlag)
{
	SSBSIP_H264_STREAM_INFO avcd_info;

// RainAde : added to get crop information (6410 since FW 1.3.E)
	SSBSIP_H264_CROP_INFO crop_info;
	
	int flag_video_frame = 0;

	if(m_mfc_flag_create == 0)
	{
		LOGE("mfc codec not yes created \n");
		return -1;
	}

	flag_video_frame = mfc_flag_video_frame(data, size);
	if(flag_video_frame < 0)
	{
		LOGE("mfc_flag_video_frame error \n");
		return -1;
	}

	// yj: multi-slice
	if(flag_video_frame == 1 && MultiSliceFlag == OMX_TRUE)
		flag_video_frame = 0;

	if(flag_video_frame == 1)
	{
		memcpy(m_mfc_buffer_now, delimiter_h264, 4);
		m_mfc_buffer_now  += 4;
		m_mfc_buffer_size += 4;

		memcpy(m_mfc_buffer_now, data, size);
		m_mfc_buffer_now  += size;
		m_mfc_buffer_size += size;
	
		if(m_mfc_flag_info_out == 0)
		{
			if(SsbSipH264DecodeExe(m_mfc_handle, m_mfc_buffer_size) < 0)
			{
				LOGE("SsbSipH264DecodeExe for GetConfig fail \n");
				return -1;
			}

			if(SsbSipH264DecodeGetConfig(m_mfc_handle, H264_DEC_GETCONF_STREAMINFO, &avcd_info) < 0)
			{
				LOGE("SsbSipH264DecodeGetConfig fail\n");
				return -1;
			}
			
			// RainAde : added to get crop information (6410 since FW 1.3.E)
			if(SsbSipH264DecodeGetConfig(m_mfc_handle, H264_DEC_GETCONF_CROP_INFO, &crop_info) < 0)
			{
				LOGE("SsbSipH264DecodeGetConfig fail\n");
				return -1;
			}

			LOGV("H264_DEC_GETCONF_CROP_INFO(top) = %d", crop_info.top);
			LOGV("H264_DEC_GETCONF_CROP_INFO(bottom) = %d", crop_info.bottom);
			LOGV("H264_DEC_GETCONF_CROP_INFO(left) = %d", crop_info.left);
			LOGV("H264_DEC_GETCONF_CROP_INFO(right) = %d", crop_info.right);

			iDisplay_Width  = crop_info.right - crop_info.left;
			iDisplay_Height = crop_info.bottom - crop_info.top;
			m_min_stride      = avcd_info.buf_width;
			m_min_sliceheight = avcd_info.buf_height;

#ifdef CROP
			// RainAde : to support Crop
			m_crop_top_offset = crop_info.top - 0;
			m_crop_bottom_offset = iDisplay_Height - crop_info.bottom;
			m_crop_left_offset = crop_info.left - 0;
			m_crop_right_offset = iDisplay_Width  - crop_info.right;

                        // memcpy just top/left case
			if(m_crop_top_offset + m_crop_left_offset != 0)
				m_mfc_flag_crop = 1;
			
			if(m_mfc_flag_crop == 1)
				m_cropped_data_virt = (unsigned char*)malloc(sizeof(char)*m_min_stride*m_min_sliceheight);
				
#endif /*CROP*/

			m_mfc_flag_info_out =  1;
		}

		if(SsbSipH264DecodeExe(m_mfc_handle, m_mfc_buffer_size) < 0)
		{
			LOGE("SsbSipH264DecodeExe(Main) fail \n");
			return -1;
		}

		m_mfc_buffer_now  = m_mfc_buffer_base;
		m_mfc_buffer_size = 0;
	}
	else
	{		
		memcpy(m_mfc_buffer_now, delimiter_h264, 4);
		m_mfc_buffer_now  += 4;
		m_mfc_buffer_size += 4;

		memcpy(m_mfc_buffer_now, data, size);
		m_mfc_buffer_now  += size;
		m_mfc_buffer_size += size;

		return 0;
	}
	
	return 1;
}

int AvcDecoder_OMX::mfc_flag_video_frame(unsigned char* data, int size)
{
	int flag_video_frame = 0; // 0: only header  1: I frame include
	int forbidden_zero_bit;

	if (size > 0)
	{
		forbidden_zero_bit = data[0] >> 7;
		if (forbidden_zero_bit != 0)
			return -1;

		if(	   1 == (data[0] & 0x1F)   // NALTYPE_SLICE
			|| 5 == (data[0] & 0x1F))  // NALTYPE_IDR
		{
			flag_video_frame = 1;
		}
		else
			flag_video_frame = 0;
	}

	return flag_video_frame;
}

unsigned char* AvcDecoder_OMX::mfc_get_yuv(unsigned int* out_size)
{
	unsigned char*	pYUVBuf = NULL;	// memory address of YUV420 Frame Buffer
	long	        nYUVLeng = 0;	// size of frame buffer	

	pYUVBuf = (unsigned char *)SsbSipH264DecodeGetOutBuf(m_mfc_handle, &nYUVLeng);

	if(pYUVBuf)
	{
		*out_size = (unsigned int)nYUVLeng;
	}
	else
	{
		*out_size  = 0;
	}

	return pYUVBuf;
}



OMX_ERRORTYPE AvcDecoder_OMX::AvcDecDeinit_OMX()
{
#ifdef MFC_FPS
	LOGV("[[ MFC_FPS ]] Decoding Time: %u, Frame Count: %d, Need Count: %d, FPS: %f\n", time, frame_cnt-1, need_cnt, (float)(frame_cnt-1)*1000/time);
#endif

	if(m_mfc_flag_create == 0)
	{
		return OMX_ErrorNone;
	}

	if(SsbSipH264DecodeDeInit(m_mfc_handle) < 0)
	{
		LOGE("SsbSipH264DecodeDeInit\n");
		return OMX_ErrorUndefined;
	}

	iAvcActiveFlag      = OMX_FALSE;
	m_mfc_buffer_base   = NULL;
	m_mfc_buffer_now    = NULL;
	m_mfc_buffer_size   = 0;
	m_mfc_flag_info_out = 0;

#ifdef CROP
	if((m_mfc_flag_crop == 1) && (m_cropped_data_virt != NULL))
	{
		free(m_cropped_data_virt);
	}		
	
	m_mfc_flag_crop = 0;
	m_crop_top_offset = 0;
	m_crop_bottom_offset = 0;
	m_crop_left_offset = 0;
	m_crop_right_offset = 0;
	m_cropped_data_virt = NULL;
#endif /*CROP*/

	m_mfc_flag_create   = 0;

    return OMX_ErrorNone;
}

#ifdef MFC_FPS
unsigned int AvcDecoder_OMX::measureTime(struct timeval *start, struct timeval *stop)
{
	unsigned int sec, usec, time;

	sec = stop->tv_sec - start->tv_sec;
	if(stop->tv_usec >= start->tv_usec)
	{
		usec = stop->tv_usec - start->tv_usec;
	}
   	else
	{	
		usec = stop->tv_usec + 1000000 - start->tv_usec;
		sec--;
  	}
	time = sec*1000 + ((double)usec)/1000;
	return time;
}
#endif
#else /* SLSI_S5P6442 */
#include "avcdec_int.h"


/*************************************/
/* functions needed for video engine */
/*************************************/

/* These two functions are for callback functions of AvcHandle */
int32 CBAVC_Malloc_OMX(void* aUserData, int32 aSize, int32 aAttribute)
{
    OSCL_UNUSED_ARG(aUserData);
    OSCL_UNUSED_ARG(aAttribute);
    void* pPtr;

    pPtr = oscl_malloc(aSize);
    return (int32) pPtr;
}

void CBAVC_Free_OMX(void* aUserData, int32 aMem)
{
    OSCL_UNUSED_ARG(aUserData);
    oscl_free((uint8*) aMem);
}


AVCDec_Status CBAVCDec_GetData_OMX(void* aUserData, uint8** aBuffer, uint* aSize)
{
    OSCL_UNUSED_ARG(aUserData);
    OSCL_UNUSED_ARG(aBuffer);
    OSCL_UNUSED_ARG(aSize);
    return AVCDEC_FAIL;  /* nothing for now */
}

int32 AvcDecoder_OMX::AllocateBuffer_OMX(void* aUserData, int32 i, uint8** aYuvBuffer)
{
    AvcDecoder_OMX* pAvcDecoder_OMX = (AvcDecoder_OMX*)aUserData;

    if (NULL == pAvcDecoder_OMX)
    {
        return 0;
    }

    *aYuvBuffer = pAvcDecoder_OMX->pDpbBuffer + i * pAvcDecoder_OMX->FrameSize;
    //Store the input timestamp at the correct index
    pAvcDecoder_OMX->DisplayTimestampArray[i] = pAvcDecoder_OMX->CurrInputTimestamp;
    return 1;
}


void UnbindBuffer_OMX(void* aUserData, int32 i)
{
    OSCL_UNUSED_ARG(aUserData);
    OSCL_UNUSED_ARG(i);
    return;
}

int32 AvcDecoder_OMX::ActivateSPS_OMX(void* aUserData, uint aSizeInMbs, uint aNumBuffers)
{
    AvcDecoder_OMX* pAvcDecoder_OMX = (AvcDecoder_OMX*)aUserData;

    if (NULL == pAvcDecoder_OMX)
    {
        return 0;
    }

    PVAVCDecGetSeqInfo(&(pAvcDecoder_OMX->AvcHandle), &(pAvcDecoder_OMX->SeqInfo));

    if (pAvcDecoder_OMX->pDpbBuffer)
    {
        oscl_free(pAvcDecoder_OMX->pDpbBuffer);
        pAvcDecoder_OMX->pDpbBuffer = NULL;
    }

    pAvcDecoder_OMX->FrameSize = (aSizeInMbs << 7) * 3;
    pAvcDecoder_OMX->pDpbBuffer = (uint8*) oscl_malloc(aNumBuffers * (pAvcDecoder_OMX->FrameSize));

    return 1;
}

/* initialize video decoder */
OMX_BOOL AvcDecoder_OMX::InitializeVideoDecode_OMX()
{
    /* Initialize AvcHandle */
    AvcHandle.AVCObject = NULL;
    AvcHandle.userData = (void*)this;
    AvcHandle.CBAVC_DPBAlloc = ActivateSPS_OMX;
    AvcHandle.CBAVC_FrameBind = AllocateBuffer_OMX;
    AvcHandle.CBAVC_FrameUnbind = UnbindBuffer_OMX;
    AvcHandle.CBAVC_Malloc = CBAVC_Malloc_OMX;
    AvcHandle.CBAVC_Free = CBAVC_Free_OMX;

    return OMX_TRUE;
}

OMX_BOOL AvcDecoder_OMX::FlushOutput_OMX(OMX_U8* aOutBuffer, OMX_U32* aOutputLength, OMX_TICKS* aOutTimestamp, OMX_S32 OldWidth, OMX_S32 OldHeight)
{
    AVCFrameIO Output;
    AVCDec_Status Status;
    int32 Index, Release, FrameSize;
    OMX_S32 OldFrameSize = ((OldWidth + 15) & (~15)) * ((OldHeight + 15) & (~15));

    Output.YCbCr[0] = Output.YCbCr[1] = Output.YCbCr[2] = NULL;
    Status = PVAVCDecGetOutput(&(AvcHandle), &Index, &Release, &Output);

    if (Status == AVCDEC_FAIL)
    {
        return OMX_FALSE;
    }

    *aOutTimestamp = DisplayTimestampArray[Index];
    *aOutputLength = 0; // init to 0

    if (Output.YCbCr[0])
    {
        FrameSize = Output.pitch * Output.height;
        // it should not happen that the frame size is smaller than available buffer size, but check just in case
        if (FrameSize <= OldFrameSize)
        {
            *aOutputLength = (Output.pitch * Output.height * 3) >> 1;

            oscl_memcpy(aOutBuffer, Output.YCbCr[0], FrameSize);
            oscl_memcpy(aOutBuffer + FrameSize, Output.YCbCr[1], FrameSize >> 2);
            oscl_memcpy(aOutBuffer + FrameSize + FrameSize / 4, Output.YCbCr[2], FrameSize >> 2);
        }
        // else, the frame length is reported as zero, and there is no copying
    }


    return OMX_TRUE;
}


/* Initialization routine */
OMX_ERRORTYPE AvcDecoder_OMX::AvcDecInit_OMX()
{
    if (OMX_FALSE == InitializeVideoDecode_OMX())
    {
        return OMX_ErrorInsufficientResources;
    }

    //Set up the cleanup object in order to do clean up work automatically
    pCleanObject = OSCL_NEW(AVCCleanupObject_OMX, (&AvcHandle));

    iAvcActiveFlag = OMX_FALSE;

    return OMX_ErrorNone;
}


/*Decode routine */
OMX_BOOL AvcDecoder_OMX::AvcDecodeVideo_OMX(OMX_U8* aOutBuffer, OMX_U32* aOutputLength,
        OMX_U8** aInputBuf, OMX_U32* aInBufSize,
        OMX_PARAM_PORTDEFINITIONTYPE* aPortParam,
        OMX_S32* iFrameCount, OMX_BOOL aMarkerFlag, OMX_TICKS* aOutTimestamp, OMX_BOOL *aResizeFlag)
{
    AVCDec_Status Status;
    OMX_S32 Width, Height;
    OMX_S32 crop_top, crop_bottom, crop_right, crop_left;
    uint8* pNalBuffer;
    int32 NalSize, NalType, NalRefId;
    //int32 PicType;
    AVCDecObject* pDecVid;

    *aResizeFlag = OMX_FALSE;
    OMX_U32 OldWidth, OldHeight;

    OldWidth =  aPortParam->format.video.nFrameWidth;
    OldHeight = aPortParam->format.video.nFrameHeight;


    if (!aMarkerFlag)
    {
        if (AVCDEC_FAIL == GetNextFullNAL_OMX(&pNalBuffer, &NalSize, *aInputBuf, aInBufSize))
        {
            Status = (AVCDec_Status) FlushOutput_OMX(aOutBuffer, aOutputLength, aOutTimestamp, OldWidth, OldHeight);

            if (AVCDEC_FAIL != Status)
            {
                return OMX_TRUE;
            }
            else
            {
                return OMX_FALSE;
            }
        }
    }
    else
    {
        pNalBuffer = *aInputBuf;
        NalSize = *aInBufSize;
        //Assuming that the buffer with marker bit contains one full NAL
        *aInBufSize = 0;
    }

    if (AVCDEC_FAIL == PVAVCDecGetNALType(pNalBuffer, NalSize, &NalType, &NalRefId))
    {
        return OMX_FALSE;
    }

    if (AVC_NALTYPE_SPS == (AVCNalUnitType)NalType)
    {
        if (PVAVCDecSeqParamSet(&(AvcHandle), pNalBuffer, NalSize) != AVCDEC_SUCCESS)
        {
            return OMX_FALSE;
        }

        pDecVid = (AVCDecObject*) AvcHandle.AVCObject;

        Width = (pDecVid->seqParams[0]->pic_width_in_mbs_minus1 + 1) * 16;
        Height = (pDecVid->seqParams[0]->pic_height_in_map_units_minus1 + 1) * 16;

        if (pDecVid->seqParams[0]->frame_cropping_flag)
        {
            crop_left = 2 * pDecVid->seqParams[0]->frame_crop_left_offset;
            crop_right = Width - (2 * pDecVid->seqParams[0]->frame_crop_right_offset + 1);

            if (pDecVid->seqParams[0]->frame_mbs_only_flag)
            {
                crop_top = 2 * pDecVid->seqParams[0]->frame_crop_top_offset;
                crop_bottom = Height - (2 * pDecVid->seqParams[0]->frame_crop_bottom_offset + 1);
            }
            else
            {
                crop_top = 4 * pDecVid->seqParams[0]->frame_crop_top_offset;
                crop_bottom = Height - (4 * pDecVid->seqParams[0]->frame_crop_bottom_offset + 1);
            }
        }
        else  /* no cropping flag, just give the first and last pixel */
        {
            crop_bottom = Height - 1;
            crop_right = Width - 1;
            crop_top = crop_left = 0;
        }

        aPortParam->format.video.nFrameWidth = crop_right - crop_left + 1;
        aPortParam->format.video.nFrameHeight = crop_bottom - crop_top + 1;

        OMX_U32 min_stride = ((aPortParam->format.video.nFrameWidth + 15) & (~15));
        OMX_U32 min_sliceheight = ((aPortParam->format.video.nFrameHeight + 15) & (~15));


        aPortParam->format.video.nStride = min_stride;
        aPortParam->format.video.nSliceHeight = min_sliceheight;


        // finally, compute the new minimum buffer size.

        // Decoder components always output YUV420 format
        aPortParam->nBufferSize = (aPortParam->format.video.nSliceHeight * aPortParam->format.video.nStride * 3) >> 1;


        if ((OldWidth != aPortParam->format.video.nFrameWidth) || (OldHeight !=  aPortParam->format.video.nFrameHeight))
            *aResizeFlag = OMX_TRUE;

        (*iFrameCount)++;

    }

    else if (AVC_NALTYPE_PPS == (AVCNalUnitType) NalType)
    {
        if (PVAVCDecPicParamSet(&(AvcHandle), pNalBuffer, NalSize) != AVCDEC_SUCCESS)
        {
            return OMX_FALSE;
        }
    }

    else if (AVC_NALTYPE_SLICE == (AVCNalUnitType) NalType ||
             AVC_NALTYPE_IDR == (AVCNalUnitType) NalType)
    {
		if (!iAvcActiveFlag)
			iAvcActiveFlag = OMX_TRUE;

        if ((Status = PVAVCDecodeSlice(&(AvcHandle), pNalBuffer, NalSize)) == AVCDEC_PICTURE_OUTPUT_READY)
        {
            FlushOutput_OMX(aOutBuffer, aOutputLength, aOutTimestamp, OldWidth, OldHeight);

            //Input buffer not consumed yet, do not mark it free.
            if (aMarkerFlag)
            {
                *aInBufSize = NalSize;
            }
            else
            {
                *aInBufSize += InputBytesConsumed;
                aInputBuf -= InputBytesConsumed;
            }
        }

        if (Status == AVCDEC_PICTURE_READY)
        {
            (*iFrameCount)++;
        }

        if ((AVCDEC_NO_DATA == Status) || (AVCDEC_PACKET_LOSS == Status) ||
                (AVCDEC_NO_BUFFER == Status) || (AVCDEC_MEMORY_FAIL == Status) ||
                (AVCDEC_FAIL == Status))
        {
            return OMX_FALSE;
        }
    }

    else if ((AVCNalUnitType)NalType == AVC_NALTYPE_SEI)
    {
        if (PVAVCDecSEI(&(AvcHandle), pNalBuffer, NalSize) != AVCDEC_SUCCESS)
        {
            return OMX_FALSE;
        }
    }

    else if ((AVCNalUnitType)NalType == AVC_NALTYPE_AUD)
    {
        //PicType = pNalBuffer[1] >> 5;
    }

    else if ((AVCNalUnitType)NalType == AVC_NALTYPE_EOSTREAM || // end of stream
             (AVCNalUnitType)NalType == AVC_NALTYPE_EOSEQ || // end of sequence
             (AVCNalUnitType)NalType == AVC_NALTYPE_FILL) // filler data
    {
        return OMX_TRUE;
    }

    //else
    //{
    //printf("\nNAL_type = %d, unsupported nal type or not sure what to do for this type\n", NalType);
    //  return OMX_FALSE;
    //}
    return OMX_TRUE;

}


OMX_ERRORTYPE AvcDecoder_OMX::AvcDecDeinit_OMX()
{
    if (pCleanObject)
    {
        OSCL_DELETE(pCleanObject);
        pCleanObject = NULL;
    }

    if (pDpbBuffer)
    {
        oscl_free(pDpbBuffer);
        pDpbBuffer = NULL;
    }

    return OMX_ErrorNone;
}


AVCDec_Status AvcDecoder_OMX::GetNextFullNAL_OMX(uint8** aNalBuffer, int32* aNalSize, OMX_U8* aInputBuf, OMX_U32* aInBufSize)
{
    uint8* pBuff = aInputBuf;
    OMX_U32 InputSize;

    *aNalSize = *aInBufSize;
    InputSize = *aInBufSize;

    AVCDec_Status ret_val = PVAVCAnnexBGetNALUnit(pBuff, aNalBuffer, aNalSize);

    if (ret_val == AVCDEC_FAIL)
    {
        return AVCDEC_FAIL;
    }

    InputBytesConsumed = ((*aNalSize) + (int32)(*aNalBuffer - pBuff));
    aInputBuf += InputBytesConsumed;
    *aInBufSize = InputSize - InputBytesConsumed;

    return AVCDEC_SUCCESS;
}

AVCCleanupObject_OMX::~AVCCleanupObject_OMX()
{
    PVAVCCleanUpDecoder(ipavcHandle);

}

void AvcDecoder_OMX::ResetDecoder()
{
    PVAVCDecReset(&(AvcHandle));
}
#endif /* SLSI_S5P6442 */

