#include <config.h>
#include <debug_manager.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

//����ѡ��ͷָ��
static PT_DebugOpr g_ptDebugOprHead;
//���Լ�����ڻ����g_iDbgLevelLimit�ĵ�����Ϣ�Ż��ӡ����
static int g_iDbgLevelLimit = 8;


/**
 * ע����Զ���: ������ѡ����뵽����ѡ���б�
 * @ptDebugOpr: ���Զ���ָ��
 * @return: 0
 */
int RegisterDebugOpr(PT_DebugOpr ptDebugOpr)
{
	PT_DebugOpr ptTmp;

	if (!g_ptDebugOprHead) {
		g_ptDebugOprHead   = ptDebugOpr;
		ptDebugOpr->ptNext = NULL;
	} else {
		ptTmp = g_ptDebugOprHead;
		while (ptTmp->ptNext)
			ptTmp = ptTmp->ptNext;

		ptTmp->ptNext	  = ptDebugOpr;
		ptDebugOpr->ptNext = NULL;
	}

	return 0;
}

/**
 * ��ʾ���е���ѡ��: ��ӡ����ѡ������
 */
void ShowDebugOpr(void)
{
	int i = 0;
	PT_DebugOpr ptTmp = g_ptDebugOprHead;

	while (ptTmp) {
		DBG_PRINTF("%02d %s\n", i++, ptTmp->name);
		ptTmp = ptTmp->ptNext;
	}
}

/**
 * ��ȡ����ѡ��: ��������ȡ��ָ����"����ģ��"��ַ����
 * @return: ��ȡ�ɹ����ط�NULL, ��ȡʧ�ܷ���NULL
 */
PT_DebugOpr GetDebugOpr(char *pcName)
{
	PT_DebugOpr ptTmp = g_ptDebugOprHead;
	
	while (ptTmp) {
		if (strcmp(ptTmp->name, pcName) == 0)
			return ptTmp;
		ptTmp = ptTmp->ptNext;
	}
	return NULL;
}


/**
 * ���ô�ӡ���Ƽ�: ����>=g_iDbgLevelLimit�����
 * @strBuf: ������"dbglevel=3"
 * @return: 0
 */
int SetDbgLevel(char *strBuf)
{
	g_iDbgLevelLimit = strBuf[9] - '0';
	return 0;
}

/**
 * �򿪻�رյ���ѡ��
 * @strBuf: �����������ַ���
 *                     stdout=0			   : �ر�stdout��ӡ
 *                     stdout=1			   : ��stdout��ӡ
 *                     netprint=0		   : �ر�netprint��ӡ
 *                     netprint=1		   : ��netprint��ӡ
 * @return: �ɹ�����0, ʧ�ܷ���-1
 */
int SetDbgChanel(char *strBuf)
{
	char *pStrTmp;
	char strName[100];
	PT_DebugOpr ptTmp;

	pStrTmp = strchr(strBuf, '=');
	if (!pStrTmp)
	{
		return -1;
	}
	else
	{
		/* �Ƚ�ȡ����, Ȼ����������ҵ�����ģ�� */
		strncpy(strName, strBuf, pStrTmp-strBuf);
		strName[pStrTmp-strBuf] = '\0';
		ptTmp = GetDebugOpr(strName);
		if (!ptTmp)
			return -1;

		if (pStrTmp[1] == '0')
			ptTmp->isCanUse = 0;
		else
			ptTmp->isCanUse = 1;
		return 0;
	}
	
}


/**
 * �����ṩ�Ĵ�ӡ�ӿں���: ��������DBG_PRINTF����ӡ, ������DebugPrint
 *            ��config.h���������ĺ궨��: #define DBG_PRINTF DebugPrint
 * @pcFormat: ͬprintf, ��ʽ���ַ���
 * @...: ��ʽ���ַ�������
 * @return: 0 - �ɹ�
 * 			-1 - ʧ��
 */
int DebugPrint(const char *pcFormat, ...)
{
	char strTmpBuf[1024];
	char *pcTmp;
	va_list tArg;
	int iNum;
	PT_DebugOpr ptTmp = g_ptDebugOprHead;
	int dbglevel = DEFAULT_DBGLEVEL;

	/* �ɱ�����Ĵ���, ����glibc��printf���� */
	va_start (tArg, pcFormat);
	iNum = vsprintf (strTmpBuf, pcFormat, tArg);
	va_end (tArg);
	strTmpBuf[iNum] = '\0'; //��ʽ��ת������ַ���


	pcTmp = strTmpBuf;
	
	/* ���ݴ�ӡ��������Ƿ��ӡ */
    /*SetDbgLevel =6 */
	if ((strTmpBuf[0] == '<') && (strTmpBuf[2] == '>')) {
		dbglevel = strTmpBuf[1] - '0';
		if (dbglevel >= 0 && dbglevel <= 9)
			pcTmp = strTmpBuf + 3; //�ڴ�ӡ�����ھ�ȥ�� "<3>"
		else
			dbglevel = DEFAULT_DBGLEVEL; //���ڴ�ӡ�����ھ�Ĭ��ֵ
	}
	//���ڴ�ӡ����ֱ�ӷ���, ���ô�ӡ
	if (dbglevel > g_iDbgLevelLimit)
		return -1;

	/* ��������������isCanUseΪ1�Ľṹ���DebugPrint���� 
	 * �������������Ϣ
	 */
	while (ptTmp) {
		if (ptTmp->isCanUse)
			ptTmp->DebugPrint(pcTmp);
		ptTmp = ptTmp->ptNext;
	}

	return 0;
}


/**
 * ��ʼ��������Ҫ��ʼ���Ĵ�ӡѡ��ͨ��, ���������ӡ�İ󶨶˿�
 * @return: 0
 */
int InitDebugChanel(void)
{
	PT_DebugOpr ptTmp = g_ptDebugOprHead;
	while (ptTmp) {
		//���ش򿪲�����Ҫ��ʼ���Ĵ�ӡͨ��
		if (ptTmp->isCanUse && ptTmp->DebugInit)
			ptTmp->DebugInit();
		ptTmp = ptTmp->ptNext;
	}

	return 0;
}


/**
 * ע�����д�ӡѡ��
 * @return: 0
 */
int DebugInit(void)
{
	int iError;
	//����������ӡѡ��
	iError = StdoutInit();
	iError |= NetPrintInit();
	return iError;
}

