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

#include "mpeg4_enc.h"
#include "oscl_mem.h"

#ifdef SLSI_S5P6442

//#define LOG_NDEBUG 0
#define LOG_TAG "Mfc_Mpeg4_enc"
#include <utils/Log.h>

//giridhar: defining max width and height for recording
#define MPEG4_MAX_HEIGHT	480
#define MPEG4_MAX_WIDTH		640
unsigned char *cb_cr_mem;
unsigned char *cb_ptr;
unsigned char *cr_ptr;

Mpeg4Encoder_OMX::Mpeg4Encoder_OMX()
{
	cb_cr_mem = (unsigned char *)(new char[MPEG4_MAX_HEIGHT * MPEG4_MAX_WIDTH / 2]);

};

Mpeg4Encoder_OMX::~Mpeg4Encoder_OMX()
{
	delete cb_cr_mem;
};


/* Initialization routine */
OMX_ERRORTYPE Mpeg4Encoder_OMX::Mp4EncInit(OMX_S32 iEncMode,
        OMX_VIDEO_PORTDEFINITIONTYPE aInputParam,
        OMX_CONFIG_ROTATIONTYPE aInputOrientationType,
        OMX_VIDEO_PORTDEFINITIONTYPE aEncodeParam,
        OMX_VIDEO_PARAM_MPEG4TYPE aEncodeMpeg4Param,
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE aErrorCorrection,
        OMX_VIDEO_PARAM_BITRATETYPE aRateControlType,
        OMX_VIDEO_PARAM_QUANTIZATIONTYPE aQuantType,
        OMX_VIDEO_PARAM_MOTIONVECTORTYPE aSearchRange,
        OMX_VIDEO_PARAM_INTRAREFRESHTYPE aIntraRefresh,
        OMX_VIDEO_PARAM_H263TYPE aH263Type,
        OMX_VIDEO_PARAM_PROFILELEVELTYPE* aProfileLevel)
{
	LOGV("MFC: Mpeg4Encoder_OMX::Mp4EncInit() \n");

	frame_cnt = 0;
	hdr_size = 0;

	int ret = 0;
	int slices[2];

	unsigned char *p_output_buffer = NULL;
	long encoded_size = 0;
	int gop_num = 0;
	
	gop_num = aEncodeParam.xFramerate >> 16;	// temporal value by RainAde : I frame every 1sec

	if(m_mpeg4enc_create_flag == 1)
	{
		LOGV("s5p6442_mpeg4enc_create == 1 \n");
		return OMX_ErrorUndefined;
	}

	if(iEncMode == 0)	// MODE_H263
	{
		LOGV("MODE_H263] nFrameWidth=%d, nFrameHeight=%d, Framerate=%d, nBitrate=%d, gop_num=%d", aEncodeParam.nFrameWidth, aEncodeParam.nFrameHeight, aEncodeParam.xFramerate >> 16, aEncodeParam.nBitrate/1000, gop_num);
		
		m_mpeg4enc_handle = SsbSipMPEG4EncodeInit(SSBSIPMFCENC_H263, aEncodeParam.nFrameWidth, aEncodeParam.nFrameHeight, aEncodeParam.xFramerate >> 16, aEncodeParam.nBitrate/1000, gop_num);		
		//slices[0] = ANNEX_K_OFF | ANNEX_T_OFF;
		slices[0] = ANNEX_K_OFF | ANNEX_T_OFF | ANNEX_J_ON;	// yj: enable deblocking-filter
		SsbSipMPEG4EncodeSetConfig(m_mpeg4enc_handle, MPEG4_ENC_SETCONF_H263_ANNEX, slices);
	}
	else
	{
		LOGV("MODE_MPEG4] nFrameWidth=%d, nFrameHeight=%d, Framerate=%d, nBitrate=%d, gop_num=%d", aEncodeParam.nFrameWidth, aEncodeParam.nFrameHeight, aEncodeParam.xFramerate >> 16, aEncodeParam.nBitrate/1000, gop_num);

		m_mpeg4enc_handle = SsbSipMPEG4EncodeInit(SSBSIPMFCENC_MPEG4, aEncodeParam.nFrameWidth, aEncodeParam.nFrameHeight, aEncodeParam.xFramerate >> 16, aEncodeParam.nBitrate/1000, gop_num);
	}		  
	if (m_mpeg4enc_handle == NULL) {
		LOGV("m_mpeg4enc_handle == NULL \n");
		return OMX_ErrorInsufficientResources;
	}

//	LOGV("aEncodeParam.nFrameWidth = %d", aEncodeParam.nFrameWidth);
//	LOGV("aEncodeParam.nFrameHeight = %d", aEncodeParam.nFrameHeight);

	ret = SsbSipMPEG4EncodeExe(m_mpeg4enc_handle);
	if(ret != SSBSIP_MPEG4_ENC_RET_OK){
		LOGV("Error : SsbSipMPEG4EncodeExe(before GetInBuf) \n");
		return OMX_ErrorUndefined;
	}
	
	m_mpeg4enc_buffer = (unsigned char *)SsbSipMPEG4EncodeGetInBuf(m_mpeg4enc_handle, 0);
	if (m_mpeg4enc_buffer == NULL) {
		LOGV("m_mpeg4enc_buffer == NULL \n");
		return OMX_ErrorInsufficientResources;
	}

	m_mpeg4enc_create_flag = 1;

	return OMX_ErrorNone;
	
}

