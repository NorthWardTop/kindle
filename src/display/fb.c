#include <config.h>
#include <disp_manager.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <string.h>

// static int FBDeviceInit(void);
// static int FBShowPixel(int iX, int iY, unsigned int dwColor);
// static int FBCleanScreen(unsigned int dwBackColor);
// static int FBShowPage(PT_VideoMem ptVideoMem);


static int g_fd;

static struct fb_var_screeninfo g_tFBVar;
static struct fb_fix_screeninfo g_tFBFix;			
static unsigned char *g_pucFBMem;
static unsigned int g_dwScreenSize;

static unsigned int g_dwLineWidth;
static unsigned int g_dwPixelWidth;



/**
 * ��ʾ�豸��ʼ��
 * 		1. ��ȡframbuffer�Ĺ̶���Ϣ�Ϳɱ���Ϣ
 * 		2. ӳ���豸�ļ�������ڴ�
 * 		3. ����ȫ����ʾѡ��
 * @return: 0 �ɹ�, -1 ʧ��
 */
static int FBDeviceInit(void)
{
	int ret;
	
	g_fd = open(FB_DEVICE_NAME, O_RDWR);

	if (0 > g_fd)
		DBG_PRINTF("can't open %s\n", FB_DEVICE_NAME);
	//��ȡFramebuffer�йصĿɱ���Ϣ
	ret = ioctl(g_fd, FBIOGET_VSCREENINFO, &g_tFBVar);
	if (ret < 0) {
		DBG_PRINTF("can't get fb's var\n");
		return -1;
	}

	//��ȡframbuffer�̶���Ϣ
	ret = ioctl(g_fd, FBIOGET_FSCREENINFO, &g_tFBFix);
	if (ret < 0) {
		DBG_PRINTF("can't get fb's fix\n");
		return -1;
	}
	// x*y*ÿ�������ֽ���
	g_dwScreenSize = g_tFBVar.xres * g_tFBVar.yres * g_tFBVar.bits_per_pixel / 8;
	//ӳ���豸�ļ�/��Ļ��С���ڴ�, SHARED��ӳ��Ķ������ڴ�ͬ���仯, ��ʼƫ��0
	g_pucFBMem = (unsigned char *)mmap(NULL, g_dwScreenSize, PROT_READ | PROT_WRITE, MAP_SHARED, g_fd, 0);
	if (0 > g_pucFBMem)	{
		DBG_PRINTF("can't mmap\n");
		return -1;
	}

	g_tFBOpr.iXres       = g_tFBVar.xres;
	g_tFBOpr.iYres       = g_tFBVar.yres;
	g_tFBOpr.iBpp        = g_tFBVar.bits_per_pixel;
	g_tFBOpr.iLineWidth  = g_tFBVar.xres * g_tFBOpr.iBpp / 8; //һ�е��ֽ���
	g_tFBOpr.pucDispMem  = g_pucFBMem; //ӳ����ڴ�ָ��, ��ʾ�豸/�Դ�

	g_dwLineWidth  = g_tFBVar.xres * g_tFBVar.bits_per_pixel / 8; //һ�е��ֽ���
	g_dwPixelWidth = g_tFBVar.bits_per_pixel / 8; //һ�����ص��ֽ���
	
	return 0;
}


/**
 * ����ָ������λĳ����ɫ
 * @iX: ������x����
 * @iY: ������y����
 * @dwColor: ��ɫֵ, ��ʽΪ32Bpp,��0x00RRGGBB
 * @return:����
 */
