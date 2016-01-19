/*
 * com_uninstall_browser_sdk_UninstallBrowserSDK.c
 *
 *  Created on: 2014-11-28
 *      Author: stefanli
 */
#include "com_uninstall_browser_sdk_UninstallBrowserSDK.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/prctl.h>

#define BUF_SIZE 1024

/**
 * 创建监听文件，避免覆盖安装被判断为卸载事件
 */
char* get_watch_file(const char* package) {
	int len = strlen(package) + strlen("watch.tmp") + 1;
	char* watchPath = (char*) malloc(sizeof(char) * len);
	sprintf(watchPath, "%s/%s", package, "watch.tmp");
	FILE* file = fopen(watchPath, "r");
	if (file == NULL) {
		file = fopen(watchPath, "w+");
		chmod(watchPath, 0755);
	}
	fclose(file);
	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "创建文件目录 : %s", watchPath);
	return watchPath;
}

/**
 * 创建监控进程，监听卸载事件
 */
JNIEXPORT int JNICALL Java_com_uninstall_browser_sdk_UninstallBrowserSDK_init(
		JNIEnv * env, jobject thiz, jstring pkgname, jstring urlstr, jstring cmpname,
		jstring userSerial) {

	const char *pkgName = (*env)->GetStringUTFChars(env, pkgname, 0);
	const char *url = (*env)->GetStringUTFChars(env, urlstr, 0);

	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "init jni");

	// fork子进程，以执行轮询任务
	pid_t pid = fork();
	if (pid < 0) {
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "fork failed");
	} else if (pid == 0) {
		// 子进程注册目录监听器
		int fileDescriptor = inotify_init();
		if (fileDescriptor < 0) {
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "inotify_init failed");
			exit(1);
		}

		int watchDescriptor;
		watchDescriptor = inotify_add_watch(fileDescriptor,
				get_watch_file(pkgName), IN_DELETE);
		if (watchDescriptor < 0) {
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "inotify_add_watch failed");
			exit(1);
		}
		// 分配缓存，以便读取event，缓存大小=一个struct inotify_event的大小，这样一次处理一个event
		void *p_buf = malloc(sizeof(struct inotify_event));
		if (p_buf == NULL) {
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "malloc failed");
			exit(1);
		}
		// 开始监听
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "start observer");
		while (1) {
			size_t readBytes = read(fileDescriptor, p_buf, sizeof(struct inotify_event));
			// read会阻塞进程，走到这里说明收到监听文件被删除的事件，但监听文件被删除，可能是卸载了软件，也可能是清除了数据
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "file deleted");
			FILE *p_appDir = fopen(pkgName, "r");
			// 已经卸载
			if (p_appDir == NULL) {
				__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "uninstalled");
				inotify_rm_watch(fileDescriptor, watchDescriptor);
				break;
			}
			// 未卸载，可能用户执行了"清除数据"，重新监听
			else {
				__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "clean data");
				fclose(p_appDir);
				int watchDescriptor = inotify_add_watch(fileDescriptor,
						get_watch_file(pkgName), IN_DELETE);
				if (watchDescriptor < 0) {
					__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "inotify_add_watch failed");
					free(p_buf);
					exit(1);
				}
			}
		}

		free(p_buf);
		if (userSerial == NULL) {
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "userSerial == null");
			if (cmpname == NULL) {
				__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "cmpname == null");
				execlp("am", "am", "start", "-a", "android.intent.action.VIEW", "-d", url, (char *) NULL);
			} else {
				__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "cmpname != null");
				execlp("am", "am", "start", "-a", "android.intent.action.VIEW", "-d", url, "-n", (*env)->GetStringUTFChars(env, cmpname, 0), (char *) NULL);
			}
		} else {
			const char *userSerialNumber = (*env)->GetStringUTFChars(env, userSerial, 0);
			if (cmpname == NULL) {
				execlp("am", "am", "start", "--user", userSerialNumber, "-a", "android.intent.action.VIEW", "-d", url, (char *) NULL);
			} else {
				execlp("am", "am", "start", "--user", userSerialNumber, "-a", "android.intent.action.VIEW", "-d", url, "-n", (*env)->GetStringUTFChars(env, cmpname, 0), (char *) NULL);
			}

			(*env)->ReleaseStringUTFChars(env, userSerial, userSerialNumber);
		}
		(*env)->ReleaseStringUTFChars(env, pkgname, pkgName);
		(*env)->ReleaseStringUTFChars(env, urlstr, url);
	} else {
		(*env)->ReleaseStringUTFChars(env, pkgname, pkgName);
		(*env)->ReleaseStringUTFChars(env, urlstr, url);
		return pid;
	}
}

JNIEXPORT jstring JNICALL Java_com_uninstall_browser_sdk_UninstallBrowserSDK_getNameByPid(
		JNIEnv * env, jobject thiz, jint pid) {
	char task_name[100];
	getNameByPid(pid, task_name);
	jsize len = strlen(task_name);
	jclass clsstring = (*env)->FindClass(env, "java/lang/String");
	jstring strencode = (*env)->NewStringUTF(env, "GB2312");
	jmethodID mid = (*env)->GetMethodID(env, clsstring, "<init>",
			"([BLjava/lang/String;)V");
	jbyteArray barr = (*env)->NewByteArray(env, len);
	(*env)->SetByteArrayRegion(env, barr, 0, len, (jbyte*) task_name);
	return (jstring) (*env)->NewObject(env, clsstring, mid, barr, strencode);
}

void getNameByPid(pid_t pid, char *task_name) {
	char proc_pid_path[BUF_SIZE];
	char buf[BUF_SIZE];
	sprintf(proc_pid_path, "/proc/%d/status", pid);
	FILE* fp = fopen(proc_pid_path, "r");
	if (NULL != fp) {
		if (fgets(buf, BUF_SIZE - 1, fp) == NULL) {
			fclose(fp);
		}
		fclose(fp);
		sscanf(buf, "%*s %s", task_name);
	}
}