static int convertYUV420sp2YUV420p(unsigned char *dst, unsigned char *src, int size)
{
     unsigned int    planar    = (size * 2) / 3;
     unsigned int    cbcr_size = (size - planar) >> 1;
     unsigned char * src_cbcr    = src + planar;
     unsigned char *dst_cb = dst + planar;
     unsigned char *dst_cr    = dst + planar + cbcr_size;
     unsigned char *cr;
     unsigned char *cb;

     // for Y
     memcpy(dst, src, planar);
#if 0
	for(unsigned int i = 0; i < cbcr_size; i++)
     {
         *dst_cr++ = *src_cbcr++;
         *dst_cb++ = *src_cbcr++;
     }
#endif
#if 1
//	unsigned long i;
//	for(i = 0; i < 10000000; i++) {
//		if((i % 32767) == 0)
//			i+=2;
//	}

	if(cbcr_size > (MPEG4_MAX_HEIGHT * MPEG4_MAX_WIDTH / 4))
	{
		LOGE("\n\n Kindly change the MPEG4_MAX_HEIGHT and MPEG4_MAX_WIDTH macros \n");
		return -1;
	}

	cb_ptr = cb_cr_mem;
	cr_ptr = cb_cr_mem + cbcr_size;
	for(unsigned int i = 0; i < cbcr_size; i++)
	{
		*cr_ptr++ = *src_cbcr++;
		*cb_ptr++ = *src_cbcr++;
	}

	cb_ptr = cb_cr_mem;
	cr_ptr = cb_cr_mem + cbcr_size;

	memcpy(dst_cr, cr_ptr, cbcr_size);
	memcpy(dst_cb, cb_ptr, cbcr_size);
#endif
         return 0;

}

/*Encode routine */
OMX_BOOL Mpeg4Encoder_OMX::Mp4EncodeVideo(OMX_U8*    aOutBuffer,
        OMX_U32*   aOutputLength,
        OMX_BOOL*  aBufferOverRun,
        OMX_U8**   aOverBufferPointer,
        OMX_U8*    aInBuffer,
        OMX_U32    aInBufSize,
        OMX_TICKS  aInTimeStamp,
        OMX_TICKS* aOutTimeStamp,
        OMX_BOOL*  aSyncFlag)
{
//	LOGV("MFC: Mpeg4Encoder_OMX::Mp4EncodeVideo() \n");

	unsigned char *p_output_buffer = NULL;
	long encoded_size = 0;
	int ret;
// RainAde for Encodig pic type
	int pic_type;

	// TimeStamp : bypass
	*aOutTimeStamp = aInTimeStamp;
	  
	// copy YUV data from OMX component src buffer to MFC src buffer
	// input data from Real camera
	if(frame_cnt != 1) {// 1: 1st I frame
		ret = convertYUV420sp2YUV420p(m_mpeg4enc_buffer, aInBuffer, aInBufSize);
		if(ret)
			return OMX_FALSE;
//		memcpy(m_mpeg4enc_buffer, aInBuffer, aInBufSize);
	}

	// *** Encoding
	if(frame_cnt != 1) // 1: 1st I frame
	{
		ret = SsbSipMPEG4EncodeExe(m_mpeg4enc_handle);
		if(ret != SSBSIP_MPEG4_ENC_RET_OK ) {
			LOGV("Error : SsbSipMPEG4EncodeExe \n");
			return OMX_FALSE;
		}
	}
	
	// get dst buffer address(stored Encoded data) and size(Encoded size)
	p_output_buffer = (unsigned char *)SsbSipMPEG4EncodeGetOutBuf(m_mpeg4enc_handle, &encoded_size);
	if (p_output_buffer == NULL) {
		LOGV("p_output_buffer == NULL \n");
		return OMX_FALSE;
	}

	if(frame_cnt == 0)	// VOL
		SsbSipMPEG4EncodeGetConfig(m_mpeg4enc_handle, MPEG4_ENC_GETCONF_HEADER_SIZE, &hdr_size);

	if(frame_cnt == 0) // 0: VOL
		encoded_size = hdr_size;
	else if(frame_cnt == 1) // 1: 1st I frame
		encoded_size -= hdr_size;
	
	if(*aOutputLength < encoded_size) {
		LOGV("output buffer is smaller than encoded data size \n");
		return OMX_FALSE;
	}

	// copy Encoded data from MFC dst buffer to OMX component dst buffer as much as the Encoded data size	
	*aOutputLength = (OMX_U32)encoded_size;

	if(frame_cnt != 1)
		memcpy(aOutBuffer, p_output_buffer, *aOutputLength);
	else
		memcpy(aOutBuffer, p_output_buffer+hdr_size, *aOutputLength);
	
// RainAde for Encoding pic type
	SsbSipMPEG4EncodeGetConfig(m_mpeg4enc_handle, MPEG4_ENC_GETCONF_PIC_TYPE, &pic_type);

// RainAde for thumbnail
	if(pic_type == PIC_TYPE_INTRA)
		*aSyncFlag = OMX_TRUE;
	else
		*aSyncFlag = OMX_FALSE;
	
	LOGV("pic_type = %d, *aSyncFlag = %d", pic_type, *aSyncFlag);

	frame_cnt++;
	
	return OMX_TRUE;

}


OMX_ERRORTYPE Mpeg4Encoder_OMX::Mp4EncDeinit()
{
	LOGV("MFC: Mpeg4Encoder_OMX::Mp4EncDeinit() \n");

	int ret;
	
	if(m_mpeg4enc_create_flag == 0)
	{
		LOGV("s5p6442_mpeg4enc_create == 0\n");
		return OMX_ErrorUndefined;
	}

	ret = SsbSipMPEG4EncodeDeInit(m_mpeg4enc_handle);
	if(ret != SSBSIP_MPEG4_ENC_RET_OK ) {
		LOGV("Error : SsbSipMPEG4EncodeDeInit\n");
		return OMX_ErrorUndefined;
	}
	
	m_mpeg4enc_buffer = NULL;
	m_mpeg4enc_create_flag = 0;

    return OMX_ErrorNone;
}


OMX_ERRORTYPE Mpeg4Encoder_OMX::Mp4RequestIFrame()
{
	int para_change[2];
	int ret;

	para_change[0] = MPEG4_ENC_PIC_OPT_IDR;
	para_change[1] = 1;

	ret = SsbSipMPEG4EncodeSetConfig(m_mpeg4enc_handle, MPEG4_ENC_SETCONF_CUR_PIC_OPT, para_change);
	
	if(ret != SSBSIP_MPEG4_ENC_RET_OK){
		LOGV("Error : SsbSipMPEG4EncodeSetConfig(Request I frame) \n");
		return OMX_ErrorUndefined;
	}

    return OMX_ErrorNone;
}


