package com.zy.photostitching;

public class ImageProc {
	public static native int proc(String path,long init,int num,int width,int height);
	public static native double getTime();
	public static native String getResultName();

}
