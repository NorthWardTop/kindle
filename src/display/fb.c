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
 * 显示设备初始化
 * 		1. 获取frambuffer的固定信息和可变信息
 * 		2. 映射设备文件句柄到内存
 * 		3. 设置全局显示选项
 * @return: 0 成功, -1 失败
 */
static int FBDeviceInit(void)
{
	int ret;
	
	g_fd = open(FB_DEVICE_NAME, O_RDWR);

	if (0 > g_fd)
		DBG_PRINTF("can't open %s\n", FB_DEVICE_NAME);
	//获取Framebuffer有关的可变信息
	ret = ioctl(g_fd, FBIOGET_VSCREENINFO, &g_tFBVar);
	if (ret < 0) {
		DBG_PRINTF("can't get fb's var\n");
		return -1;
	}

	//获取frambuffer固定信息
	ret = ioctl(g_fd, FBIOGET_FSCREENINFO, &g_tFBFix);
	if (ret < 0) {
		DBG_PRINTF("can't get fb's fix\n");
		return -1;
	}
	// x*y*每个像素字节数
	g_dwScreenSize = g_tFBVar.xres * g_tFBVar.yres * g_tFBVar.bits_per_pixel / 8;
	//映射设备文件/屏幕大小的内存, SHARED是映射的对象与内存同步变化, 起始偏移0
	g_pucFBMem = (unsigned char *)mmap(NULL, g_dwScreenSize, PROT_READ | PROT_WRITE, MAP_SHARED, g_fd, 0);
	if (0 > g_pucFBMem)	{
		DBG_PRINTF("can't mmap\n");
		return -1;
	}

	g_tFBOpr.iXres       = g_tFBVar.xres;
	g_tFBOpr.iYres       = g_tFBVar.yres;
	g_tFBOpr.iBpp        = g_tFBVar.bits_per_pixel;
	g_tFBOpr.iLineWidth  = g_tFBVar.xres * g_tFBOpr.iBpp / 8; //一行的字节数
	g_tFBOpr.pucDispMem  = g_pucFBMem; //映射的内存指针, 显示设备/显存

	g_dwLineWidth  = g_tFBVar.xres * g_tFBVar.bits_per_pixel / 8; //一行的字节数
	g_dwPixelWidth = g_tFBVar.bits_per_pixel / 8; //一个像素的字节数
	
	return 0;
}


/**
 * 设置指定像素位某个颜色
 * @iX: 横坐标x像素
 * @iY: 纵坐标y像素
 * @dwColor: 颜色值, 格式为32Bpp,即0x00RRGGBB
 * @return:　０
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

	//排除参数超出屏幕范围
	if ((iX >= g_tFBVar.xres) || (iY >= g_tFBVar.yres)) {
		DBG_PRINTF("out of region\n");
		return -1;
	}

	//计算相应bpp的地址
	pucFB      = g_pucFBMem + g_dwLineWidth * iY + g_dwPixelWidth * iX;//单字节指针
	pwFB16bpp  = (unsigned short *)pucFB;//2字节指针
	pdwFB32bpp = (unsigned int *)pucFB;//4字节指针
	
	//根据bpp将32位颜色值转化位8/16位
	switch (g_tFBVar.bits_per_pixel) {
		case 8: {
			//指针强转为
			*pucFB = (unsigned char)dwColor;
			break;
		}
		case 16: {
			/* 从dwBackColor中取出红绿蓝三原色,
			 * 构造为16Bpp的颜色 */
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
 * 显示ptVideoMem的整屏内存数据
 * @ptVideoMem: 包含一个屏幕数据的内存
 * @return: 0
 */
static int FBShowPage(PT_VideoMem ptVideoMem)
{
	memcpy(g_tFBOpr.pucDispMem, ptVideoMem->tPixelDatas.aucPixelDatas, ptVideoMem->tPixelDatas.iTotalBytes);
	return 0;
}


/**
 * 将屏幕清除位dwBackColor颜色
 * @dwBackColor: 清屏颜色
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
			/* 从dwBackColor中取出红绿蓝三原色,
			 * 构造为16Bpp的颜色
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
 * fd显示选项对象
 */
static T_DispOpr g_tFBOpr = {
	.name        = "fb",
	.DeviceInit  = FBDeviceInit,
	.ShowPixel   = FBShowPixel,
	.CleanScreen = FBCleanScreen,
	.ShowPage    = FBShowPage,
};



/**
 * 注册fb设备, 将fd设备显示选项加入管理链表
 */
int FBInit(void)
{
	return RegisterDispOpr(&g_tFBOpr);
}


