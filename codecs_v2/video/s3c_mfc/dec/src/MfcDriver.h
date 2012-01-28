#ifndef __SAMSUNG_SYSLSI_APDEV_S3C_MFC_H__
#define __SAMSUNG_SYSLSI_APDEV_S3C_MFC_H__


#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif


#define IOCTL_MFC_MPEG4_DEC_INIT			(0x00800001)
#define IOCTL_MFC_MPEG4_ENC_INIT			(0x00800002)
#define IOCTL_MFC_MPEG4_DEC_EXE				(0x00800003)
#define IOCTL_MFC_MPEG4_ENC_EXE				(0x00800004)

#define IOCTL_MFC_H264_DEC_INIT				(0x00800005)
#define IOCTL_MFC_H264_ENC_INIT				(0x00800006)
#define IOCTL_MFC_H264_DEC_EXE				(0x00800007)
#define IOCTL_MFC_H264_ENC_EXE				(0x00800008)

#define IOCTL_MFC_H263_DEC_INIT				(0x00800009)
#define IOCTL_MFC_H263_ENC_INIT				(0x0080000A)
#define IOCTL_MFC_H263_DEC_EXE				(0x0080000B)
#define IOCTL_MFC_H263_ENC_EXE				(0x0080000C)

#define IOCTL_MFC_VC1_DEC_INIT				(0x0080000D)
#define IOCTL_MFC_VC1_DEC_EXE				(0x0080000E)

#define IOCTL_MFC_GET_LINE_BUF_ADDR			(0x0080000F)
#define IOCTL_MFC_GET_RING_BUF_ADDR			(0x00800010)
#define IOCTL_MFC_GET_FRAM_BUF_ADDR			(0x00800011)
#define IOCTL_MFC_GET_POST_BUF_ADDR			(0x00800012)
#define IOCTL_MFC_GET_PHY_FRAM_BUF_ADDR		(0x00800013)
#define IOCTL_MFC_GET_CONFIG				(0x00800016)
#define IOCTL_MFC_GET_MPEG4_ASP_PARAM		(0x00800017)

#define IOCTL_MFC_SET_H263_MULTIPLE_SLICE	(0x00800014)
#define IOCTL_MFC_SET_CONFIG				(0x00800015)

#define IOCTL_MFC_GET_DBK_BUF_ADDR			(0x00800018)	// yj

#define MFCDRV_RET_OK						(0)
#define MFCDRV_RET_ERR_INVALID_PARAM		(-1001)
#define MFCDRV_RET_ERR_HANDLE_INVALIDATED	(-1004)
#define MFCDRV_RET_ERR_OTHERS				(-9001)

// RainAde : This size should be same as MfcConfig setting
#if 1
#define BUF_SIZE							0x4AB480 // RainAde: 832x512, instance 2, ref buf 6 (including DBK_BUF)
#else
#define BUF_SIZE							0x4DEC80 // yj: input and output buffer size (including DBK_BUF)
#define BUF_SIZE							0x373480 // RainAde: 832x512, instance 2, ref buf 4 (including DBK_BUF)
#endif


#define MFC_DEV_NAME						"/dev/s3c-mfc"

#endif /* __SAMSUNG_SYSLSI_APDEV_S3C_MFC_H__ */
