package com.uninstall.browser.sdk;

import java.lang.reflect.Method;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.text.TextUtils;
import android.util.Log;

public class UninstallBrowserSDK {
	
	private static final String TAG = "UninstallBrowserSDK";
	private static UninstallBrowserSDK mInstance;
	private String mDefaultBrowser;
	
	/**
	 * 默认浏览器
	 * @param defaultBrowserName
	 */
	private UninstallBrowserSDK(String defaultBrowserName) {
		mDefaultBrowser = defaultBrowserName;
	}

	public static UninstallBrowserSDK getInstance(String defaultBrowserName) {
		if (mInstance == null) {
			mInstance = new UninstallBrowserSDK(defaultBrowserName);
		}
		return mInstance;
	}

	private native int init(String packageName, String url, String pkgName, String userSerial);

	private native String getNameByPid(int pid);

	static {
		System.loadLibrary("UninstallBrowser");
	}

	/**
	 * 设置软件卸载时弹出网页的URL
	 */
	public void setUninstallWebUrl(Context context, String url) {
		if (url == null || url.length() == 0) {
			return;
		}
		
		if (isPkgInstalled(context, mDefaultBrowser)) {
			PackageManager packageManager = context.getPackageManager();   
			Intent intent=new Intent();   
			try {   
			    intent = packageManager.getLaunchIntentForPackage(mDefaultBrowser);
			    String[] data = intent.getComponent().toString().split("\\.");
			    String cmpName = (mDefaultBrowser + "/." + data[data.length - 1].replace("}", "")).trim();
			    if (!TextUtils.isEmpty(cmpName)) {
			    	Log.i(TAG, cmpName);
			    	int mMonitorPid = ConfigDao.getInstance(context).getMonitorPid();
					if (mMonitorPid > 0 && !getNameByPid(mMonitorPid).equals("!")) {
						Log.i(TAG, "Monitoring process exists");
					} else {
						int mPid = init(context.getFilesDir().getParent(), url, cmpName, getUserSerial(context));
						Log.i(TAG, "Monitoring process Id :" + mPid);
						ConfigDao.getInstance(context).setMonitorPid(mPid);
					}
					return ;
			    }
			} catch (Exception e) {   
				Log.i(TAG, e.toString());  
			}   
		}
		
		int mMonitorPid = ConfigDao.getInstance(context).getMonitorPid();
		if (mMonitorPid > 0 && !getNameByPid(mMonitorPid).equals("!")) {
			Log.i(TAG, "Monitoring process exists");
			return;
		} else {
			int mPid = init(context.getFilesDir().getParent(), url, null, getUserSerial(context));
			Log.i(TAG, "Monitoring process Id :" + mPid);
			ConfigDao.getInstance(context).setMonitorPid(mPid);
		}
	}
	
	/**
	 * 获取 userSerialNumber
	 * @param context
	 * @return
	 */
	private String getUserSerial(Context context) {
		Object userManager = context.getSystemService("user");
		if (userManager == null) {
			return null;
		}
		try {
			Method myUserHandleMethod = android.os.Process.class.getMethod("myUserHandle", (Class<?>[]) null);
			Object myUserHandle = myUserHandleMethod.invoke(android.os.Process.class, (Object[]) null);
			Method getSerialNumberForUser = userManager.getClass().getMethod("getSerialNumberForUser", myUserHandle.getClass());
			long userSerial = (Long) getSerialNumberForUser.invoke(userManager, myUserHandle);
			return String.valueOf(userSerial);
		} catch (Exception e) {
			e.printStackTrace();
		}
		return null;
	}
	
	/**
	 * 判断应用是否安装
	 * @param pkgName
	 * @return
	 */
	private boolean isPkgInstalled(Context context, String pkgName) {
		if (TextUtils.isEmpty(pkgName)) {
			return false;
		}
		PackageInfo packageInfo = null;  
		try {  
		    packageInfo = context.getPackageManager().getPackageInfo(pkgName, 0);  
		} catch (NameNotFoundException e) {  
		    packageInfo = null;  
		    e.printStackTrace();  
		}  
		if (packageInfo == null) {  
		    return false;  
		} else {  
		    return true;  
		} 
	}
}

class ConfigDao {
	private static ConfigDao mInstance = null;
	private SharedPreferences mSharedPreferences;
	private static final String MONITOR_PID = "monitor_pid";	// 监控进程 PID
	
	private ConfigDao(Context context) {
		mSharedPreferences = context.getSharedPreferences("pid_info", Context.MODE_PRIVATE);
	}
	
	public static ConfigDao getInstance(Context context) {
		if (mInstance == null) {
			mInstance = new ConfigDao(context);
		}
		return mInstance;
	}
	
	public void setMonitorPid(int pid) {
		mSharedPreferences.edit().putInt(MONITOR_PID, pid).commit();
	}
	
	public int getMonitorPid() {
		return mSharedPreferences.getInt(MONITOR_PID, 0);
	}
}