OMX_BOOL Mpeg4Encoder_OMX::Mp4UpdateBitRate(OMX_U32 aEncodedBitRate)
{
	int para_change[2];
	int ret;

	para_change[0] = MPEG4_ENC_PARAM_BITRATE;
	para_change[1] = aEncodedBitRate/1000;

	ret = SsbSipMPEG4EncodeSetConfig(m_mpeg4enc_handle, MPEG4_ENC_SETCONF_PARAM_CHANGE, para_change);

	if(ret != SSBSIP_MPEG4_ENC_RET_OK){
		LOGV("Error : SsbSipMPEG4EncodeSetConfig(Bitrate) \n");
		return OMX_FALSE;
	}

    return OMX_TRUE;
}


OMX_BOOL Mpeg4Encoder_OMX::Mp4UpdateFrameRate(OMX_U32 aEncodeFramerate)
{
	int para_change[2];
	int ret;

	para_change[0] = MPEG4_ENC_PARAM_F_RATE;
	para_change[1] = aEncodeFramerate >> 16;

	ret = SsbSipMPEG4EncodeSetConfig(m_mpeg4enc_handle, MPEG4_ENC_SETCONF_PARAM_CHANGE, para_change);

	if(ret != SSBSIP_MPEG4_ENC_RET_OK){
		LOGV("Error : SsbSipMPEG4EncodeSetConfig(Framerate) \n");
		return OMX_FALSE;
	}

    return OMX_TRUE;
}

#else /* SLSI_S5P6442 */
#define MAX_SUPPORTED_LAYER 1

const uint8 DEFAULT_VOL_HEADER[DEFAULT_VOL_HEADER_LENGTH] =
{
    0x00, 0x00, 0x01, 0xB0, 0x08, 0x00, 0x00, 0x01,
    0xB5, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x20, 0x00, 0x84, 0x40, 0xFA, 0x28, 0x2C,
    0x20, 0x90, 0xA2, 0x1F
};



Mpeg4Encoder_OMX::Mpeg4Encoder_OMX()
{
    iVolHeaderFlag = OMX_FALSE;

    iInitialized = OMX_FALSE;
    iYUVIn = NULL;
    ccRGBtoYUV = NULL;

    // Create a default VOL header
    oscl_memset(iVolHeader, 0, DEFAULT_VOL_HEADER_LENGTH);
    oscl_memcpy(iVolHeader, (OsclAny*)DEFAULT_VOL_HEADER, DEFAULT_VOL_HEADER_LENGTH);
    iVolHeaderSize = DEFAULT_VOL_HEADER_LENGTH;



}


