
static unsigned short crc16_ccitt_table[256] =
{
0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

//#define CRC_INIT 0xffff   //CCITT��ʼCRCΪȫ1
//#define GOOD_CRC 0xf0b8   //У��ʱ������Ĺ̶����ֵ
/*****CRC���㺯��,�ɽ���һ������reg_init�򻯵�******** crc16_ccitt_table ��stdafx.h�� */
static unsigned short do_crc(unsigned short reg_init, unsigned char *message, unsigned int len)
{
    unsigned short crc_reg = reg_init;
         
    while (len--)
        crc_reg = (crc_reg >> 8) ^ crc16_ccitt_table[(crc_reg ^ *message++) & 0xff];

	crc_reg ^= 0xffff;     //����ȡ����һ��Ҫ������һ����?       
    return crc_reg;
} 

#define FR_TYPE_RSU 0
#define FR_TYPE_OBU 1
#define FR_TYPE_GB_RSU 2
#define FR_TYPE_GB_OBU 3
#define FR_TYPE_14K 4

#define UP_EDGE 0
#define DOWN_EDGE 1

typedef struct _DB44_WAVE {
	int start_width_std;
	int bit_1_wd_std;
	int bit_0_wd_std;
	int end_width_std;

	int multi;	/* ��1us(1΢��)Ϊ��λ�� ���������nMHz�������Ϊn�� Ҳ����˵�� 1us(1΢��)����1��, ��ֵΪ1; 1us(1΢��)����n��, ��ֵΪn */
	unsigned char dsrc_type, edge_type;
/*
int pre_define_type		���룺FR_TYPE_RSU�� ���ر�RSU��������;
							FR_TYPE_OBU�� ���ر�OBU��������;
*/
	int mode(unsigned char pre_define_type) 
	{
		if ( pre_define_type == FR_TYPE_RSU )
		{
			dsrc_type = FR_TYPE_RSU;
		} else if ( pre_define_type == FR_TYPE_OBU)
		{
			dsrc_type = FR_TYPE_OBU;
		} else 
			return 0;

		switch ( dsrc_type ) 
		{
		case FR_TYPE_RSU :
			bit_1_wd_std = 80	*multi;
			bit_0_wd_std = 60*multi;	
			end_width_std = 120	*multi;
			edge_type = DOWN_EDGE;
			break;

		case FR_TYPE_OBU :
			start_width_std = 20*multi;
			bit_1_wd_std = 11	*multi;
			bit_0_wd_std = 7	*multi;	
			end_width_std = 20	*multi;
			edge_type = UP_EDGE;

			break;
		}
		return 1;
	};
/* 
������������
unsigned short *levarr, �����ƽֵ����
int &lev_num,			���ֵ����
unsigned char *in,		���룺ͨ���ֽ�����
int in_num				���룺ͨ���ֽڸ���
unsigned short high		���룺�ߵ�ƽֵ
unsigned short low		���룺�͵�ƽֵ
*/
	int generate( unsigned short *levarr, int &lev_num, unsigned char *in, int in_num, unsigned short high, unsigned short low)
	{
		unsigned short *plev;
		unsigned char *pin;
		int max_num, i,j,k, lev_wid;
		unsigned short bit_1_front_lev_wid, bit_1_back_lev_wid;
		unsigned short bit_0_front_lev_wid, bit_0_back_lev_wid;
		unsigned short front_lev, back_lev;	//ǰһ����ƽֵ����һ����ƽֵ, ����һ������

		plev = levarr;
		max_num = lev_num;
		lev_num = 0;	

		/* �ȸ�50��us�ĵ͵�ƽ */
		for ( i = 0 ; i < 50*multi; i++, plev++) *plev = low;
		lev_num += (50*multi);

		if ( this->edge_type == DOWN_EDGE) //�������½����жϵģ�
		{
			/* Ҫ׼��ǰ���ߵ�ƽ */
			lev_wid = start_width_std;
			for ( i = 0 ; i < lev_wid; i++, plev++) *plev = high;
			lev_num += lev_wid;

			front_lev = low; //�ȵ͵�ƽ
			back_lev = high; //��ߵ�ƽ
		} else if ( this->edge_type == UP_EDGE) //�������������жϵģ�
		{
			front_lev = high;	//�ȸߵ�ƽ
			back_lev = low;	//��͵�ƽ
		}

		/* ׼��ǰ������ */
		if ( dsrc_type ==FR_TYPE_RSU )
		{
			lev_wid = bit_0_wd_std;
		} else if ( dsrc_type == FR_TYPE_OBU)
		{
			lev_wid = bit_1_wd_std;
		} 

		for ( i = 0 ; i < lev_wid; i++, plev++) *plev = front_lev;
		lev_num += lev_wid;

		for ( i = 0 ; i < lev_wid; i++, plev++) *plev = back_lev;
		lev_num += lev_wid;

		/* ǰ�����ν��� */

		bit_1_front_lev_wid = bit_1_wd_std/2;
		bit_1_back_lev_wid = bit_1_wd_std-bit_1_front_lev_wid;

		bit_0_front_lev_wid = bit_0_wd_std/2;
		bit_0_back_lev_wid = bit_0_wd_std - bit_0_front_lev_wid;

		/* ׼��λԪ���� */
		for ( i = 0,pin=in; i < in_num; i++, pin++)
		{
			unsigned char tch=*pin;
			for ( j = 0 ; j < 8 ; j++)
			{
				if ( (tch & 0x01) == 1 ) 
				{
					for ( k = 0 ; k < bit_1_front_lev_wid; k++, plev++) *plev = front_lev;
					lev_num += bit_1_front_lev_wid;

					for ( k = 0 ; k < bit_1_back_lev_wid; k++, plev++) *plev = back_lev;
					lev_num += bit_1_back_lev_wid;
				} else {
					for ( k = 0 ; k < bit_0_front_lev_wid; k++, plev++) *plev = front_lev;
					lev_num += bit_0_front_lev_wid;

					for ( k = 0 ; k < bit_0_back_lev_wid; k++, plev++) *plev = back_lev;
					lev_num += bit_0_back_lev_wid;
				}
				tch=tch >> 1;
			}
		}
		/* ׼��λԪ���ν��� */

		/* ׼���󵼲��� */
		for ( i = 0 ; i < lev_wid; i++, plev++) *plev = front_lev;
		lev_num += lev_wid;

		for ( i = 0 ; i < lev_wid; i++, plev++) *plev = back_lev;
		lev_num += lev_wid;
		/* �󵼲��ν��� */

		if ( edge_type == UP_EDGE) //���������жϵģ�Ҫ׼���󵼸ߵ�ƽ
		{
			lev_wid = end_width_std;
			for ( i = 0 ; i < lev_wid; i++, plev++) *plev = high;
			lev_num += lev_wid;
		}

		/* ��50��us�ĵ͵�ƽ */
		for ( i = 0 ; i < 50*multi; i++, plev++) *plev = low;
		lev_num += (50*multi);

		return lev_num;
	};

} DB44_WAVE;

typedef struct _DB_WAVE {
	bool started; /*waveform started? to be initialized false. 
		A frame is ready  when it is false and len > 0.  */
	bool ended;
	bool wait_to_change;       /* wait to change level */
	int tail;
	int pos;
	bool has_data; /*to be initialized false,  true:data starting, false:not yet */
	int length;	/* to be initialized 0 */
	unsigned char data[1024];  //�洢����
	bool crc_ok;

	char suspect_msg[10240];
	bool report_msg;

	int start_at, data_start_at, end_at;
	int guide_width; //ǰ���������, ��RSU��120us�� ��OBU��20us��
	int first_high_len;	//��һ���ߵ�ƽ��ά��ʱ�䣬�����RSU������


	unsigned char last_logic; //��һ���߼���ƽ�� 
	int last_down_edge, last_up_edge;
	int pre_edge;


	int type_width,type_wid_low;		/* �ӵ�һ���ߵ�ƽ����ж���RSU����, ����OBU���� */
	int wav_end_low_width; //����͵�ƽ�������ʱ�䣬��֡����
	int multi;	/* ��1us(1΢��)Ϊ��λ�� ���������nMHz�������Ϊn�� Ҳ����˵�� 1us(1΢��)����1��, ��ֵΪ1; 1us(1΢��)����n��, ��ֵΪn */
	bool try_no_guide_rsu;
	double rsu_delta, obu_delta;

	int start_width_max;
	int start_width_min;
	int start_width_std;
	int bit_1_wd_max;
	int bit_1_wd_std;
	int bit_1_wd_min;
	int bit_0_wd_max;
	int bit_0_wd_std;
	int bit_0_wd_min;
	int end_width_max;
	int end_width_min;
	int end_width_std;
	unsigned char dsrc_type, edge_type;

	void pre_set (int fmulti,bool no_guide, bool report) {
		wav_end_low_width = 100 * fmulti;	//100us�͵�ƽ����Ϊ���ν���
		type_width = 25	*fmulti;	//RSU����30����OBU����20
		type_wid_low = 2	*fmulti;	//ǰ���ߵ�ƽ��ʱ�����ֵ,����̫����
		multi = fmulti;
		try_no_guide_rsu = no_guide;
		report_msg = report;
	};

	void init ()
	{
		started = false;
		ended = false;
		wait_to_change = false;
		tail = 0;
		pos = -2;
		has_data = false;
		length = 0;
		crc_ok = false;
		suspect_msg[0] = 0;

		last_logic = 0;
		last_down_edge = -1;
		last_up_edge = -1;
		pre_edge = -1;
	};

	void suspect(int val, int bit, int when_at)
	{
		bool r = false;
		if ( !report_msg ) return ;

		switch ( dsrc_type ) 
		{
		case FR_TYPE_RSU :
			if (bit == 1 )
			{
				if  ( val > (bit_1_wd_std + 10*multi) || val <= (bit_1_wd_std - 4*multi)  ) 
				r = true;
			}
			if (bit == 0 )
			{
				if  ( val >= (bit_0_wd_std + 4*multi) || val < (bit_0_wd_std - 8*multi)  ) 
				r = true;
			}

			break;

		case FR_TYPE_OBU :
			if (bit == 1 )
			{
				if  ( val >= (bit_1_wd_std + 0.5*multi) || val <= (bit_1_wd_std - 0.5*multi)  ) 
				r = true;
			}
			if (bit == 0 )
			{
				if  ( val >= (bit_0_wd_std + 0.5*multi) || val <= (bit_0_wd_std - 0.5*multi)  ) 
				r = true;
			}

			break;
		default:

			break;
		}
		if (  r ) 
		{ 
			char tmp[32];
			sprintf_s(tmp, "%d(%d,%d) ", val, bit, when_at);
			//sprintf_s(tmp, "%d(%d)", val, bit);
			strcat_s(this->suspect_msg, tmp);
		}
	};

	void mode() 
	{
		switch ( dsrc_type ) 
		{
		case FR_TYPE_RSU :
			start_width_max = 160*multi;
			start_width_min = 105*multi;
			start_width_std = 120*multi;
			bit_1_wd_max = 160	*multi;
			bit_1_wd_std = 80	*multi;
			bit_1_wd_min = 75	*multi;
			bit_0_wd_max = 74	*multi;	
			if ( bit_1_wd_min - bit_0_wd_max >= 2 ) 
				bit_1_wd_min -= (multi-1);
			bit_0_wd_std = 60*multi;	
			bit_0_wd_min = 30	*multi;	
			end_width_max = 160	*multi;
			end_width_min = 105	*multi;
			end_width_std = 120	*multi;
			edge_type = DOWN_EDGE;

			break;

		case FR_TYPE_OBU :
			start_width_max = 25*multi;
			start_width_min = 15*multi;
			start_width_std = 20*multi;
			bit_1_wd_max = 18	*multi;
			bit_1_wd_std = 11	*multi;
			bit_1_wd_min = 9	*multi;
			bit_0_wd_max = 8	*multi;
			if ( bit_1_wd_min - bit_0_wd_max >= 2 ) 
				bit_1_wd_min -= (multi-1);
			bit_0_wd_std = 7	*multi;	
			bit_0_wd_min = 3	*multi;	
			end_width_max = 25	*multi;
			end_width_min = 15	*multi;
			end_width_std = 20	*multi;
			edge_type = UP_EDGE;

			break;
		default:
			break;
		}
	}

	int feed_point  ( unsigned char cur_logic, long point,  int pre_define_type)  
	{
		int fret=0; //0:δ���һ֡�� 5:һ֡�Ѿ�����, �����͵�ƽ̫��������� 6: �͵�ƽ̫����Ӧ�ý���,������һ��֡��ʼ��
		if ( wait_to_change )   //���Ƕ���OBU��ǰ�����ݶ��Ѿ�OK�ˡ�
		{
			if( cur_logic != last_logic )  //
			{
				tail = point - end_at ;
				wait_to_change = false;
				fret = 5;
			} 
			goto NEXT;
		}
 
		if ( last_down_edge > 0 && (cur_logic == 0|| (last_logic == 0 && cur_logic == 1))  && point > ( last_down_edge + wav_end_low_width) )
		{
			end_at = last_down_edge;
			ended = true;
			if (last_logic == 0 && cur_logic == 1)  
				fret = 6;
			else
				fret = 5;	
			goto NEXT;
		}

		if ( last_logic == cur_logic) 
			goto NEXT;

		if ( !started )	//wait a start level.
		{
			if (last_logic==0 && cur_logic==1 )	//ǰ���ߵ�ƽ��������
			{
				started = true;
				has_data = false;
				start_at = point;
				pre_edge = -1;	//��û�ж�
				tail = 0;
			}		
			goto NEXT;
		}

		if ( last_logic == 1 && cur_logic == 0 )  //�½���
		{
			last_down_edge = point;
		} else if (last_logic == 0 && cur_logic == 1) {
			last_down_edge =  -1;		//���������ˣ��½���ȡ����
			last_up_edge = point;
		}

		/* here, frame has started */
		if ( pre_edge < 0 )
		{
			if ( last_logic==1 && cur_logic==0 ) //ǰ���ߵ�ƽ���½���
			{
				int pre_wid = point - start_at; //ǰ���ߵ�ƽ�Ŀ��
				if ( last_up_edge > 0 ) 
					pre_wid = point - last_up_edge;
				first_high_len = pre_wid;
				if (pre_wid <= type_wid_low ) 
				{	//����
					if ( report_msg )
					{
						char tmp[32];
						sprintf_s(tmp, "dis(%d,%d) ", pre_wid, point);
						strcat_s(this->suspect_msg, tmp);
					}
				}

				if (  pre_define_type == FR_TYPE_OBU ) {
					dsrc_type = FR_TYPE_OBU;
					pre_edge = start_at;	//����OBU,�������������,��Ϊ����ͷ
				} else 	if (  pre_define_type == FR_TYPE_RSU  || pre_wid > type_width )
				{
					dsrc_type = FR_TYPE_RSU;
					pre_edge = point;	//����RSU,���½��������,��Ӵ˿�ʼ
				} else  {
					dsrc_type = FR_TYPE_OBU;
					pre_edge = start_at;	//����OBU,�������������,��Ϊ����ͷ
				}

				mode();
				if ( try_no_guide_rsu  && pre_define_type==FR_TYPE_RSU && pre_wid*2 >= start_width_min && pre_wid*2 <= start_width_max )
				{	/* ����ǰ���ߵ�ƽ��ʧ�ĳ��� data start  */
					first_high_len = -1*multi;
					has_data = true;
					guide_width = pre_wid;
					pos = 0;
					data_start_at = point - pre_wid;
				}
			}
			goto NEXT;
		}

		if ( (edge_type == DOWN_EDGE && last_logic==1 && cur_logic==0 )
			|| (edge_type == UP_EDGE && last_logic==0 && cur_logic==1 )
			)
		{ /* encouter a edge , has a previous edge */
			int cur_edge = point;
			int m_interv = cur_edge - pre_edge;
			if ( has_data )
			{
				unsigned char &ab = data[pos/8];
				unsigned char bit = 0x01 << (pos % 8);
				if ( m_interv >= end_width_min && m_interv <= end_width_max )
				{	/* wave end */
					unsigned short crc;
					unsigned char cbarr[2];

					if( edge_type == UP_EDGE) // �����OBU����������Ϊ�ж�ʱ���, ��Ҫ�ȵ��͵�ƽ���㲨�ν���
					{
						wait_to_change = true;
					}
					end_at = point;
					length = pos / 8;
					started = false;
					ended = true;
					
//					sprintf_s(msg, "RSU data start at %d", point);
//					OutputDebugString(msg);
					if ( length > 2 )
					{
						crc = do_crc(0xffff, data, length-2);//��CRC
						cbarr[0] = crc&0x00ff;   //���ֽ���ǰ
						cbarr[1] = crc>>8&0x00ff; //���ֽ��ں�
						this->crc_ok = ( cbarr[0] == data[length-2] && cbarr[1] ==  data[length-1] );
					}
					goto NEXT;
				}

				if ( m_interv >= bit_1_wd_min && m_interv <= bit_1_wd_max )
				{
//					sprintf_s(msg, "RSU bit 1 end at %d", point);
//					OutputDebugString(msg);
					suspect(m_interv, 1, point);
					ab = ab | bit;
				} else if ( m_interv >= bit_0_wd_min && m_interv <= bit_0_wd_max )
				{
//					sprintf_s(msg, "RSU bit 0 end at %d", point);
//					OutputDebugString(msg);
					suspect(m_interv, 0, point);
					bit = ~bit;
					ab = ab & bit;
				} else {
					if ( this->report_msg ) 
					{
						char tmp[32];
						sprintf_s(tmp,  "err_wid(%d,%d) ", m_interv, point); 							
//						OutputDebugString(tmp);
						strcat_s(this->suspect_msg, tmp);
					}
					pos--;
				}
				pos++;
			} else {
				if ( m_interv >= start_width_min && m_interv <= start_width_max )
				{	/* data start  */
					has_data = true;
					guide_width = m_interv;
					pos = 0;
					data_start_at = point - guide_width;
					fret = 1; //����DB44֡�ˡ�
				} 
			}
			pre_edge = cur_edge;
		}
		
NEXT:
		last_logic = cur_logic;
		return fret;
	};

} DB_WAVE;

typedef struct _GB_DATA {
	short c_14K;	//14K��������
	bool started ;
	bool ended;

	int len;	/* to be initialized 0 */
	unsigned char data[1024];
	bool crc_ok;

	unsigned char cur; //��ǰ�ֽ�
	int bit_pos;	//��ǰ�ֽ�����һbitҪռ��λ��
	int set_bit_cont; //����1bit����Ŀ��

	int start_bit_at ; //�ڶ���λ��ʼʵ�ʵ�����
	int before_guide_bit_num ; //ǰ��0����֮ǰ��λ��, ��Ȼ��ǰ��0������16λ��
	int post_guide_bit_num;	//�󵼱��ص�λ��

	int guided_num;	//ǰ��16��0��
	int  put_bit ( unsigned char bt, int when_at)
	{
		int ret = 0;
		if ( guided_num < 16) 
		{
			if ( bt==0 ) 
				guided_num++;
			else {
				before_guide_bit_num += guided_num; //����ǰ0�����ӹ���
				before_guide_bit_num++;
				guided_num =0 ; //һ���ֲ���0, ���������λ	
			}
			goto END;
		}

		if ( ended ) 
		{
			post_guide_bit_num++;
			goto END;
		}

		if ( bt == 0 && set_bit_cont ==5 )
			goto NEXT; //��Ҫ

		if ( bt == 1 && set_bit_cont ==  5  ) //�Ѿ�����5��1��
		{
			goto NEXT;//�ٿ���һ��
		}

		if ( set_bit_cont == 6 )  //6��1��
		{
			if ( bt == 0 ) 
			{
				if ( started )  //�Ѿ���ʼ�ˣ� ��������һ֡�����ˡ�
				{
					ret = 4;
					started = false;
					ended = true;
					if ( len > 2 )
					{
						unsigned short crc;
						unsigned char cbarr[2];
						crc = do_crc(0xffff, data, len-2);//��CRC
						cbarr[0] = crc&0x00ff;   //���ֽ���ǰ
						cbarr[1] = crc>>8&0x00ff; //���ֽ��ں�
						this->crc_ok = ( cbarr[0] == data[len-2] && cbarr[1] ==  data[len-1] );
					}
				} else {		//δ��ʼ���µ�һ֡��ʼ��, ��һbit��ʼ��֡������
					//reset();
					started = true; 
					start_bit_at  = when_at + 1;
					ret = 3;
				}
				goto NEXT;
			} else {
				//��Ч֡
				reset();
			}
		}
			
		if ( started ) 
		{
			cur = cur | (bt << bit_pos);
			bit_pos++;
			if ( bit_pos == 8 )  //һ���ֽ��Ѿ�����
			{
				data[len] = cur; len++;
				cur =0x00; bit_pos = 0;
			}
			ret = 3;
		}
NEXT:
		if ( bt) 
			set_bit_cont++;
		else 
			set_bit_cont =0;
END:
		return ret;
	};

	void reset()
	{
		started = false;
		set_bit_cont = 0;
		len = 0;
		cur = 0x00;
		bit_pos = 0;
		ended = false;
		c_14K = 0;
		start_bit_at = -1;
		crc_ok = false;
		before_guide_bit_num = 0 ; //ǰ��0����֮ǰ��λ��, ��Ȼ��ǰ��0������16λ��
		post_guide_bit_num = 0;	//�󵼱��ص�λ��
		guided_num = 0;
	};
} GB_DATA;

typedef struct _GB_WAVE {
	bool started;		//�����Ƿ�ʼ��
	bool isInvalid;		//�Ѿ����Ϸ��ˣ���ֻ��started֮��ſ���
	int start_at;		/*���ο�ʼ�㣬Ӧ���ǵ�һ�������ء�
							���ڵ�һ���½��أ������35.7us���������14K��ʼ����һ�������أ�71.4us�����϶�14K��ʼ
							���ڵ�һ���½��أ������35.7us���������14K��ʼ����һ�������أ�71.4us�����϶�14K��ʼ��
						*/
	int end_at;			//���ν�����

	unsigned char last_logic;//��һ���߼���ƽ
	int last_up_edge_at;	//��һ��������λ��
	int last_down_edge_at;	//��һ���½���λ��

	long sof_at[10240]; //�Ӳ��ο�ʼ����¼n����ƽ��ת�㣬�����������ء��½���....,��֤��¼��һ֡����
	int sof_at_index;	/* 0~n����ʾ��һ��sof_atҪ�ŵ�����λ�ã�n==8ʱ�� ���ɽ����жϣ����Զ�ż��λ�ã��������أ�����λ���½��ء�
							��������½���ʱ��Ҫ��һ�µ͵�ƽ�Ĳ�����λ�ã������ʱ��(>0us)������Ϊһ֡���ν��� */
	unsigned char bit_arr[10240]; //����bit
	int bit_at;		//bit����ָʾ����һ�ο���λ��

	int dsrc_type; //ͨ�������жϣ�256KΪRSU; 512KΪOBU��

	int clk_base_at, work_clk_base_at;	//ʱ����ʼ�㣬һ����start_at��ͬ�� Ҳ�����⡣ ��һ�����ڷ���ʱ�õ����ֵ�����ʱ��Ʈ��̫�� ����ʼ��ĵ����һ��ʱ���ء�
	int invalid_at; //FM0����Ƿ�������
	double clock_period, clock_delta, clock_shift;	//ʱ������������3.90625 ��256k�� �� 1.953125 (512k)���֣�������Ϊ0�� ���60����ת��ƽ������ȷ����ͬʱ�϶����겨�Ρ�
	int clock_counter_total, clock_counter;	//ʱ�Ӽ���
	bool levelChanged; //��¼λ���ڵ�ƽ�Ƿ�ת
	double t256K, t512K, t14K, t14K_delta, t256K_delta, t512K_delta, too_width_low;

	GB_DATA fm_dat;

#define T256K 3.90625
#define T512K 1.953125
#define T14K  71.4285714

	void pre_set ( int fmulti)
	{
		t256K = T256K * fmulti;
		t512K = T512K * fmulti;
		t14K = T14K*fmulti;
		t14K_delta = 5* fmulti;
		t256K_delta = 0.95 * fmulti;	//0.95Ӧ��������������, 3.90625��1/4��0.975��
		t512K_delta = 0.48 * fmulti;
		too_width_low = 70 * fmulti;
	};

	void init() {
		started = false;
		isInvalid = false;
		start_at= 0;
		clk_base_at = 0;
		end_at = 0;
		invalid_at = 0;

		sof_at_index = 0;
		bit_at = 0;

		last_logic = 0;
		last_up_edge_at = 0;
		last_down_edge_at = -1;

		clock_period = 0;
		clock_delta = 0;
		clock_counter = 0;
		levelChanged = false;

		dsrc_type =-1;
		fm_dat.reset();
	};

	int feed_point( unsigned char cur_logic, long point, unsigned char *pre_logic, long *pre_point, int &pre_num) 
	{
		int fret = 0; //0:���겨��δ��ʼ�� 1:���겨���ѿ�ʼ�� 3:��������֡��ʼ, 4:��������֡����, 5:���겨�ν���, 6~7:���겨�ν�����������һ���¿�ʼ

		if ( !started ) 
		{
			if (cur_logic == 1 ) //������
			{
				init();
				started = true;
				start_at = point;
				last_up_edge_at = point;
				sof_at[sof_at_index] = point;
				sof_at_index++;
				fret  = 0;
			}
			goto END;
		}

		if ( last_down_edge_at > 0 && (cur_logic == 0|| (last_logic == 0 && cur_logic == 1)) && point > ( last_down_edge_at + too_width_low ) )
		{ //�͵�ƽά��ʱ�䳬��70us�� ��Ϊ��ǰ֡����, �ر���60us�ĵ͵�ƽ����������ܿ����������
			if ( fm_dat.c_14K > 9 ) //��14K��
			{
				dsrc_type = FR_TYPE_14K;
			}
			started = false;
			end_at = last_down_edge_at;
			if ( last_logic == 0 && cur_logic == 1 ) 
			{
				fret = 6; //������������أ�14K�����������²��ο�ʼ��֪ͨ������ͬ�������ٵ���
				pre_logic[0] = 1;
				pre_point[0] = sof_at[sof_at_index-1] ;
				pre_num = 1; 
			}
			else
				fret = 5;
			goto END;
		}

		if (  last_logic != cur_logic ) //��ת��¼
		{
			if ( sof_at_index < sizeof(sof_at) ) 
			{
				sof_at[sof_at_index] = point;
				sof_at_index++;
			}
		} else
			goto END;  //�����һ��ɡ�

		if ( isInvalid ) {
			fret =2 ;
			goto END;		//���в��ο�ʼ�ˣ� �Ѿ�ȷ�����ǹ��겨�Σ�ֱ�ӷ��أ���һ��������
		}

		//�ã����ﲨ�ο�ʼ�ˡ�
		if ( clock_period > 1 ) 
			goto BIT_PRO;

		if (  last_logic == 0 && cur_logic == 1) //������
		{
			bool is14 = false;
			if ( t14K - t14K_delta<= point- last_up_edge_at &&  point- last_up_edge_at <= t14K+t14K_delta)  
			{
				is14 = true;
				fm_dat.c_14K++;
			} else  if ( fm_dat.c_14K > 9 ) //���ﲻ��14K��, ��������14K�������FM0��������, ������һ�����ھ���FM0��
			{
				//14K���ν���
				dsrc_type = FR_TYPE_14K;
				end_at = last_up_edge_at;
				started = false;
				fret = 7; //����ر������ Ҫ������߻ص�ǰһ���������ٵ��á�
				pre_logic[0] = 1;
				pre_point[0] = sof_at[sof_at_index-1] ;
				pre_num = 1; 
				goto END;
			} else 
				fm_dat.c_14K = 0; //һ�������У������ǵر겨�Σ��м������ڷ�����14K�����������ϲ��ᣩ�� ���ԣ���һ�����ڲ����ϣ�����Ҫ���㡣

			if (is14) 
				goto END;
		}

#define PRE_NUMBER 127
#define CLK_BASE 32
		if ( clock_period < 1 ) 
		{
			fret = 0;
			if (sof_at_index == PRE_NUMBER )	//Ըǰn����ȷ�����ʱ��Ƶ�ʣ�����FM0����.
			{
				int m_start, start_index ;
				int m_clk;
				bool m_chg;
				unsigned char m_barr[PRE_NUMBER];
				int m_bat;
#define TRY_NUM 2
				double m_clk_prd[TRY_NUM], m_clk_dlt[TRY_NUM];
				int k, m_try; 

				m_clk_prd[0] = t256K;
				m_clk_prd[1] = t512K;
				m_clk_dlt[0] = t256K_delta;
				m_clk_dlt[1] = t512K_delta;

				for ( m_try = 0 ; m_try < 2; m_try++ ) 
				{
					for ( start_index = 0; start_index <= CLK_BASE; start_index+=2)
					{
						m_start = sof_at[start_index];
						m_clk=0;
						m_chg=false;
						m_bat=0;

						for ( k = start_index+1 ; k < PRE_NUMBER; k++ )
						{
							double s,t,t_tmp;
							t_tmp = (m_clk+1)*m_clk_prd[m_try];
							s= t_tmp -m_clk_dlt[m_try];
							t = t_tmp + m_clk_dlt[m_try];

							if (  s <= (sof_at[k] - m_start) && (sof_at[k] - m_start) <= t ) 
							{
								//������һ��λ����ʼ��
								if (m_chg ) 
									m_barr[m_bat] = 0;
								else 
									m_barr[m_bat] = 1;
								m_bat++;
								m_clk++;
								m_chg = false;
							} else if  ( sof_at[k] < (m_start + t_tmp) &&  !m_chg ) 
							{		//������һ��λ����ʼ��, �м䷭ת, �������δ����һ��λ����ʼ, ���һ�û�о�����ת
								m_chg = true;
							} else {	//�������, �Ǿ����Ѿ���תһ����, �ٷ�ת������FM0������. ����,���λ�ó�����Ԥ�����һ��λ����ʼ
								break;
							}
						}

						if ( k == PRE_NUMBER ) //���������з�ת,����FM0����,�������ʱ����, 
						{
							int jj;
							clock_period = m_clk_prd[m_try];
							clock_delta = m_clk_dlt[m_try];
							clock_counter = m_clk;
							clock_counter_total = clock_counter;
							clock_shift = 0;
							levelChanged = m_chg;
							memcpy(bit_arr, m_barr, m_bat*sizeof(unsigned char));
							bit_at= m_bat;
							clk_base_at = m_start;
							work_clk_base_at = clk_base_at;
							//���潫bit���뵽fm_dat�з���
							for ( jj = 0 ; jj < bit_at; jj++)
							{
								fret = fm_dat.put_bit (bit_arr[jj],jj);
							}
							break;
						}
					}
					if ( clock_period > 0 ) break;
				}

				if ( m_try == 0 ) 
					dsrc_type = FR_TYPE_GB_RSU;
				else if ( m_try == 1 ) 
					dsrc_type = FR_TYPE_GB_OBU;

				if (m_try == 2 ) //����ȷ��512K,����256K
				{
					isInvalid = true;	//�����β��ǹ���ģ� ����һ�ز����ˡ�
				}
			}
			goto END;
		}

BIT_PRO:
		if (  last_logic != cur_logic ) //�з�ת,��ȷ��FM0������
		{	
			double tmp,shift;
			bool try_new_base = false;
COMPARE_CLK:
			tmp = (clock_counter+1)*clock_period + work_clk_base_at  - point;
			if ( (-clock_delta) <= tmp && tmp <= clock_delta ) 
			{
				//����λ����ʼ��
				if (levelChanged ) 
					bit_arr[bit_at] = 0;
				else 
					bit_arr[bit_at] = 1;
				fret = fm_dat.put_bit (bit_arr[bit_at], bit_at); //fm_dat����
				shift = (clock_counter_total+1)*clock_period + clk_base_at  - point; 
				if ( shift < 0 ) shift = -shift;
				if (shift > clock_shift ) clock_shift = shift;
				
				bit_at++;
				clock_counter++;
				clock_counter_total++;
				levelChanged = false;
			}  else if ( tmp > 0  &&  !levelChanged ) 
			{	//������һ��λ����ʼ��, �м䷭ת, �������δ����һ��λ����ʼ, ���һ�û�о�����ת
				levelChanged = true;
			} else {	//�������, �Ǿ����Ѿ���תһ����, �ٷ�ת������FM0������. ����,���λ�ó�����Ԥ�����һ��λ����ʼ
				this->isInvalid = true;
				invalid_at = point;
				if ( fm_dat.ended && cur_logic == 0 ) 
				{
					this->end_at = point;
					fret = 5; //���겨�ν����������Ǹ��������жϵġ�
				} else if (!try_new_base) {
					//����ʱ�����, sof_at_index-1 ��ָ���ǵ�ǰ���point��
					this->isInvalid = false;
					clock_counter = 0;
					try_new_base = true;
					if ( levelChanged )  //���м䷭ת�� ����ǰһ����Ϊ�����
					{
						work_clk_base_at = sof_at[sof_at_index-3];
					} else {	//�����м䷭ת�� ��ǰһ����
						work_clk_base_at = sof_at[sof_at_index-2];
					}
					goto COMPARE_CLK;
				}				
				goto END;
			}
		}

END:
		if ( last_logic == 1 && cur_logic == 0 )  //�½���
		{
			last_down_edge_at = point;
		} else if (last_logic == 0 && cur_logic == 1) {
			last_down_edge_at =  -1;		//���������ˣ��½���ȡ����
			last_up_edge_at = point;
		}

		last_logic = cur_logic;
		return fret;
	};
} GB_WAVE;

typedef struct _UNI_WAVE {
	long first_sample_at;	//��һ��������ֵ�� ���ʱ��ȥ���ֵ��
	unsigned short high_threshold;  /* high level threshold */
	unsigned short low_threshold;   /*low level threshold */
	unsigned char last_logic;
	long now_sample;

	int multi;	/* ��1us(1΢��)Ϊ��λ�� ���������nMHz�������Ϊn�� Ҳ����˵�� 1us(1΢��)����1��, ��ֵΪ1; 1us(1΢��)����n��, ��ֵΪn */

	GB_WAVE gb_wave;
	DB_WAVE db44_wave;
	unsigned char again_logic;
	long again_point;

	unsigned char pre_logic[32];
	long pre_point[32];
	int pre_num;
	int done;

	void pre_set ( int fmulti, bool no_guide, unsigned short high,unsigned short low,bool report) 
	{
		gb_wave.pre_set(fmulti);
		db44_wave.pre_set(fmulti, no_guide,report);
		first_sample_at = -1;
		now_sample = 0;
		multi = fmulti;

		pre_num = 0;
		high_threshold = high;
		low_threshold = low;

	};
	void init() {
		done = 0;
		gb_wave.init();
		db44_wave.init();

	};

	int feed_points(unsigned short *&levarr, long *&posarr, int &lev_num, int pre_define_type, int step)
	{
		int i;
		unsigned short *levp;
		long point;
		unsigned char cur_logic=0;
		int fret=0;
		int pre_size=32, an_size=pre_size-pre_num;

		done=0;
		if ( first_sample_at  < 0 )  { 
			if (posarr ) 
				first_sample_at = posarr[0]; //��ס��һ��ȡ���㡣
			else
				first_sample_at = 0;
		}
		for ( i = 0; i < pre_num ;i++) 
		{
			if ( gb_wave.clock_period > 1 || gb_wave.fm_dat.c_14K > 9) 
				goto GB_PRO2; //�Ѿ��ǹ�����߹��꣬���ߵر�; ���14K����10�����ڣ�Ҳ���ǹ����ˡ�

			fret = db44_wave.feed_point (pre_logic[i], pre_point[i], pre_define_type);
			if ( db44_wave.ended && !db44_wave.wait_to_change ) 
			{
				done = 1;		//��һ֡Ҫ�����
			}

			if ( db44_wave.has_data ) goto STEP; //�Ѿ��ǵر�Ͳ����߹��ꡣ
GB_PRO2:
			fret = gb_wave.feed_point (pre_logic[i], pre_point[i], &pre_logic[pre_num], &pre_point[pre_num], an_size);  //������ݷ���ǰ�����㣬 �ַ����������������ں���
			if (fret >= 5 ) 
				done = 2;
		}

STEP:
		levp = levarr;
		pre_num = 0;
		cur_logic = last_logic;
		for ( i = 0 ; i < lev_num; i++, levp+=step)
		{
			if ( done) //��һ֡����ˣ��жϣ������㶨����һ��Ҫ����ĵ��� 
				break;

			if ( posarr)
				point = posarr[i];
			else
				point = now_sample +i;

			if ( *levp >= high_threshold && *levp <= 4096)
				cur_logic = 1;
			else if ( *levp <= low_threshold  || *levp > 4096)
				cur_logic = 0;		//������������������� �򱣳���һ�ε�״̬��

/* ������Σ� Ϊ�˵��ԣ� ����и������ļ��������ԣ� ������������������������Ƿ������� */
			
//			if ( point == 4628 ) 
//			{
//				int ss = 0;
//			}
			
			if ( gb_wave.clock_period > 1 || gb_wave.fm_dat.c_14K > 9) goto GB_PRO; //�Ѿ��ǹ�����߹��꣬���ߵرꡣ

			fret = db44_wave.feed_point (cur_logic, point, pre_define_type);
			if ( db44_wave.ended && !db44_wave.wait_to_change ) 
			{
				done = 1;		//��һ֡Ҫ�����
			}
			if ( fret == 6 )  //��fret�ж�һ�£��Ƿ�Ҫ��һ�ε���ͬһ�㡣
			{
				pre_logic[0]= cur_logic;
				pre_point[0] = point;
				pre_num = 1;
			}
			
			if ( db44_wave.has_data ) continue; //�Ѿ��ǵر�Ͳ����߹��ꡣ
GB_PRO:
			fret = gb_wave.feed_point (cur_logic, point, this->pre_logic, this->pre_point, pre_size); 
			if (fret >= 5 ) 
				done = 2;
			if ( fret >= 6) //��fret�ж�һ�£� �Ƿ��ٴ��ص��ü�����
			{
				pre_num = pre_size;
			}
		}

		last_logic = cur_logic;
		levarr = levp;
		if ( posarr) 
			posarr += i;
		else
			now_sample += i;
		lev_num = lev_num - i;

		return done;
	};
} UNI_WAVE;

