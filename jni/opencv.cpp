#include <opencv.h>
#include<android/log.h>
//#include <opencv2/core/core.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <opencv2/opencv_modules.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/stitching/detail/autocalib.hpp>
#include <opencv2/stitching/detail/blenders.hpp>
#include <opencv2/stitching/detail/camera.hpp>
#include <opencv2/stitching/detail/exposure_compensate.hpp>
#include <opencv2/stitching/detail/matchers.hpp>
#include <opencv2/stitching/detail/motion_estimators.hpp>
#include <opencv2/stitching/detail/seam_finders.hpp>
#include <opencv2/stitching/detail/util.hpp>
#include <opencv2/stitching/detail/warpers.hpp>
#include <opencv2/stitching/warpers.hpp>
#include <cv.h>

#define  LOG_TAG    "libopencv"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

using namespace cv;
using namespace std;
using namespace cv::detail;

//¶šÒå²ÎÊý
vector<string> img_names;
std::string result_name;
bool try_gpu = false;  // ÊÇ·ñÊ¹ÓÃGPU(ÍŒÐÎŽŠÀíÆ÷)£¬Ä¬ÈÏÎªno

/* ÔË¶¯¹ÀŒÆ²ÎÊý */
double work_megapix = 0.6;//<--work_megapix <float>> ÍŒÏñÆ¥ÅäµÄ·Ö±æÂÊŽóÐ¡£¬
						//ÍŒÏñµÄÃæ»ý³ßŽç±äÎªwork_megapix*100000£¬Ä¬ÈÏÎª0.6
float conf_thresh = 1.f;//conf_thresh <float>Áœ·ùÍŒÀŽ×ÔÍ¬Ò»È«Ÿ°ÍŒµÄÖÃÐÅ¶È
WaveCorrectKind wave_correct = detail::WAVE_CORRECT_HORIZ;//wave_correct (no|horiz|vert) ²šÐÎÐ£Ñé(Ë®Æœ£¬Ž¹Ö±»òÕßÃ»ÓÐ),
															//Ä¬ÈÏÊÇhoriz
float match_conf = 0.3f;//match_conf <float> ÌØÕ÷µãŒì²âÖÃÐÅµÈŒ¶£¬×îœüÁÚÆ¥ÅäŸàÀëÓëŽÎœüÁÚÆ¥ÅäŸàÀëµÄ±ÈÖµ£¬
						//surfÄ¬ÈÏÎª0.65£¬orbÄ¬ÈÏÎª0.3

/*ÍŒÏñÈÚºÏ²ÎÊý*/
double seam_megapix = 0.1;//seam_megapix <double> ÆŽœÓ·ìÏñËØµÄŽóÐ¡£¬Ä¬ÈÏÎª0.1
double compose_megapix =0.6;//compose_megapix <double>ÆŽœÓ·Ö±æÂÊ£¬Ä¬ÈÏÎª-1
int expos_comp_type = ExposureCompensator::GAIN_BLOCKS;//expos_comp (no|gain|gain_blocks)¹âÕÕ²¹³¥·œ·š£¬Ä¬ÈÏÊÇgain_blocks
int blend_type = Blender::MULTI_BAND;//blend (no|feather|multiband) ÈÚºÏ·œ·š£¬Ä¬ÈÏÊÇ¶àÆµ¶ÎÈÚºÏ
float blend_strength = 5;//ÈÚºÏÇ¿¶È£¬0 - 100.Ä¬ÈÏÊÇ5.
double totaltime;
//string result_name = "sdcard/result.jpg";//Êä³öÍŒÏñµÄÎÄŒþÃû

char* jstringTostring(JNIEnv* env, jstring jstr)
{
       char* rtn = NULL;
       jclass clsstring = env->FindClass("java/lang/String");
       jstring strencode = env->NewStringUTF("utf-8");
       jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
       jbyteArray barr= (jbyteArray)env->CallObjectMethod(jstr, mid, strencode);
       jsize alen = env->GetArrayLength(barr);
       jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
       if (alen > 0)
       {
                 rtn = (char*)malloc(alen + 1);
                 memcpy(rtn, ba, alen);
                 rtn[alen] = 0;
       }
       env->ReleaseByteArrayElements(barr, ba, 0);
       return rtn;
}


jstring stoJstring(JNIEnv* env, const char* pat)
{
	jclass strClass = env->FindClass("java/lang/String");
	jmethodID ctorID = env->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
	jbyteArray bytes = env->NewByteArray(strlen(pat));
	env->SetByteArrayRegion(bytes, 0, strlen(pat), (jbyte*)pat);
	jstring encoding = env->NewStringUTF("utf-8");
	return (jstring)env->NewObject(strClass, ctorID, bytes, encoding);
} 

