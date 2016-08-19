/******************************************************************************************
 ** �ļ����ƣ�ATR.h						                                                 **
 ** �ļ�������IC���ϵ縴λ��Ϣ�������		                                             **
 **--------------------------------------------------------------------------------------**
 ** �� �� �ˣ����ܿ�ҵ��                                                               **
 ** �������ڣ�2012-07-09                                                                 **
 **--------------------------------------------------------------------------------------**
 ** �� �� �ˣ�                                                                           **
 ** �޸����ڣ�                                                                           **
 **--------------------------------------------------------------------------------------**
 ** �� �� �ţ�V1.0                                                                       **
 **--------------------------------------------------------------------------------------**
 **                   Copyright (c) 2012  ftsafe                                         **
 ******************************************************************************************/
#include <windows.h>
#include "ATR.h"


#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif


// ����һ����̬����
static unsigned atr_num_ib_table[16] =
{
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
};

/****************************************************************************************** 
 *******    �������ƣ�ftATRInitFromArray		-��ţ�1 -							*******
 *******	�������ܣ���ATR�ֽ���ת����ATR�ṹ����									*******
 *******----------------------------------------------------------------------------******* 
 *******	����������																*******
 *******				ATRStruct *atr��(���)ת��ATR�ֽ������ɵ�ATR��Ϣ�ṹָ��	*******
 *******				unsigned char atr_buffer[]��ATR�ֽ�������(BCD��)			*******
 *******				unsigned      length��ATR�ֽ�������							*******
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�ATR_OK(0)��ʧ�ܷ��أ�<0	;�����������				******* 
  *******				ATR_MALFORMED(2)��ATR��������								*******
 ******************************************************************************************/
int ftATRInitFromArray (ATRStruct * atr, unsigned char  atr_buffer[ATR_MAX_SIZE], unsigned int length)
{
    BYTE TDi;
    unsigned pointer = 0, pn = 0;

    // ��⴫��� ATR ���Ȳ����Ϸ���  
    if (length < 2)
        return (ATR_MALFORMED);

    // ����ATR �ֽ����ĵ�1�ֽڣ�TS[��ʼ�ֽ�]  
    atr->TS = atr_buffer[0];

	// ����ATR �ֽ����ĵ�2�ֽڣ�T0[��ʽ�ַ�],��4BITΪ��TA1~TD1�Ƿ���ڣ���4BITΪ����ʷ�ֽ���*/
    atr->T0 = TDi = atr_buffer[1];
    pointer = 1;

    // ����T0 ��4BIT����ʷ�ֽ���  
    atr->hbn = TDi & 0x0F;

    // ȱʡ��TCK[У���ַ�] ������  
    (atr->TCK).present = FALSE;
  

    // ���ݳ���ѭ������ATR���������  
    while (pointer < length)
    {
        
        // ���ATR��������С  
        if (pointer + atr_num_ib_table[(0xF0 & TDi) >> 4] >= length)
        {
            return (ATR_MALFORMED);
        }
        
        // ���TA[i] �Ƿ����  
        if ((TDi | 0xEF) == 0xFF)
        {		// �洢 TA[i] ��ֵ
            pointer++;
            atr->ib[pn][ATR_INTERFACE_BYTE_TA].value = atr_buffer[pointer];
            atr->ib[pn][ATR_INTERFACE_BYTE_TA].present = TRUE;
        }
        else
            atr->ib[pn][ATR_INTERFACE_BYTE_TA].present = FALSE;
            
        // ���TB[i] �Ƿ���� 
        if ((TDi | 0xDF) == 0xFF)
        {		// �洢 TB[i] ��ֵ
            pointer++;
            atr->ib[pn][ATR_INTERFACE_BYTE_TB].value = atr_buffer[pointer];
            atr->ib[pn][ATR_INTERFACE_BYTE_TB].present = TRUE;
        }
        else
            atr->ib[pn][ATR_INTERFACE_BYTE_TB].present = FALSE;

        // ���TC[i] �Ƿ����  
        if ((TDi | 0xBF) == 0xFF)
        {		// �洢 TC[i] ��ֵ
            pointer++;
            atr->ib[pn][ATR_INTERFACE_BYTE_TC].value = atr_buffer[pointer];
            atr->ib[pn][ATR_INTERFACE_BYTE_TC].present = TRUE;
        }
        else
            atr->ib[pn][ATR_INTERFACE_BYTE_TC].present = FALSE;

        // ���TD[i] �Ƿ����  
        if ((TDi | 0x7F) == 0xFF)
        {
            pointer++;
            
            // ��ȡTDi��ֵ
            TDi = atr->ib[pn][ATR_INTERFACE_BYTE_TD].value = atr_buffer[pointer];
            atr->ib[pn][ATR_INTERFACE_BYTE_TD].present = TRUE;
            
            // TCK����
            (atr->TCK).present = ((TDi & 0x0F) != ATR_PROTOCOL_TYPE_T0);
            if (pn >= ATR_MAX_PROTOCOLS)
            		return (ATR_MALFORMED);
            pn++;
        }
        else
        {
            atr->ib[pn][ATR_INTERFACE_BYTE_TD].present = FALSE;
            break;
        }
    }// ATR����ѭ���������


    // TA~TD ���ݽṹ����  
    atr->pn = pn + 1;

    // ���ATR�е���ʷ�ֽ����ͺϷ��� 
    if (pointer + atr->hbn >= length)
        return (ATR_MALFORMED);

	// ������ʷ�ֽ��� 
    memcpy (atr->hb, atr_buffer + pointer + 1, atr->hbn);
    pointer += (atr->hbn);

    // ���TCK �Ƿ���ڣ������򱣴�TCK��ֵ  
    if ((atr->TCK).present)
    {

        if (pointer + 1 >= length)
            return (ATR_MALFORMED);

        pointer++;

        (atr->TCK).value = atr_buffer[pointer];
    }

	// ���� ATR�ĳ���
    atr->length = pointer + 1;
    
    // ���سɹ�
    return (ATR_OK);
}


