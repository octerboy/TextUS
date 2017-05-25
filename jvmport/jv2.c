#include <jni.h>       /* where everything is defined */ 
 
int main()
{
 
    JavaVM *jvm;       /* denotes a Java VM */ 
    JNIEnv *env;       /* pointer to native method interface */ 
	int res;
 
    JDK1_8InitArgs vm_args; /* JDK 1.1 VM initialization arguments */ 
 
    vm_args.version = 0x00010001; /* New in 1.1.2: VM version */ 
    /* Get the default initialization arguments and set the class  
     * path */ 
    JNI_GetDefaultJavaVMInitArgs(&vm_args); 
    //vm_args.classpath = ...; 
 
    /* load and initialize a Java VM, return a JNI interface  
     * pointer in env */ 
    res = JNI_CreateJavaVM(&jvm, &env, &vm_args); 
	printf("res %d\n", res);
 

}
