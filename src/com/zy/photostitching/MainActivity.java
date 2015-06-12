package com.zy.photostitching;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Iterator;
import java.util.List;

import org.opencv.android.OpenCVLoader;



import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.Intent;
import android.graphics.Color;
import android.hardware.Camera.Size;
import android.util.Log;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;
import android.widget.TableLayout;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {

	
	private Button mStitchBut;
	private Button mViewBut;
	private Button mPicBut;
	private static final String TAG="opencv";
	private TextView info;
	private MyCamera mCamera;
	private SurfaceView msurface;
	private String initPath;
	private long initNum;
	private int picNum =0;
	private SimpleDateFormat mdate;
	private String resultName;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		this.mStitchBut=(Button) findViewById(R.id.stitch);
		this.mPicBut = (Button) findViewById(R.id.takepic);
		this.mViewBut = (Button) findViewById(R.id.view);
		this.info=(TextView) findViewById(R.id.info);
		this.msurface = (SurfaceView) findViewById(R.id.surface);
		
		this.mStitchBut.setOnClickListener(new StitchOnclick());
		this.mPicBut.setOnClickListener(new PicOnclick());
		this.mViewBut.setOnClickListener(new ViewOnclick());
		this.info=(TextView) findViewById(R.id.info);
		
		
		this.mdate = new SimpleDateFormat("yyyyMMddhhmmss");
		this.initNum = Long.parseLong(mdate.format(new java.util.Date()));
		this.initPath = Environment.getExternalStorageDirectory().getAbsolutePath()+"/"+initNum;
		this.mCamera = new MyCamera(msurface);
		mCamera.startPreview();
	}
	
	
	public void clearNum(){
		this.initNum = Long.parseLong(mdate.format(new java.util.Date()));
		Log.d(TAG,String.valueOf(initNum)+"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		this.picNum = 0;
	}
	
	private class StitchOnclick implements OnClickListener{

		@Override
		public void onClick(View v) {
			mCamera.stopPreview();
			Log.d(TAG,String.valueOf(picNum)+"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
			int ret = ImageProc.proc(Environment.getExternalStorageDirectory().getAbsolutePath()+"/",initNum-picNum,picNum,mCamera.getSize().width,mCamera.getSize().height);		
			if(ret == 0){
				Log.d(TAG,"SUCCESS");
				resultName = ImageProc.getResultName();
				MainActivity.this.info.setText("success"+ImageProc.getTime());
			}else{
				Toast.makeText(MainActivity.this, "error", Toast.LENGTH_SHORT).show();
			}
			clearNum();
			mCamera.startPreview();

		}
		
	}
	
	private class ViewOnclick implements OnClickListener{

		@Override
		public void onClick(View v) {
				 Intent intent = new Intent();
			     intent.setAction(android.content.Intent.ACTION_VIEW);
			     intent.setDataAndType(Uri.parse("file://"+resultName), "image/*");
			     startActivity(intent); 

		}
		
	}
	
	private class PicOnclick implements OnClickListener{
		@Override
		public void onClick(View v) {
			if(mCamera.isPriv()){
				initPath =  Environment.getExternalStorageDirectory().getAbsolutePath()+"/"+initNum;
				mCamera.shootAt(initPath+".jpg");
				initNum++;
				picNum++;
				Toast.makeText(MainActivity.this, "拍照成功", Toast.LENGTH_LONG).show();
				MainActivity.this.info.setText("");
			}
		}
		
		
	}
	
	
	
	
	
	
	@Override
	protected void onResume() {
		Log.e(TAG,"5");
		super.onResume();
//		OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_2_4_10, this, this.mLoaderCallback);
		if(OpenCVLoader.initDebug()){
			//mLoaderCallback.onManagerConnected(LoaderCallbackInterface.SUCCESS);
			System.loadLibrary("opencv");
			Log.d(TAG,"load success");
		}else{
			Log.d(TAG,"load failed");
		}
			
		
	}




	@Override
	protected void onRestart() {
		Log.e(TAG,"activity restart");
		mCamera.init();
		if(!mCamera.isPriv()){
			mCamera.startPreview();
		}
		
		super.onRestart();
	}






	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		// TODO Auto-generated method stub
		super.onActivityResult(requestCode, resultCode, data);
	}




	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch(item.getItemId()){
		case R.id.setting:{
			final Builder settingAlert = new AlertDialog.Builder(this);
			settingAlert.setTitle("选择分辨率");
			LinearLayout linearLayout = (LinearLayout) getLayoutInflater().inflate(R.layout.setting, null);
			RadioGroup group = (RadioGroup) linearLayout.findViewById(R.id.radioGroup);
			List<Size> picList = mCamera.getPictureSize();
			Iterator<Size> it = picList.iterator();
			int id = 0;
			while(it.hasNext()){
				Size size = it.next();
				RadioButton radioButton = new RadioButton(this); 
				radioButton.setText(String.valueOf(size.width)+","+String.valueOf(size.height));
				radioButton.setTextColor(Color.WHITE);
				radioButton.setId(id);
				id++;
				group.addView(radioButton, LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
				Log.d(TAG,"SupportedPictureSizes,height:"+size.height+",width:"+size.width);
			}
			group.setOnCheckedChangeListener(new OnCheckedChangeListener() {
				
				@Override
				public void onCheckedChanged(RadioGroup group, int checkedId) {
					Size size = mCamera.getPictureSize().get(checkedId);
					mCamera.setSize(size.width, size.height);
				
				}
			});
		
			settingAlert.setView(linearLayout);
			final AlertDialog dialog = settingAlert.create();
			dialog.show();
			break;
		}
		
		}
		
		return true;
	}
	

}
