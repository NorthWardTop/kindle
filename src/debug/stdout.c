
#include <config.h>
#include <debug_manager.h>
#include <stdio.h>
#include <string.h>


/**
 * "��׼�������ͨ��"���������
 * @strData: Ҫ��ӡ���ַ���������Ϣ
 * @return: ��ӡ�ĳ���
 */
static int StdoutDebugPrint(char *strData)
{
	/* ֱ�Ӱ������Ϣ��printf��ӡ���� */
	printf("%s", strData);
	return strlen(strData);	
}

static T_DebugOpr g_tStdoutDbgOpr = {
	.name       = "stdout",
	.isCanUse   = 1,                 /* 1��ʾ��ʹ���������������Ϣ */
	.DebugPrint = StdoutDebugPrint,  /* ��ӡ���� */
};

/**
 * ע���׼������Է���, �����ϲ�ע�����ѡ��ʹ��
 * @return: 0 �ɹ�, -1 ʧ��
 */
int StdoutInit(void)
{
	return RegisterDebugOpr(&g_tStdoutDbgOpr);
}