static int FBShowPixel(int iX, int iY, unsigned int dwColor)
{
	unsigned char *pucFB;
	unsigned short *pwFB16bpp;
	unsigned int *pdwFB32bpp;
	unsigned short wColor16bpp; /* 565 */
	int iRed;
	int iGreen;
	int iBlue;

	//�ų�����������Ļ��Χ
	if ((iX >= g_tFBVar.xres) || (iY >= g_tFBVar.yres)) {
		DBG_PRINTF("out of region\n");
		return -1;
	}

	//������Ӧbpp�ĵ�ַ
	pucFB      = g_pucFBMem + g_dwLineWidth * iY + g_dwPixelWidth * iX;//���ֽ�ָ��
	pwFB16bpp  = (unsigned short *)pucFB;//2�ֽ�ָ��
	pdwFB32bpp = (unsigned int *)pucFB;//4�ֽ�ָ��
	
	//����bpp��32λ��ɫֵת��λ8/16λ
	switch (g_tFBVar.bits_per_pixel) {
		case 8: {
			//ָ��ǿתΪ
			*pucFB = (unsigned char)dwColor;
			break;
		}
		case 16: {
			/* ��dwBackColor��ȡ����������ԭɫ,
			 * ����Ϊ16Bpp����ɫ */
			iRed   = (dwColor >> (16+3)) & 0x1f;
			iGreen = (dwColor >> (8+2)) & 0x3f;
			iBlue  = (dwColor >> 3) & 0x1f;
			wColor16bpp = (iRed << 11) | (iGreen << 5) | iBlue;
			*pwFB16bpp	= wColor16bpp;
			break;
		}
		case 32: {
			*pdwFB32bpp = dwColor;
			break;
		}
		default: {
			DBG_PRINTF("can't support %d bpp\n", g_tFBVar.bits_per_pixel);
			return -1;
		}
	}

	return 0;
}


/**
 * ��ʾptVideoMem�������ڴ�����
 * @ptVideoMem: ����һ����Ļ���ݵ��ڴ�
 * @return: 0
 */
static int FBShowPage(PT_VideoMem ptVideoMem)
{
	memcpy(g_tFBOpr.pucDispMem, ptVideoMem->tPixelDatas.aucPixelDatas, ptVideoMem->tPixelDatas.iTotalBytes);
	return 0;
}


/**
 * ����Ļ���λdwBackColor��ɫ
 * @dwBackColor: ������ɫ
 * @return: 0
 */
static int FBCleanScreen(unsigned int dwBackColor)
{
	unsigned char *pucFB;
	unsigned short *pwFB16bpp;
	unsigned int *pdwFB32bpp;
	unsigned short wColor16bpp; /* 565 */
	int iRed;
	int iGreen;
	int iBlue;
	int i = 0;

	pucFB      = g_pucFBMem;
	pwFB16bpp  = (unsigned short *)pucFB;
	pdwFB32bpp = (unsigned int *)pucFB;

	switch (g_tFBVar.bits_per_pixel)
	{
		case 8: {
			memset(g_pucFBMem, dwBackColor, g_dwScreenSize);
			break;
		}
		case 16: {
			/* ��dwBackColor��ȡ����������ԭɫ,
			 * ����Ϊ16Bpp����ɫ
			 */
			iRed   = (dwBackColor >> (16+3)) & 0x1f;
			iGreen = (dwBackColor >> (8+2)) & 0x3f;
			iBlue  = (dwBackColor >> 3) & 0x1f;
			wColor16bpp = (iRed << 11) | (iGreen << 5) | iBlue;
			while (i < g_dwScreenSize) {
				*pwFB16bpp	= wColor16bpp;
				pwFB16bpp++;
				i += 2;
			}
			break;
		}
		case 32: {
			while (i < g_dwScreenSize) {
				*pdwFB32bpp	= dwBackColor;
				pdwFB32bpp++;
				i += 4;
			}
			break;
		}
		default: {
			DBG_PRINTF("can't support %d bpp\n", g_tFBVar.bits_per_pixel);
			return -1;
		}
	}

	return 0;
}


/**
 * fd��ʾѡ�����
 */
static T_DispOpr g_tFBOpr = {
	.name        = "fb",
	.DeviceInit  = FBDeviceInit,
	.ShowPixel   = FBShowPixel,
	.CleanScreen = FBCleanScreen,
	.ShowPage    = FBShowPage,
};



/**
 * ע��fb�豸, ��fd�豸��ʾѡ������������
 */
int FBInit(void)
{
	return RegisterDispOpr(&g_tFBOpr);
}


