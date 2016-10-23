/* �ƿ�ָ����ļ��еĸ��ֱ��� */
enum Var_Type {VAR_FlowPrint=2, VAR_TotalIns = 3, VAR_Dynamic = 10, VAR_Refer=11, VAR_Constant=98,  VAR_None=99};

struct PVarBase {
	Var_Type kind;
	const char *name;	//������doc�ĵ�
	int n_len;			//���Ƴ���

	char content[512];	//��������. ������������������һ�㲻��ͬʱ��
	int c_len;			//���ݳ���
	int dynamic_pos;	//��̬����λ��, -1��ʾ��̬
};

struct DyVarBase {	//��̬����
	Var_Type kind;	//��̬����, 
	int index;	//����, Ҳ�����±�ֵ
	int c_len;
	char val[512];	//��������. ��ʱ����, �㹻�ռ���
};

typedef int (*IWayCallType)(int snap_num, struct DyVarBase **snap, int pv_num, struct PVarBase **pv);
	
#if !defined(IWay_EXPORT)
	#if defined(_WIN32) 
		#define IWay_EXPORT __declspec(dllexport) 
	#else	/* OS: Linux/unix, etc */
		#define IWay_EXPORT 
	#endif
#endif /* end ifdef _WIN32 */

