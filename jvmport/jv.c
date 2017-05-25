#include <jni.h>
int main() {
int res,n;
jsize sz;
JavaVM *jvm;
JNIEnv *env;
JavaVMInitArgs vm_args;
JavaVMOption options[8];
vm_args.version=JNI_VERSION_1_6;
//这个字段必须设置为该值
/*设置初始化参数*/
options[1].optionString = "-Djava.class.path=.";
options[2].optionString = "-verbose:jni";
options[0].optionString = "-Xdebug";
options[3].optionString = "-Djava.compiler=NONE";
options[4].optionString = 0;
options[0].extraInfo = 0;
options[1].extraInfo = 0;
options[2].extraInfo = 0;
options[3].extraInfo = 0;
//用于跟踪运行时的信息
/*版本号设置不能漏*/
//vm_args.version = JNI_VERSION_1_4;
vm_args.nOptions = 3;
vm_args.options = options;
vm_args.ignoreUnrecognized = JNI_FALSE;

res = JNI_GetDefaultJavaVMInitArgs(&vm_args);
fprintf(stderr, "JNI_GetDefaultJavaVMInitArgs %08x\n", res);
for (n = 0 ; n < vm_args.nOptions; n++)
	fprintf(stderr, "%d %s\n", n, vm_args.options[n].optionString);
	
sz = 1;
res =  JNI_GetCreatedJavaVMs(&jvm, sizeof(jvm), &sz);
fprintf(stderr, "JNI_GetCreatedJavaVMs %08x, %d\n", res, sz);
jvm = 0;
res = JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);
if (res < 0)
{
fprintf(stderr,
"Can't create Java VM %08x\n", res);
	return 1;
}
(*jvm)->DestroyJavaVM(jvm);
fprintf(stdout,
"Java VM destory.\n");
}
