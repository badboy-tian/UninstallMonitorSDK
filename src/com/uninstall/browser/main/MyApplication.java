package com.uninstall.browser.main;

import com.uninstall.browser.sdk.UninstallBrowserSDK;
import android.app.Application;

public class MyApplication extends Application {

	@Override
	public void onCreate() {
		super.onCreate();
		UninstallBrowserSDK.getInstance("com.tencent.mtt").setUninstallWebUrl(this,"http://shouji.360.cn/web/uninstall/uninstall.html");
	}
}
