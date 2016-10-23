/* 制卡指令定义文件中的各种变量 */
enum Var_Type {VAR_FlowPrint=2, VAR_TotalIns = 3, VAR_Dynamic = 10, VAR_Refer=11, VAR_Constant=98,  VAR_None=99};

struct PVarBase {
	Var_Type kind;
	const char *name;	//来自于doc文档
	int n_len;			//名称长度

	char content[512];	//变量内容. 这个内容与下面的内容一般不会同时有
	int c_len;			//内容长度
	int dynamic_pos;	//动态变量位置, -1表示静态
};

struct DyVarBase {	//动态变量
	Var_Type kind;	//动态类型, 
	int index;	//索引, 也就是下标值
	int c_len;
	char val[512];	//变量内容. 随时更新, 足够空间啦
};

typedef int (*IWayCallType)(int snap_num, struct DyVarBase **snap, int pv_num, struct PVarBase **pv);
	
#if !defined(IWay_EXPORT)
	#if defined(_WIN32) 
		#define IWay_EXPORT __declspec(dllexport) 
	#else	/* OS: Linux/unix, etc */
		#define IWay_EXPORT 
	#endif
#endif /* end ifdef _WIN32 */