/****************************************************************************************** 
 *******    �������ƣ�ftGetATRT					-��ţ�2 -							*******
 *******	�������ܣ���ȡATR�ṹ��TD1��TD2��Э������(ֵΪ��0~F)					*******
 *******----------------------------------------------------------------------------******* 
 *******	����������																*******
 *******				ATRStruct stATR����λӦ����Ϣ(ATR)�ṹ����					*******
 *******				int      *TD1T�� (���)ATR��TD1�����Э������,				*******
 *******                        ȱʡΪ��0����T=0,������TD1�ĵͰ��ֽ�Ϊ:0��1,		*******
 *******                int      *TD2T�� (���)ATR��TD2�����Э�����ͣ�				*******	
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�ATR_OK(0)��ʧ�ܷ��أ�<0	;�����������				******* 
 ******************************************************************************************/
int ftGetATRT(ATRStruct stATR, int *TD1T,int *TD2T)
{
		int nTD1T,nTD2T;
	
		// ȱʡֵΪ��T=0 
		nTD1T = 0; 
		
		// ȱʡֵΪ��TD2 ������
		nTD2T = -1;

		// ��ȡTD1�Ͱ��ֽڵ�ֵ��ȱʡ����T=0 
		if (stATR.ib[0][3].present == TRUE)  
		{
			// ȡTD1�ĵ�4λ��
			nTD1T = stATR.ib[0][3].value & 0x0F;
		}

		// ��ȡTD2�Ͱ��ֽڵ�ֵ
		if( stATR.ib[1][3].present == TRUE)
		{
			nTD2T = stATR.ib[1][3].value & 0x0F;
		}

		// �����������ֵ
		*TD1T = nTD1T;
		*TD2T = nTD2T;

		return ATR_OK;
}


/****************************************************************************************** 
 *******    �������ƣ�ftATRTCKCal				-��ţ�3 -							*******
 *******	�������ܣ�ATR�ֽ�����TCK���㺯��										*******
 *******----------------------------------------------------------------------------******* 
 *******	����������																*******
 *******				unsigned char atr_buffer[]��ATR�ֽ�������[BCD��]			*******
 *******				int           length      ��ATR�ֽ�������[��������]			*******
 *******                unsigned int  *TCK		  ��(���)�������ATR��TCKֵ		*******	
 *******----------------------------------------------------------------------------*******
 *******	�� �� ֵ���ɹ����أ�ATR_OK(0)��ʧ�ܷ��أ�<0	;�����������				******* 
 ******************************************************************************************/
int ftATRTCKCal(unsigned char  atr_buffer[ATR_MAX_SIZE],int length ,unsigned int *TCK)
{
		unsigned char nCH;
		int i ;

		nCH = atr_buffer[1];

		// TCK 
		for(i = 2; i< (length -1) ;i++)
		{
			nCH = (unsigned char )atr_buffer[i] ^ (unsigned char)nCH;
		}
	
		*TCK = nCH;

		return ATR_OK;
}


 