/* Initialization routine */
OMX_ERRORTYPE Mpeg4Encoder_OMX::Mp4EncInit(OMX_S32 iEncMode,
        OMX_VIDEO_PORTDEFINITIONTYPE aInputParam,
        OMX_CONFIG_ROTATIONTYPE aInputOrientationType,
        OMX_VIDEO_PORTDEFINITIONTYPE aEncodeParam,
        OMX_VIDEO_PARAM_MPEG4TYPE aEncodeMpeg4Param,
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE aErrorCorrection,
        OMX_VIDEO_PARAM_BITRATETYPE aRateControlType,
        OMX_VIDEO_PARAM_QUANTIZATIONTYPE aQuantType,
        OMX_VIDEO_PARAM_MOTIONVECTORTYPE aSearchRange,
        OMX_VIDEO_PARAM_INTRAREFRESHTYPE aIntraRefresh,
        OMX_VIDEO_PARAM_H263TYPE aH263Type,
        OMX_VIDEO_PARAM_PROFILELEVELTYPE* aProfileLevel)
{
    OMX_U8* pMemBuffer;
    VideoEncOptions aEncOption; /* encoding options */

    Int quantType[2] = {0, 0};      /* default H.263 quant*/

    oscl_memset((void*) &iEncoderControl, 0, sizeof(VideoEncControls));

    switch (iEncMode)
    {
        case MODE_H263:
        {
            if (aH263Type.nGOBHeaderInterval > 0)
            {
                ENC_Mode = H263_MODE_WITH_ERR_RES;
            }
            else
            {
                ENC_Mode = H263_MODE;
            }
        }
        break;

        case MODE_MPEG4:
        {
            if (OMX_TRUE == aEncodeMpeg4Param.bSVH)
            {
                if (aH263Type.nGOBHeaderInterval > 0)
                {
                    ENC_Mode = SHORT_HEADER_WITH_ERR_RES;
                }
                else
                {
                    ENC_Mode = SHORT_HEADER;
                }
            }
            else
            {
                if (OMX_TRUE == aErrorCorrection.bEnableDataPartitioning)
                {
                    ENC_Mode = DATA_PARTITIONING_MODE;
                }
                else if (OMX_TRUE == aErrorCorrection.bEnableResync)
                {
                    ENC_Mode = COMBINE_MODE_WITH_ERR_RES;
                }
                else
                {
                    ENC_Mode = COMBINE_MODE_NO_ERR_RES;
                }
            }
        }
        break;

        default:
        {
            return OMX_ErrorUnsupportedSetting;
        }
    }

    iSrcWidth = aInputParam.nFrameWidth;
    iSrcHeight = aInputParam.nFrameHeight;
    iSrcFrameRate = aInputParam.xFramerate;

    iFrameOrientation = aInputOrientationType.nRotation;


    if ((OMX_COLOR_FormatYUV420Planar == aInputParam.eColorFormat) ||
            (OMX_COLOR_Format24bitRGB888 == aInputParam.eColorFormat) ||
            (OMX_COLOR_Format12bitRGB444 == aInputParam.eColorFormat) ||
            (OMX_COLOR_FormatYUV420SemiPlanar == aInputParam.eColorFormat))
    {
        iVideoFormat = aInputParam.eColorFormat;
    }
    else
    {
        return OMX_ErrorUnsupportedSetting;
    }

    //Verify the input compression format
    if (OMX_VIDEO_CodingUnused != aInputParam.eCompressionFormat)
    {
        //Input port must have no compression supported
        return OMX_ErrorUnsupportedSetting;
    }


    if (OMX_TRUE == iInitialized)
    {
        /* clean up before re-initialized */
        PVCleanUpVideoEncoder(&iEncoderControl);
        if (iYUVIn)
        {
            oscl_free(iYUVIn);
            iYUVIn = NULL;
        }

    }

    // allocate iYUVIn
    if (((iSrcWidth & 0xF) || (iSrcHeight & 0xF)) || OMX_COLOR_FormatYUV420Planar != iVideoFormat) /* Not multiple of 16 */
    {
        iYUVIn = (uint8*) oscl_malloc(((((iSrcWidth + 15) >> 4) * ((iSrcHeight + 15) >> 4)) * 3) << 7);
        if (NULL == iYUVIn)
        {
            return OMX_ErrorInsufficientResources;
        }
    }

    /* Initialize the color conversion */
    if (OMX_COLOR_Format24bitRGB888 == iVideoFormat)
    {
        ccRGBtoYUV = CCRGB24toYUV420::New();
        ccRGBtoYUV->Init(iSrcWidth, iSrcHeight, iSrcWidth, iSrcWidth, iSrcHeight, ((iSrcWidth + 15) >> 4) << 4, (iFrameOrientation == 180 ? CCBOTTOM_UP : 0));
    }
    if (OMX_COLOR_Format12bitRGB444 == iVideoFormat)
    {
        ccRGBtoYUV = CCRGB12toYUV420::New();
        ccRGBtoYUV->Init(iSrcWidth, iSrcHeight, iSrcWidth, iSrcWidth, iSrcHeight, ((iSrcWidth + 15) >> 4) << 4, (iFrameOrientation == 180 ? CCBOTTOM_UP : 0));
    }
    if (OMX_COLOR_FormatYUV420SemiPlanar == iVideoFormat)
    {
        ccRGBtoYUV = CCYUV420SEMItoYUV420::New();
        ccRGBtoYUV->Init(iSrcWidth, iSrcHeight, iSrcWidth, iSrcWidth, iSrcHeight, ((iSrcWidth + 15) >> 4) << 4, (iFrameOrientation == 180 ? CCBOTTOM_UP : 0));
    }


    PVGetDefaultEncOption(&aEncOption, 0);

    aEncOption.encWidth[0] = aEncodeParam.nFrameWidth;
    aEncOption.encHeight[0] = aEncodeParam.nFrameHeight;
    aEncOption.encFrameRate[0] = (aEncodeParam.xFramerate >> 16) + (OsclFloat)(aEncodeParam.xFramerate & 0xFFFF) / (1 << 16);



    switch (aRateControlType.eControlRate)
    {
        case OMX_Video_ControlRateDisable:
        {
            aEncOption.rcType = CONSTANT_Q;
            aEncOption.vbvDelay = (float)10.0;
        }
        break;

        case OMX_Video_ControlRateConstant:
        {
            aEncOption.rcType = CBR_1;
            aEncOption.vbvDelay = (float)2.0;
        }
        break;

        case OMX_Video_ControlRateVariable:
        {
            aEncOption.rcType = VBR_1;
            aEncOption.vbvDelay = (float)5.0;
        }
        break;

        default:
            return OMX_ErrorUnsupportedSetting;
    }

    //Set the profile level of encoder
    switch (aEncodeMpeg4Param.eProfile)
    {
        case OMX_VIDEO_MPEG4ProfileSimple:
        {
            if (OMX_VIDEO_MPEG4Level0 == aEncodeMpeg4Param.eLevel)
            {
                aEncOption.profile_level = SIMPLE_PROFILE_LEVEL0;
            }
            else if (OMX_VIDEO_MPEG4Level1 == aEncodeMpeg4Param.eLevel)
            {
                aEncOption.profile_level = SIMPLE_PROFILE_LEVEL1;
            }
            else if (OMX_VIDEO_MPEG4Level2 == aEncodeMpeg4Param.eLevel)
            {
                aEncOption.profile_level = SIMPLE_PROFILE_LEVEL2;
            }
            else if (OMX_VIDEO_MPEG4Level3 == aEncodeMpeg4Param.eLevel)
            {
                aEncOption.profile_level = SIMPLE_PROFILE_LEVEL3;
            }
            else
            {
                return OMX_ErrorUnsupportedSetting;
            }
        }
        break;

        case OMX_VIDEO_MPEG4ProfileSimpleScalable:
        {
            if (OMX_VIDEO_MPEG4Level0 == aEncodeMpeg4Param.eLevel)
            {
                aEncOption.profile_level = SIMPLE_SCALABLE_PROFILE_LEVEL0;
            }
            else if (OMX_VIDEO_MPEG4Level1 == aEncodeMpeg4Param.eLevel)
            {
                aEncOption.profile_level = SIMPLE_SCALABLE_PROFILE_LEVEL1;
            }
            else if (OMX_VIDEO_MPEG4Level2 == aEncodeMpeg4Param.eLevel)
            {
                aEncOption.profile_level = SIMPLE_SCALABLE_PROFILE_LEVEL2;
            }
            else
            {
                return OMX_ErrorUnsupportedSetting;
            }
        }
        break;

        case OMX_VIDEO_MPEG4ProfileCore:
        {
            if (OMX_VIDEO_MPEG4Level1 == aEncodeMpeg4Param.eLevel)
            {
                aEncOption.profile_level = CORE_PROFILE_LEVEL1;
            }
            else if (OMX_VIDEO_MPEG4Level2 == aEncodeMpeg4Param.eLevel)
            {
                aEncOption.profile_level = CORE_PROFILE_LEVEL2;
            }
            else
            {
                return OMX_ErrorUnsupportedSetting;
            }
        }
        break;

        case OMX_VIDEO_MPEG4ProfileCoreScalable:
        {
            if (OMX_VIDEO_MPEG4Level1 == aEncodeMpeg4Param.eLevel)
            {
                aEncOption.profile_level = CORE_SCALABLE_PROFILE_LEVEL1;
            }
            else if (OMX_VIDEO_MPEG4Level2 == aEncodeMpeg4Param.eLevel)
            {
                aEncOption.profile_level = CORE_SCALABLE_PROFILE_LEVEL2;
            }
            else if (OMX_VIDEO_MPEG4Level3 == aEncodeMpeg4Param.eLevel)
            {
                aEncOption.profile_level = CORE_SCALABLE_PROFILE_LEVEL3;
            }
            else
            {
                return OMX_ErrorUnsupportedSetting;
            }
        }
        break;

        default:
        {
            return OMX_ErrorUnsupportedSetting;
        }

    }

    aEncOption.encMode = ENC_Mode;

    if (DATA_PARTITIONING_MODE == ENC_Mode)
    {
        aEncOption.packetSize = aEncodeMpeg4Param.nMaxPacketSize;
    }
    else
    {
        aEncOption.packetSize = aErrorCorrection.nResynchMarkerSpacing / 8;
    }

    if ((OMX_TRUE == aErrorCorrection.bEnableRVLC) ||
            (OMX_TRUE == aEncodeMpeg4Param.bReversibleVLC))
    {
        aEncOption.rvlcEnable = PV_ON;
    }
    else
    {
        aEncOption.rvlcEnable = PV_OFF;
    }

    aEncOption.numLayers = MAX_SUPPORTED_LAYER;
    if ((aEncodeMpeg4Param.nTimeIncRes << 16) > iSrcFrameRate)
    {
        aEncOption.timeIncRes = aEncodeMpeg4Param.nTimeIncRes;
    }
    else
    {
        //Default value
        aEncOption.timeIncRes = 1000;
    }

    if (iSrcFrameRate > 0)
    {
        aEncOption.tickPerSrc = (aEncOption.timeIncRes << 16) / iSrcFrameRate;
    }

    aEncOption.bitRate[0] = aEncodeParam.nBitrate;
    aEncOption.iQuant[0] = aQuantType.nQpI;
    aEncOption.pQuant[0] = aQuantType.nQpP;
    aEncOption.quantType[0] = quantType[0]; /* default to H.263 */


    aEncOption.noFrameSkipped = PV_OFF;

    //IPPPPPPPPPP, indicates I-frame followed by all P-frames
    if (0xFFFFFFFF == aEncodeMpeg4Param.nPFrames)
    {
        aEncOption.intraPeriod = -1;
    }
    else
    {
        aEncOption.intraPeriod = aEncodeMpeg4Param.nPFrames + 1;
    }

    //No support for B Frames
    if (aEncodeMpeg4Param.nBFrames > 0)
    {
        return OMX_ErrorUnsupportedSetting;
    }

    //Encoder support only I and P frames picture type
    if (0 == (aEncodeMpeg4Param.nAllowedPictureTypes &
              (OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP)))
    {
        return OMX_ErrorUnsupportedSetting;
    }

    if (OMX_VIDEO_PictureTypeI == aEncodeMpeg4Param.nAllowedPictureTypes) // I-only
    {
        aEncOption.intraPeriod = 1;
    }


    if ((OMX_VIDEO_IntraRefreshCyclic == aIntraRefresh.eRefreshMode) ||
            (OMX_VIDEO_IntraRefreshBoth == aIntraRefresh.eRefreshMode))
    {
        aEncOption.numIntraMB = aIntraRefresh.nCirMBs;
    }

    if ((OMX_VIDEO_IntraRefreshAdaptive == aIntraRefresh.eRefreshMode) ||
            (OMX_VIDEO_IntraRefreshBoth == aIntraRefresh.eRefreshMode))
    {
        aEncOption.sceneDetect = PV_ON;
    }
    else
    {
        aEncOption.sceneDetect = PV_OFF;
    }

    aEncOption.searchRange = (aSearchRange.sXSearchRange <= aSearchRange.sYSearchRange ? aSearchRange.sXSearchRange : aSearchRange.sYSearchRange);
    aEncOption.mv8x8Enable = PV_OFF; //disable for now(aSearchRange.bFourMV == OMX_TRUE) ? PV_ON : PV_OFF;

    aEncOption.gobHeaderInterval = aH263Type.nGOBHeaderInterval;

    aEncOption.useACPred = ((aEncodeMpeg4Param.bACPred == OMX_TRUE) ? PV_ON : PV_OFF);

    aEncOption.intraDCVlcTh = aEncodeMpeg4Param.nIDCVLCThreshold;
    /* note aEncOption.intraDCVlcTh will be clipped btw (0,7) inside PV M4V encoder */

    //Checking the range of parameters that the encoder can't support here
    // and return OMX_ErrorUnsupportedSetting

    /***** Initlaize the encoder *****/
    if (PV_FALSE == PVInitVideoEncoder(&iEncoderControl, &aEncOption))
    {
        iInitialized = OMX_FALSE;
        return OMX_ErrorBadParameter;
    }

    iInitialized = OMX_TRUE;
    iNextModTime = 0;


    //Update the vol header for non-h263 modes
    if ((DATA_PARTITIONING_MODE == ENC_Mode) ||
            (COMBINE_MODE_WITH_ERR_RES == ENC_Mode) ||
            (COMBINE_MODE_NO_ERR_RES == ENC_Mode) ||
            (SHORT_HEADER == ENC_Mode) ||
            (SHORT_HEADER_WITH_ERR_RES == ENC_Mode))
    {
        // M4V output, get VOL header
        iVolHeaderSize = 32; // Encoder requires that buffer size is greater than vol header size (28)

        pMemBuffer = (OMX_U8*) oscl_malloc(iVolHeaderSize);
        oscl_memset(pMemBuffer, 0, iVolHeaderSize);

        int32 Length = iVolHeaderSize;

        if (PV_FALSE == PVGetVolHeader(&iEncoderControl, (uint8*)pMemBuffer, &Length, 0))
        {
            return OMX_ErrorBadParameter;
        }

        iVolHeaderSize = Length;
        oscl_memcpy(iVolHeader, (OsclAny*)pMemBuffer, iVolHeaderSize);

        oscl_free(pMemBuffer);
        pMemBuffer = NULL;
    }


    //Updating the profile and level from the encoder after initialization
    if (MODE_MPEG4 == iEncMode)
    {
        ProfileLevelType EncProfileLevel;
        Int temp;

        if (PV_FALSE == PVGetMPEG4ProfileLevelID(&iEncoderControl, &temp, 0))
        {
            return OMX_ErrorBadParameter;
        }
        EncProfileLevel = (ProfileLevelType) temp;

        //Convert the EncProfileLevel to appropriate profile and level of openmax
        if (SIMPLE_PROFILE_LEVEL0 == EncProfileLevel)
        {
            aProfileLevel->eProfile = OMX_VIDEO_MPEG4ProfileSimple;
            aProfileLevel->eLevel = OMX_VIDEO_MPEG4Level0;

        }
        else if (SIMPLE_PROFILE_LEVEL1 == EncProfileLevel)
        {
            aProfileLevel->eProfile = OMX_VIDEO_MPEG4ProfileSimple;
            aProfileLevel->eLevel = OMX_VIDEO_MPEG4Level1;
        }
        else if (SIMPLE_PROFILE_LEVEL2 == EncProfileLevel)
        {
            aProfileLevel->eProfile = OMX_VIDEO_MPEG4ProfileSimple;
            aProfileLevel->eLevel = OMX_VIDEO_MPEG4Level2;
        }
        else if (SIMPLE_PROFILE_LEVEL3 == EncProfileLevel)
        {
            aProfileLevel->eProfile = OMX_VIDEO_MPEG4ProfileSimple;
            aProfileLevel->eLevel = OMX_VIDEO_MPEG4Level3;
        }
        else if (CORE_PROFILE_LEVEL1 == EncProfileLevel)
        {
            aProfileLevel->eProfile = OMX_VIDEO_MPEG4ProfileCore;
            aProfileLevel->eLevel = OMX_VIDEO_MPEG4Level1;
        }
        else if (CORE_PROFILE_LEVEL2 == EncProfileLevel)
        {
            aProfileLevel->eProfile = OMX_VIDEO_MPEG4ProfileCore;
            aProfileLevel->eLevel = OMX_VIDEO_MPEG4Level2;
        }
        else if (SIMPLE_SCALABLE_PROFILE_LEVEL0 == EncProfileLevel)
        {
            aProfileLevel->eProfile = OMX_VIDEO_MPEG4ProfileSimpleScalable;
            aProfileLevel->eLevel = OMX_VIDEO_MPEG4Level0;
        }
        else if (SIMPLE_SCALABLE_PROFILE_LEVEL1 == EncProfileLevel)
        {
            aProfileLevel->eProfile = OMX_VIDEO_MPEG4ProfileSimpleScalable;
            aProfileLevel->eLevel = OMX_VIDEO_MPEG4Level1;
        }
        else if (SIMPLE_SCALABLE_PROFILE_LEVEL2 == EncProfileLevel)
        {
            aProfileLevel->eProfile = OMX_VIDEO_MPEG4ProfileSimpleScalable;
            aProfileLevel->eLevel = OMX_VIDEO_MPEG4Level2;
        }
        else if (CORE_SCALABLE_PROFILE_LEVEL1 == EncProfileLevel)
        {
            aProfileLevel->eProfile = OMX_VIDEO_MPEG4ProfileCoreScalable;
            aProfileLevel->eLevel = OMX_VIDEO_MPEG4Level1;
        }
        else if (CORE_SCALABLE_PROFILE_LEVEL2 == EncProfileLevel)
        {
            aProfileLevel->eProfile = OMX_VIDEO_MPEG4ProfileCoreScalable;
            aProfileLevel->eLevel = OMX_VIDEO_MPEG4Level2;
        }
        else if (CORE_SCALABLE_PROFILE_LEVEL3 == EncProfileLevel)
        {
            aProfileLevel->eProfile = OMX_VIDEO_MPEG4ProfileCoreScalable;
            aProfileLevel->eLevel = OMX_VIDEO_MPEG4Level3;
        }
        else
        {
            //None of the supported parameter found
            return OMX_ErrorBadParameter;
        }
    }
    //mode H263
    else
    {
        Int ProfileID, LevelID;

        if (PV_FALSE == PVGetH263ProfileLevelID(&iEncoderControl, &ProfileID, &LevelID))
        {
            return OMX_ErrorBadParameter;
        }

        //Converting the encoder level into the openmax parameters
        //Profile is set constant at baseline
        if (10 == LevelID)
        {
            aProfileLevel->eLevel = OMX_VIDEO_H263Level10;
        }
        else if (20 == LevelID)
        {
            aProfileLevel->eLevel = OMX_VIDEO_H263Level20;
        }
        else if (30 == LevelID)
        {
            aProfileLevel->eLevel = OMX_VIDEO_H263Level30;
        }
        else if (40 == LevelID)
        {
            aProfileLevel->eLevel = OMX_VIDEO_H263Level40;
        }
        else if (50 == LevelID)
        {
            aProfileLevel->eLevel = OMX_VIDEO_H263Level50;
        }
        else if (60 == LevelID)
        {
            aProfileLevel->eLevel = OMX_VIDEO_H263Level60;
        }
        else if (70 == LevelID)
        {
            aProfileLevel->eLevel = OMX_VIDEO_H263Level70;
        }
        else
        {
            //None of the supported parameter found
            return OMX_ErrorBadParameter;
        }
    }

    return OMX_ErrorNone;

}


