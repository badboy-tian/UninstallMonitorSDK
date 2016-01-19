package com.uninstall.browser.main;

import com.uninstall.browser.sdk.UninstallBrowserSDK;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class BootBroadcastReceiver extends BroadcastReceiver{
	
	private static final String ACTION_BOOT = "android.intent.action.BOOT_COMPLETED";
	
	@Override
	public void onReceive(Context context, Intent intent) {
		if (intent.getAction().equals(ACTION_BOOT)) {  
			UninstallBrowserSDK.getInstance("com.tencent.mtt").setUninstallWebUrl(context,"http://shouji.360.cn/web/uninstall/uninstall.html");
        } 
	}
}
