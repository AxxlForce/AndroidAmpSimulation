#include <jni.h>
#include <SLES/OpenSLES.h>

jlong Java_com_alex_testing_MainActivity_invokeNativeFunction(JNIEnv* env, jobject javaThis) {
  return 666;
}