OMX_ERRORTYPE Mpeg4Encoder_OMX::Mp4RequestIFrame()
{
    if (PV_TRUE != PVIFrameRequest(&iEncoderControl))
    {
        return OMX_ErrorUndefined;
    }

    return OMX_ErrorNone;
}


OMX_BOOL Mpeg4Encoder_OMX::Mp4UpdateBitRate(OMX_U32 aEncodedBitRate)
{
    Int BitRate[2] = {0, 0};
    OMX_BOOL Status = OMX_TRUE;

    //Update the bit rate only if encoder has been initialized
    if (OMX_TRUE == iInitialized)
    {
        BitRate[0] = aEncodedBitRate;

        Status = (OMX_BOOL) PVUpdateBitRate(&iEncoderControl, BitRate);
    }
    return Status;
}


OMX_BOOL Mpeg4Encoder_OMX::Mp4UpdateFrameRate(OMX_U32 aEncodeFramerate)
{
    float EncFrameRate[2] = {0., 0.};
    OMX_BOOL Status = OMX_TRUE;

    //Update the frame rate only if encoder has been initialized
    if (OMX_TRUE == iInitialized)
    {
        EncFrameRate[0] = (float)(aEncodeFramerate >> 16);
        Status = (OMX_BOOL) PVUpdateEncFrameRate(&iEncoderControl, EncFrameRate);
    }
    return Status;

}