std::string jstring2str(JNIEnv* env, jstring jstr)  
{     
    char*   rtn   =   NULL;     
    jclass   clsstring   =   env->FindClass("java/lang/String");     
    jstring   strencode   =   env->NewStringUTF("GB2312");     
    jmethodID   mid   =   env->GetMethodID(clsstring,   "getBytes",   "(Ljava/lang/String;)[B");     
    jbyteArray   barr=   (jbyteArray)env->CallObjectMethod(jstr,mid,strencode);     
    jsize   alen   =   env->GetArrayLength(barr);     
    jbyte*   ba   =   env->GetByteArrayElements(barr,JNI_FALSE);     
    if(alen   >   0)     
    {     
        rtn   =   (char*)malloc(alen+1);           
        memcpy(rtn,ba,alen);     
        rtn[alen]=0;     
    }     
    env->ReleaseByteArrayElements(barr,ba,0);     
    std::string stemp(rtn);  
    free(rtn);  
    return   stemp;     
}   


JNIEXPORT jint JNICALL Java_com_zy_photostitching_ImageProc_proc
		(JNIEnv *env, jclass obj,jstring path,jlong init,jint num,jint width,jint height)
{
	clock_t start,finish;
	start=clock();

    int mwidth = (int)(width * 0.6);
    int mheight = (int)(height * 0.6);
        

	int argc = num;
    int i;
    char resultBuf[100];
	long long tmpNum ;
	char numBuf[100][argc+1]; 
    std::string charPath = jstring2str(env,path);
	std::string pathbuf[argc];


    for(i= 0;i<argc;i++) {
	tmpNum = (long long)(init + i);
	sprintf(numBuf[i],"%lld",tmpNum);
	//strcat
	pathbuf[i].append(charPath);
    pathbuf[i].append(numBuf[i]);
	pathbuf[i].append(".jpg");
    LOGI("%s\n",pathbuf[i].c_str());
	img_names.push_back(pathbuf[i]);
    }
        sprintf(resultBuf,"%lld",tmpNum+1);        
        result_name = charPath+"result"+resultBuf+".jpg"; 
	

	int num_images = static_cast<int>(img_names.size());
	double work_scale = 1, seam_scale = 1, compose_scale = 1;
	//ÌØÕ÷µãŒì²âÒÔŒ°¶ÔÍŒÏñœøÐÐÔ€ŽŠÀí£š³ßŽçËõ·Å£©£¬È»ºóŒÆËãÃ¿·ùÍŒÐÎµÄÌØÕ÷µã£¬ÒÔŒ°ÌØÕ÷µãÃèÊö×Ó
	LOGI("finding feature\n");
	Ptr<FeaturesFinder> finder;
	finder = new SurfFeaturesFinder();///²ÉÓÃSurfÌØÕ÷µãŒì²â

	Mat full_img1,full_img, img;
	vector<ImageFeatures> features(num_images);
	vector<Mat> images(num_images);
	vector<Size> full_img_sizes(num_images);
	double seam_work_aspect = 1;

	for (int i = 0; i < num_images; ++i)
	{
		full_img1 = imread(img_names[i]);
		resize(full_img1,full_img, Size(mwidth,mheight));
		full_img_sizes[i] = full_img.size();

		//ŒÆËãwork_scale£¬œ«ÍŒÏñresizeµœÃæ»ýÔÚwork_megapix*10^6ÒÔÏÂ
		work_scale = min(1.0, sqrt(work_megapix * 1e6 / full_img.size().area()));

		resize(full_img, img, Size(), work_scale, work_scale);

		//œ«ÍŒÏñresizeµœÃæ»ýÔÚwork_megapix*10^6ÒÔÏÂ
		seam_scale = min(1.0, sqrt(seam_megapix * 1e6 / full_img.size().area()));
		seam_work_aspect = seam_scale / work_scale;

		// ŒÆËãÍŒÏñÌØÕ÷µã£¬ÒÔŒ°ŒÆËãÌØÕ÷µãÃèÊö×Ó£¬²¢œ«img_idxÉèÖÃÎªi
		(*finder)(img, features[i]);
		features[i].img_idx = i;
		//cout<<"Features in image #" << i+1 << ": " << features[i].keypoints.size()<<endl;
		LOGI("Features in image # %d : %d",i+1,features[i].keypoints.size());
		//œ«ÔŽÍŒÏñresizeµœseam_megapix*10^6£¬²¢ŽæÈëimage[]ÖÐ
		resize(full_img, img, Size(), seam_scale, seam_scale);
		images[i] = img.clone();
	}

	finder->collectGarbage();
	full_img.release();
	img.release();

	//¶ÔÍŒÏñœøÐÐÁœÁœÆ¥Åä
	//cout<<"Pairwise matching"<<endl;
	LOGI("Pairwise matching\n");

	//Ê¹ÓÃ×îœüÁÚºÍŽÎœüÁÚÆ¥Åä£¬¶ÔÈÎÒâÁœ·ùÍŒœøÐÐÌØÕ÷µãÆ¥Åä
	vector<MatchesInfo> pairwise_matches;
	BestOf2NearestMatcher matcher(try_gpu, match_conf);//×îœüÁÚºÍŽÎœüÁÚ·š
	matcher(features, pairwise_matches); //¶ÔÃ¿ÁœžöÍŒÆ¬œøÐÐÆ¥Åä
	matcher.collectGarbage();


	//œ«ÖÃÐÅ¶ÈžßÓÚÃÅÏÞµÄËùÓÐÆ¥ÅäºÏ²¢µœÒ»žöŒ¯ºÏÖÐ
	//Ö»ÁôÏÂÈ·¶šÊÇÀŽ×ÔÍ¬Ò»È«Ÿ°ÍŒµÄÍŒÆ¬
	//vector<int> leaveBiggestComponent(vector<ImageFeatures> &features,  vector<MatchesInfo> &pairwise_matches,
										//float conf_threshold)
	//features±íÊŸÍŒÆ¬ÌØÕ÷µãÐÅÏ¢
	//pairwise_matches±íÊŸÍŒÆ¬ÁœÁœÅä¶ÔÐÅÏ¢
	//conf_threshold±íÊŸÍŒÆ¬Åä¶ÔÖÃÐÅãÐÖµ
	vector<int> indices = leaveBiggestComponent(features, pairwise_matches, conf_thresh);
	vector<Mat> img_subset;
	vector<string> img_names_subset;
	vector<Size> full_img_sizes_subset;
	for (size_t i = 0; i < indices.size(); ++i)
	{
		img_names_subset.push_back(img_names[indices[i]]);
		img_subset.push_back(images[indices[i]]);
		full_img_sizes_subset.push_back(full_img_sizes[indices[i]]);
	}

	images = img_subset;
	img_names = img_names_subset;
	full_img_sizes = full_img_sizes_subset;

	// Œì²éÍŒÆ¬ÊýÁ¿ÊÇ·ñÒÀŸÉÂú×ãÒªÇó
	num_images = static_cast<int>(img_names.size());
	if (num_images < 2)
	{
		LOGI("Need more images\n");
		return -1;
	}

	HomographyBasedEstimator estimator;//»ùÓÚµ¥ÓŠÐÔµÄ¹ÀŒÆÁ¿
	vector<CameraParams> cameras;//Ïà»ú²ÎÊý
	estimator(features, pairwise_matches, cameras);

	for (size_t i = 0; i < cameras.size(); ++i)
	{
		Mat R;
		cameras[i].R.convertTo(R, CV_32F);
		cameras[i].R = R;
		LOGI("Initial intrinsics # %d\n",indices[i]+1);
	}

	Ptr<detail::BundleAdjusterBase> adjuster;//¹âÊøµ÷ÕûÆ÷²ÎÊý
	adjuster = new detail::BundleAdjusterRay();//Ê¹ÓÃBundle Adjustment£š¹âÊø·šÆœ²î£©·œ·š¶ÔËùÓÐÍŒÆ¬œøÐÐÏà»ú²ÎÊýÐ£Õý

	adjuster->setConfThresh(conf_thresh);//ÉèÖÃÅäÖÃãÐÖµ
	Mat_<uchar> refine_mask = Mat::zeros(3, 3, CV_8U);
	refine_mask(0,0) = 1;
	refine_mask(0,1) = 1;
	refine_mask(0,2) = 1;
	refine_mask(1,1) = 1;
	refine_mask(1,2) = 1;
	adjuster->setRefinementMask(refine_mask);
	(*adjuster)(features, pairwise_matches, cameras);//œøÐÐœÃÕý


	// Çó³öµÄœ¹ŸàÈ¡ÖÐÖµºÍËùÓÐÍŒÆ¬µÄœ¹Ÿà²¢¹¹œšcamera²ÎÊý£¬œ«ŸØÕóÐŽÈëcamera
	vector<double> focals;
	for (size_t i = 0; i < cameras.size(); ++i)
	{
		LOGI("camera # %d:\n",indices[i]+1);
		focals.push_back(cameras[i].focal);
	}

	sort(focals.begin(), focals.end());
	float warped_image_scale;
	if (focals.size() % 2 == 1)
		warped_image_scale = static_cast<float>(focals[focals.size() / 2]);
	else
		warped_image_scale = static_cast<float>(focals[focals.size() / 2 - 1] + focals[focals.size() / 2]) * 0.5f;

	///²šÐÎœÃÕý
	vector<Mat> rmats;
	for (size_t i = 0; i < cameras.size(); ++i)
		rmats.push_back(cameras[i].R);
	waveCorrect(rmats, wave_correct);////²šÐÎœÃÕý
	for (size_t i = 0; i < cameras.size(); ++i)
		cameras[i].R = rmats[i];


	//cout<<"Warping images ... "<<endl;
	LOGI("Warping images...\n");


	vector<Point> corners(num_images);//Í³Ò»×ø±êºóµÄ¶¥µã
	vector<Mat> masks_warped(num_images);
	vector<Mat> images_warped(num_images);
	vector<Size> sizes(num_images);
	vector<Mat> masks(num_images);//ÈÚºÏÑÚÂë

	// ×Œ±žÍŒÏñÈÚºÏÑÚÂë
	for (int i = 0; i < num_images; ++i)
	{
		masks[i].create(images[i].size(), CV_8U);
		masks[i].setTo(Scalar::all(255));
	}

	//ÍäÇúÍŒÏñºÍÈÚºÏÑÚÂë

	Ptr<WarperCreator> warper_creator;
	warper_creator = new cv::SphericalWarper();

	Ptr<RotationWarper> warper = warper_creator->create(static_cast<float>(warped_image_scale * seam_work_aspect));

	for (int i = 0; i < num_images; ++i)
	{
		Mat_<float> K;
		cameras[i].K().convertTo(K, CV_32F);
		float swa = (float)seam_work_aspect;
		K(0,0) *= swa; K(0,2) *= swa;
		K(1,1) *= swa; K(1,2) *= swa;

		corners[i] = warper->warp(images[i], K, cameras[i].R, INTER_LINEAR, BORDER_REFLECT, images_warped[i]);//ŒÆËãÍ³Ò»ºó×ø±ê¶¥µã
		sizes[i] = images_warped[i].size();

		warper->warp(masks[i], K, cameras[i].R, INTER_NEAREST, BORDER_CONSTANT, masks_warped[i]);//ÍäÇúµ±Ç°ÍŒÏñ
	}

	vector<Mat> images_warped_f(num_images);
	for (int i = 0; i < num_images; ++i)
		images_warped[i].convertTo(images_warped_f[i], CV_32F);


	Ptr<ExposureCompensator> compensator = ExposureCompensator::createDefault(expos_comp_type);//œšÁ¢²¹³¥Æ÷ÒÔœøÐÐ¹ØÕÕ²¹³¥£¬²¹³¥·œ·šÊÇgain_blocks
	compensator->feed(corners, images_warped, masks_warped);

	//²éÕÒœÓ·ì
	Ptr<SeamFinder> seam_finder;
	seam_finder = new detail::GraphCutSeamFinder(GraphCutSeamFinderBase::COST_COLOR);
	seam_finder->find(images_warped_f, corners, masks_warped);

	// ÊÍ·ÅÎŽÊ¹ÓÃµÄÄÚŽæ
	images.clear();
	images_warped.clear();
	images_warped_f.clear();
	masks.clear();

	//////ÍŒÏñÈÚºÏ
	//cout<<"Compositing..."<<endl;
	LOGI("Compositing...\n");

	Mat img_warped, img_warped_s;
	Mat dilated_mask, seam_mask, mask, mask_warped;
	Ptr<Blender> blender;

	double compose_work_aspect = 1;

	for (int img_idx = 0; img_idx < num_images; ++img_idx)
	{
		//cout<<"Compositing image #" << indices[img_idx]+1<<endl;
		LOGI("Compositing image # %d\n",indices[img_idx]+1);
		//ÓÉÓÚÒÔÇ°œøÐÐŽŠÀíµÄÍŒÆ¬¶ŒÊÇÒÔwork_scaleœøÐÐËõ·ÅµÄ£¬ËùÒÔÍŒÏñµÄÄÚ²Î
		//corner£šÍ³Ò»×ø±êºóµÄ¶¥µã£©£¬mask£šÈÚºÏµÄÑÚÂë£©¶ŒÐèÒªÖØÐÂŒÆËã

		// ¶ÁÈ¡ÍŒÏñºÍ×ö±ØÒªµÄµ÷Õû

		full_img1 = imread(img_names[img_idx]);
		resize(full_img1,full_img, Size(mwidth,mheight));
		compose_scale = min(1.0, sqrt(compose_megapix * 1e6 / full_img.size().area()));
		compose_work_aspect = compose_scale / work_scale;
		// žüÐÂÍäÇúÍŒÏñ±ÈÀý
		warped_image_scale *= static_cast<float>(compose_work_aspect);
		warper = warper_creator->create(warped_image_scale);

		// žüÐÂcornersºÍsizes
		for (int i = 0; i < num_images; ++i)
		{
			// žüÐÂÏà»úÒÔÏÂÌØÐÔ
			cameras[i].focal *= compose_work_aspect;
			cameras[i].ppx *= compose_work_aspect;
			cameras[i].ppy *= compose_work_aspect;

			// žüÐÂcornersºÍsizes
			Size sz = full_img_sizes[i];
			if (std::abs(compose_scale - 1) > 1e-1)
			{
				sz.width = cvRound(full_img_sizes[i].width * compose_scale);
				sz.height = cvRound(full_img_sizes[i].height * compose_scale);
			}

			Mat K;
			cameras[i].K().convertTo(K, CV_32F);
			Rect roi = warper->warpRoi(sz, K, cameras[i].R);
			corners[i] = roi.tl();
			sizes[i] = roi.size();
		}

		if (abs(compose_scale - 1) > 1e-1)
			resize(full_img, img, Size(), compose_scale, compose_scale);
		else
			img = full_img;
		full_img.release();
		Size img_size = img.size();

		Mat K;
		cameras[img_idx].K().convertTo(K, CV_32F);
		// Å€Çúµ±Ç°ÍŒÏñ
		warper->warp(img, K, cameras[img_idx].R, INTER_LINEAR, BORDER_REFLECT, img_warped);
		// Å€Çúµ±Ç°ÍŒÏñÑÚÄ£
		mask.create(img_size, CV_8U);
		mask.setTo(Scalar::all(255));
		warper->warp(mask, K, cameras[img_idx].R, INTER_NEAREST, BORDER_CONSTANT, mask_warped);

		// ÆØ¹â²¹³¥
		compensator->apply(img_idx, corners[img_idx], img_warped, mask_warped);

		img_warped.convertTo(img_warped_s, CV_16S);
		img_warped.release();
		img.release();
		mask.release();

		dilate(masks_warped[img_idx], dilated_mask, Mat());
		resize(dilated_mask, seam_mask, mask_warped.size());
		mask_warped = seam_mask & mask_warped;
		//³õÊŒ»¯blender
		if (blender.empty())
		{
			blender = Blender::createDefault(blend_type, try_gpu);
			Size dst_sz = resultRoi(corners, sizes).size();
			float blend_width = sqrt(static_cast<float>(dst_sz.area())) * blend_strength / 100.f;
			if (blend_width < 1.f)
				blender = Blender::createDefault(Blender::NO, try_gpu);
			else
			{
				MultiBandBlender* mb = dynamic_cast<MultiBandBlender*>(static_cast<Blender*>(blender));
				mb->setNumBands(static_cast<int>(ceil(log(blend_width)/log(2.)) - 1.));
				//cout<<"Multi-band blender, number of bands: " << mb->numBands()<<endl;
			}
			//žùŸÝcorners¶¥µãºÍÍŒÏñµÄŽóÐ¡È·¶š×îÖÕÈ«Ÿ°ÍŒµÄ³ßŽç
			blender->prepare(corners, sizes);
		}

		// // ÈÚºÏµ±Ç°ÍŒÏñ
		blender->feed(img_warped_s, mask_warped, corners[img_idx]);
	}

	Mat result, result_mask;
	blender->blend(result, result_mask);

	imwrite(result_name, result);

	finish=clock();
	totaltime=(double)(finish-start)/CLOCKS_PER_SEC;
	//cout<<"\nŽË³ÌÐòµÄÔËÐÐÊ±ŒäÎª"<<totaltime<<"Ãë£¡"<<endl;
	LOGI("TOTAL TIME IS %f\n",totaltime);
	
	//release
	vector<string>().swap(img_names);
	 //system("pause");
	return 0;
}

JNIEXPORT jdouble JNICALL Java_com_zy_photostitching_ImageProc_getTime
		(JNIEnv *env, jclass obj)
{
	return (jdouble)totaltime;
}

JNIEXPORT jstring JNICALL Java_com_zy_photostitching_ImageProc_getResultName
		(JNIEnv *env, jclass obj)
{
	LOGI("%s\n",result_name.c_str());
	return stoJstring(env,result_name.c_str());
	
}

