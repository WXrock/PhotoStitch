package com.zy.photostitching;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;

import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.PictureCallback;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class MyCamera implements SurfaceHolder.Callback {

	private boolean ispriv = false;
	private Camera mCamera = null;
	private SurfaceView mSurfaceView = null;
	private SurfaceHolder holder = null;
	private Parameters parameter = null;
	private static final String TAG = "TEST CAMERA";
	public MyCamera(SurfaceView surfaceView){
		this.mSurfaceView = surfaceView;
		this.holder = mSurfaceView.getHolder();
		holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
		holder.addCallback(this);
		mCamera = Camera.open();
		mCamera.setDisplayOrientation(90);
		
	}
	
	private Camera getCamearaInstance(){
		if(mCamera == null){
			Log.e(TAG,"CAMERA INSTANCE IS NULL");
		}
		return mCamera;
	}
	
	
	public void init(){
		mCamera = Camera.open();
		mCamera.setDisplayOrientation(90);
	}
	
	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		parameter = getCamearaInstance().getParameters();
		parameter.setPreviewFrameRate(15);
		parameter.setPreviewFormat(ImageFormat.YV12);
		parameter.setPreviewSize(640, 480);
		parameter.setPictureSize(640, 480);
		parameter.setRotation(90);
		getCamearaInstance().setParameters(parameter);
		try {
			getCamearaInstance().setPreviewDisplay(holder);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width,
			int height) {
		//Log.d(TAG,Integer.toString(width)+","+Integer.toString(height));
		
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		getCamearaInstance().release();	
		ispriv = false;
	}
	
	public boolean isPriv(){
		return ispriv;
	}

	public void startPreview(){
		ispriv = true;
		getCamearaInstance().startPreview();
	}
	
	public void stopPreview(){
		ispriv = false;
		getCamearaInstance().stopPreview();
	}
	
	public void shootAt(final String path){
		getCamearaInstance().takePicture(null, null, new PictureCallback() {
			
			@Override
			public void onPictureTaken(byte[] data, Camera camera) {
				//Log.d(TAG,"ONPICTURE:"+Integer.toString(data.length));
				Bitmap bitmap = BitmapFactory.decodeByteArray(data, 0, data.length);
				savePicture(bitmap, path);
				getCamearaInstance().stopPreview();
				getCamearaInstance().startPreview();
			}
		});	
	}
	
	private void savePicture(Bitmap bitmap ,String path){
		File file = new File(path);
		try {
			bitmap.compress(CompressFormat.JPEG, 100, new FileOutputStream(file));
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	
	public List<Size> getPictureSize(){
		
		List<Size> psizeList = new ArrayList<Size>();
		psizeList = parameter.getSupportedPictureSizes();
		Iterator<Size> it = psizeList.iterator();
		while(it.hasNext()){
			Size size = it.next();
			Log.d(TAG,"SupportedPictureSizes,height:"+size.height+",width:"+size.width);
		}
		
		return psizeList;
	}
	
	public void setSize(int width,int height){
		parameter.setPictureSize(width, height);
		getCamearaInstance().setParameters(parameter);
		Log.e(TAG,"set size");
	}
	
	public Size getSize(){
		return parameter.getPictureSize();
	}


}