/*Encode routine */
OMX_BOOL Mpeg4Encoder_OMX::Mp4EncodeVideo(OMX_U8*    aOutBuffer,
        OMX_U32*   aOutputLength,
        OMX_BOOL*  aBufferOverRun,
        OMX_U8**   aOverBufferPointer,
        OMX_U8*    aInBuffer,
        OMX_U32    aInBufSize,
        OMX_TICKS  aInTimeStamp,
        OMX_TICKS* aOutTimeStamp,
        OMX_BOOL*  aSyncFlag)
{
    *aSyncFlag = OMX_FALSE;

    if (OMX_FALSE == iVolHeaderFlag)
    {
        iVolHeaderFlag = OMX_TRUE;
        iNextModTime = aInTimeStamp;

        //Send the first output buffer as vol header in case of m4v format
        if ((DATA_PARTITIONING_MODE == ENC_Mode) ||
                (COMBINE_MODE_WITH_ERR_RES == ENC_Mode) ||
                (COMBINE_MODE_NO_ERR_RES == ENC_Mode) ||
                (SHORT_HEADER == ENC_Mode) ||
                (SHORT_HEADER_WITH_ERR_RES == ENC_Mode))
        {
            oscl_memcpy(aOutBuffer, iVolHeader, iVolHeaderSize);
            *aOutputLength = iVolHeaderSize;
            *aOutTimeStamp = aInTimeStamp;
            return OMX_TRUE;
        }
    }


    /* Input Buffer Size Check
     * Input buffer size should be equal to one frame, otherwise drop the frame
     * as it is a corrupt data and don't encode it */

    if (OMX_COLOR_FormatYUV420Planar == iVideoFormat)
    {
        if (aInBufSize < (OMX_U32)((iSrcWidth * iSrcHeight * 3) >> 1))
        {
            *aOutputLength = 0;
            return OMX_FALSE;
        }
    }
    else if (OMX_COLOR_Format24bitRGB888 == iVideoFormat)
    {
        if (aInBufSize < (OMX_U32)(iSrcWidth * iSrcHeight * 3))
        {
            *aOutputLength = 0;
            return OMX_FALSE;
        }
    }
    else if (OMX_COLOR_Format12bitRGB444 == iVideoFormat)
    {
        if (aInBufSize < (OMX_U32)(iSrcWidth * iSrcHeight * 2))
        {
            *aOutputLength = 0;
            return OMX_FALSE;
        }
    }
    else if (OMX_COLOR_FormatYUV420SemiPlanar == iVideoFormat)
    {
        if (aInBufSize < (OMX_U32)((iSrcWidth * iSrcHeight * 3) >> 1))
        {
            *aOutputLength = 0;
            return OMX_FALSE;
        }
    }

    //Now encode the input buffer
    VideoEncFrameIO vid_in, vid_out;
    Int Size;
    Bool status;
    ULong modTime;
    Int nLayer = 0;

    // iNextModTime is in milliseconds (although it's a 64 bit value) whereas
    // aInTimestamp is in microseconds
    if ((iNextModTime * 1000) <= aInTimeStamp)
    {
        Size = *aOutputLength;

        if (iVideoFormat == OMX_COLOR_FormatYUV420Planar)
        {
            if (iYUVIn) /* iSrcWidth or iSrcHeight is not multiple of 16 */
            {
                CopyToYUVIn(aInBuffer, iSrcWidth, iSrcHeight,
                            ((iSrcWidth + 15) >> 4) << 4, ((iSrcHeight + 15) >> 4) << 4);
                iVideoIn = iYUVIn;
            }
            else /* otherwise, we can just use aVidIn->iSource */
            {
                iVideoIn = aInBuffer;
            }
        }

        else if ((iVideoFormat == OMX_COLOR_Format12bitRGB444) || (iVideoFormat == OMX_COLOR_Format24bitRGB888) || (iVideoFormat == OMX_COLOR_FormatYUV420SemiPlanar))
        {
            ccRGBtoYUV->Convert((uint8*)aInBuffer, iYUVIn);
            iVideoIn = iYUVIn;
        }

        /* with backward-P or B-Vop this timestamp must be re-ordered */
        *aOutTimeStamp = aInTimeStamp;

        vid_in.height = ((iSrcHeight + 15) >> 4) << 4;
        vid_in.pitch = ((iSrcWidth + 15) >> 4) << 4;
        vid_in.timestamp = ((ULong) aInTimeStamp / 1000); //converting microsec to millisec
        vid_in.yChan = (UChar*)iVideoIn;
        vid_in.uChan = (UChar*)(iVideoIn + vid_in.height * vid_in.pitch);
        vid_in.vChan = vid_in.uChan + ((vid_in.height * vid_in.pitch) >> 2);

        status = PVEncodeVideoFrame(&iEncoderControl, &vid_in, &vid_out,
                                    &modTime, (UChar*)aOutBuffer,
                                    &Size, &nLayer);


        if (status == PV_TRUE)
        {
            iNextModTime = modTime; // this is time in milliseconds

            if ((nLayer >= 0) && ((OMX_U32) Size > *aOutputLength)) // overrun buffer is used by the encoder
            {
                *aOverBufferPointer = PVGetOverrunBuffer(&iEncoderControl);
                *aBufferOverRun = OMX_TRUE;
            }

            *aOutputLength = Size;

            if (Size > 0)
            {
                *aOutTimeStamp = ((OMX_TICKS) vid_out.timestamp * 1000);  //converting millisec to microsec

                PVGetHintTrack(&iEncoderControl, &iHintTrack);
                if (0 == iHintTrack.CodeType)
                {
                    //Its an I Frame, mark the sync flag as true
                    *aSyncFlag = OMX_TRUE;
                }
            }

            return OMX_TRUE;
        }
        else
        {
            *aOutputLength = 0;
            return OMX_FALSE;
        }
    }
    else /* if(aInTimeStamp >= iNextModTime) */
    {
        *aOutputLength = 0;
        return OMX_TRUE;
    }
}



OMX_ERRORTYPE Mpeg4Encoder_OMX::Mp4EncDeinit()
{
    if (OMX_TRUE == iInitialized)
    {
        PVCleanUpVideoEncoder(&iEncoderControl);
        iInitialized = OMX_FALSE;

        if (iYUVIn)
        {
            oscl_free(iYUVIn);
            iYUVIn = NULL;
        }

        if (ccRGBtoYUV)
        {
            OSCL_DELETE(ccRGBtoYUV);
            ccRGBtoYUV = NULL;
        }

    }
    return OMX_ErrorNone;
}


/* COLOUR CONVERSION ROUTINES ARE WRITTEN BELOW*/

/* ///////////////////////////////////////////////////////////////////////// */
/* Copy from YUV input to YUV frame inside M4VEnc lib                       */
/* When input is not YUV, the color conv will write it directly to iVideoInOut. */
/* ///////////////////////////////////////////////////////////////////////// */

void Mpeg4Encoder_OMX::CopyToYUVIn(uint8 *YUV, Int width, Int height, Int width_16, Int height_16)
{
    UChar *y, *u, *v, *yChan, *uChan, *vChan;
    Int y_ind, ilimit, jlimit, i, j, ioffset;
    Int size = width * height;
    Int size16 = width_16 * height_16;

    /* do padding at the bottom first */
    /* do padding if input RGB size(height) is different from the output YUV size(height_16) */
    if (height < height_16 || width < width_16) /* if padding */
    {
        Int offset = (height < height_16) ? height : height_16;

        offset = (offset * width_16);

        if (width < width_16)
        {
            offset -= (width_16 - width);
        }

        yChan = (UChar*)(iYUVIn + offset);
        oscl_memset(yChan, 16, size16 - offset); /* pad with zeros */

        uChan = (UChar*)(iYUVIn + size16 + (offset >> 2));
        oscl_memset(uChan, 128, (size16 - offset) >> 2);

        vChan = (UChar*)(iYUVIn + size16 + (size16 >> 2) + (offset >> 2));
        oscl_memset(vChan, 128, (size16 - offset) >> 2);
    }

    /* then do padding on the top */
    yChan = (UChar*)iYUVIn; /* Normal order */
    uChan = (UChar*)(iYUVIn + size16);
    vChan = (UChar*)(uChan + (size16 >> 2));

    u = (UChar*)(&(YUV[size]));
    v = (UChar*)(&(YUV[size*5/4]));

    /* To center the output */
    if (height_16 > height)
    {  /* output taller than input */
        if (width_16 >= width)
        { /* output wider than or equal input */
            i = ((height_16 - height) >> 1) * width_16 + (((width_16 - width) >> 3) << 2);
            /* make sure that (width_16-width)>>1 is divisible by 4 */
            j = ((height_16 - height) >> 2) * (width_16 >> 1) + (((width_16 - width) >> 4) << 2);
            /* make sure that (width_16-width)>>2 is divisible by 4 */
        }
        else
        { /* output narrower than input */
            i = ((height_16 - height) >> 1) * width_16;
            j = ((height_16 - height) >> 2) * (width_16 >> 1);
            YUV += ((width - width_16) >> 1);
            u += ((width - width_16) >> 2);
            v += ((width - width_16) >> 2);
        }
        oscl_memset((uint8 *)yChan, 16, i);
        yChan += i;
        oscl_memset((uint8 *)uChan, 128, j);
        uChan += j;
        oscl_memset((uint8 *)vChan, 128, j);
        vChan += j;
    }
    else
    {  /* output shorter or equal input */
        if (width_16 >= width)
        {  /* output wider or equal input */
            i = (((width_16 - width) >> 3) << 2);
            /* make sure that (width_16-width)>>1 is divisible by 4 */
            j = (((width_16 - width) >> 4) << 2);
            /* make sure that (width_16-width)>>2 is divisible by 4 */
            YUV += (((height - height_16) >> 1) * width);
            u += (((height - height_16) >> 1) * width) >> 2;
            v += (((height - height_16) >> 1) * width) >> 2;
        }
        else
        { /* output narrower than input */
            i = 0;
            j = 0;
            YUV += (((height - height_16) >> 1) * width + ((width - width_16) >> 1));
            u += (((height - height_16) >> 1) * width + ((width - width_16) >> 1)) >> 2;
            v += (((height - height_16) >> 1) * width + ((width - width_16) >> 1)) >> 2;
        }
        oscl_memset((uint8 *)yChan, 16, i);
        yChan += i;
        oscl_memset((uint8 *)uChan, 128, j);
        uChan += j;
        oscl_memset((uint8 *)vChan, 128, j);
        vChan += j;
    }

    /* Copy with cropping or zero-padding */
    if (height < height_16)
        jlimit = height;
    else
        jlimit = height_16;

    if (width < width_16)
    {
        ilimit = width;
        ioffset = width_16 - width;
    }
    else
    {
        ilimit = width_16;
        ioffset = 0;
    }

    /* Copy Y */
    /* Set up pointer for fast looping */
    y = (UChar*)YUV;

    if (width == width_16 && height == height_16) /* no need to pad */
    {
        oscl_memcpy(yChan, y, size);
    }
    else
    {
        for (y_ind = 0; y_ind < (jlimit - 1) ; y_ind++)
        {
            oscl_memcpy(yChan, y, ilimit);
            oscl_memset(yChan + ilimit, 16, ioffset); /* pad with zero */
            yChan += width_16;
            y += width;
        }
        oscl_memcpy(yChan, y, ilimit); /* last line no padding */
    }
    /* Copy U and V */
    /* Set up pointers for fast looping */
    if (width == width_16 && height == height_16) /* no need to pad */
    {
        oscl_memcpy(uChan, u, size >> 2);
        oscl_memcpy(vChan, v, size >> 2);
    }
    else
    {
        for (y_ind = 0; y_ind < (jlimit >> 1) - 1; y_ind++)
        {
            oscl_memcpy(uChan, u, ilimit >> 1);
            oscl_memcpy(vChan, v, ilimit >> 1);
            oscl_memset(uChan + (ilimit >> 1), 128, ioffset >> 1);
            oscl_memset(vChan + (ilimit >> 1), 128, ioffset >> 1);
            uChan += (width_16 >> 1);
            u += (width >> 1);
            vChan += (width_16 >> 1);
            v += (width >> 1);
        }
        oscl_memcpy(uChan, u, ilimit >> 1); /* last line no padding */
        oscl_memcpy(vChan, v, ilimit >> 1);
    }

    return ;
}

#endif /* SLSI_S5P6442 */
