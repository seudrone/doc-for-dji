#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include <fstream>
#include <iostream>
#include "DJI_guidance.h"
#include "DJI_utility.h"

using namespace cv;
using namespace std;

bool selectObject = false;//代表是否在选要跟踪的初始目标，true表示正在用鼠标选择
int trackObject = 0;//代表跟踪目标数目
Point origin;//用于保存鼠标选择第一次单击时点的位置
Rect selection;//用于保存鼠标选择的矩形框
DJI_event g_event;
DJI_lock g_lock;
#define WIDTH 320
#define HEIGHT 240
#define IMAGE_SIZE (HEIGHT * WIDTH)
float zhangaijuli=0.0f;
//static void onMouse( int event, int x, int y, int, void* )
//{
//    if( selectObject )
//    {
//        selection.x = MIN(x, origin.x);
//        selection.y = MIN(y, origin.y);
//        selection.width = std::abs(x - origin.x);
//        selection.height = std::abs(y - origin.y);
//
//        selection &= Rect(0, 0, image.cols, image.rows);
//    }
//
//    switch( event )
//    {
//    case CV_EVENT_LBUTTONDOWN:
//        origin = Point(x,y);
//        selection = Rect(x,y,0,0);
//        selectObject = true;
//        break;
//    case CV_EVENT_LBUTTONUP:
//        selectObject = false;
//        if( selection.width > 0 && selection.height > 0 )
//            trackObject = -1;
//        break;
//    }
//}

static void help()
{
    cout << "\nThis is a demo that shows camshift based tracking with Guidance.\n"
            "You select an objects such as your face and it tracks it.\n"
            "This reads from greyscale and depth images from DJI Guidance.\n"
            "Usage: \n"
            "   ./camshift.exe\n";

    cout << "\n\nHot keys: \n"
            "\tESC/q - quit the program\n"
            "\tc - stop the tracking\n"
            "\tb - switch to/from backprojection view\n"
            "\tp - pause video\n"
            "To initialize tracking, select the object with mouse\n";
}


//然后定义一个回调函数，当订阅的数据到来时程序会自动调用该函数。
Mat g_imleft(HEIGHT, WIDTH, CV_8U);
Mat g_imright(HEIGHT, WIDTH, CV_8U);
Mat	g_depth(HEIGHT, WIDTH, CV_16SC1);
e_vbus_index selected_vbus = e_vbus4;  // 选择摄像头
string winDemoName = "Guidance Tracking Demo";

Mat g_iml[CAMERA_PAIR_NUM];
Mat g_imr[CAMERA_PAIR_NUM];
Mat g_dep[CAMERA_PAIR_NUM];

int my_callback( int data_type, int data_len, char *content )
{
//	printf("enter callback..\n");
	g_lock.enter();
	if ( e_image == data_type && NULL != content )
	{
//		printf("callback: type is image..\n");
		image_data *data=(image_data*)content;//后面这句话也可以这样写，image_data data;然后加上后面这句话
		//memcpy((char*)&data, content, sizeof(data));//memcpy函数的功能是从源src所指的内存地址的起始位置开始拷贝sizeof个字节到目标dest所指的内存地址的起始位置中。
		memcpy(g_imleft.data, data->m_greyscale_image_left[selected_vbus], IMAGE_SIZE);
		memcpy(g_imright.data, data->m_greyscale_image_right[selected_vbus], IMAGE_SIZE);
		memcpy(g_depth.data, data->m_depth_image[selected_vbus], IMAGE_SIZE * 2);
	}

	//if ( e_image == data_type && NULL != content )
	//{
	//	image_data *data=(image_data*)content;
	//	for(int d=0;d<CAMERA_PAIR_NUM;d++)
	//	{
	//		if(data->m_greyscale_image_left[d])
	//		{
	//			g_iml[d]=Mat::zeros(HEIGHT, WIDTH, CV_8U);
	//			memcpy(g_iml[d].data, data->m_greyscale_image_left[d], IMAGE_SIZE);
	//		}else g_iml[d].release();
	//		if(data->m_greyscale_image_right[d])
	//		{
	//			g_imr[d]=Mat::zeros(HEIGHT, WIDTH, CV_8U);
	//			memcpy(g_imr[d].data, data->m_greyscale_image_right[d], IMAGE_SIZE);
	//		}else g_imr[d].release();
	//		if(data->m_depth_image[d])
	//		{
	//			g_dep[d]=Mat::zeros(HEIGHT, WIDTH, CV_16SC1);
	//			memcpy(g_dep[d].data, data->m_depth_image[d], IMAGE_SIZE* 2);
	//		}else g_iml[d].release();
	//	}
	//}
	//if ( e_imu == data_type && NULL != content )
	//{
	//	imu *imu_data = (imu*)content;
	//	printf( "imu:%f %f %f,%f %f %f %f\n", imu_data->acc_x, imu_data->acc_y, imu_data->acc_z, imu_data->q[0], imu_data->q[1], imu_data->q[2], imu_data->q[3] );
	//	printf( "frame index:%d,stamp:%d\n", imu_data->frame_index, imu_data->time_stamp );
	//}

	if ( e_obstacle_distance == data_type && NULL != content )
	{
		obstacle_distance *oa = (obstacle_distance*)content;
		printf( "obstacle distance:" );
//		for ( int i = 0; i < CAMERA_PAIR_NUM; ++i )
			printf( " %f ", 0.01f * oa->distance[4] );
		zhangaijuli=0.01f * oa->distance[4];
		printf( "\n" );
		//printf( "frame index:%d,stamp:%d\n", oa->frame_index, oa->time_stamp );
	}

	g_lock.leave();//g_lock是与线程操作保护有关的对象
	g_event.set_event();//g_event是与数据读取事件触发有关的对象，二者在DJI_utility.h中均有定义
	return 0;
}

//当初始化、订阅或停止传输等操作失败时，返回为非0的int类型值，因此用户可以通过这些操作函数的返回值来判断失败的原因。我们这里宏定义两个操作，
//如果操作失败，则打印错误码或者直接终止程序；发生错误时，err_code非零
#define RETURN_IF_ERR(err_code) { if( err_code ){ printf( "USB error code:%d in file %s %d\n", err_code, __FILE__, __LINE__ );}}
#define RELEASE_IF_ERR(err_code) { if( err_code ){ release_transfer(); printf( "USB error code:%d in file %s %d\n", err_code, __FILE__, __LINE__ );}}

int main( int argc, const char** argv )
{
	if(argc>1)
		help();
	//* 准备写入视频 */
//	VideoWriter	vWriter("result.avi", CV_FOURCC('M','J','P','G'), 25, Size(WIDTH, HEIGHT), false);
	 /* 跟踪窗口 */
 //   Rect trackWindow;
	//int hsize[] = {16, 16};
 //   float hranges[] = {0, 255};
	//float dranges[] = {0, 255};
	//
	////const float* phranges[] = { hranges, dranges };

	//selection = Rect(10,10,100,100);
	//trackObject = 0;

  //  namedWindow( winDemoName, 0 );
	 ///* 等待鼠标框出 */
  //  setMouseCallback( winDemoName, onMouse, 0 );

//    Mat imcolor, mask, hist, hist_of_object, backproj;
    bool paused = false;
//	float hist_update_ratio = 0.2f;

	// Connect to Guidance and subscribe data/* 初始化Guidance数据传输 */
	reset_config();
	int err_code = init_transfer();
	RETURN_IF_ERR(err_code);

//	select_imu();//我加的订阅数据
	select_obstacle_distance();//wojiade 
	//for(int d=0;d<CAMERA_PAIR_NUM;d++)
	//{
	//	/* 订阅selected_vbus左侧灰度图 */  
	//	err_code = select_greyscale_image((e_vbus_index)d, true);
	//	RELEASE_IF_ERR(err_code);
	//	 /* 订阅selected_vbus右侧灰度图 */ 
	//	err_code = select_greyscale_image((e_vbus_index)d, false);
	//	RELEASE_IF_ERR(err_code);
	//}
	/* 订阅selected_vbus左侧灰度图 */  
	err_code = select_greyscale_image(selected_vbus, true);
	RELEASE_IF_ERR(err_code);
		/* 订阅selected_vbus右侧灰度图 */ 
	err_code = select_greyscale_image(selected_vbus, false);
	RELEASE_IF_ERR(err_code);
	/* 订阅selected_vbus深度图 */
	err_code = select_depth_image(selected_vbus);
	RELEASE_IF_ERR(err_code);
	
	/* 设置数据到来时的回调函数*/
	err_code = set_sdk_event_handler(my_callback);
	RELEASE_IF_ERR(err_code);
	/* 开启数据传输线程 */
	err_code = start_transfer();
	RELEASE_IF_ERR(err_code);

	Mat image;

	Mat depth(HEIGHT, WIDTH, CV_8UC1);
	/* 设置传输事件循环次数 */

	string greyimage[]={"down","front","right","back","left"};

	int a=0;
	char image_name[13];  //作为保存图片的内存空间
    for( int times = 0; times < 30000; ++times )
    {
		/* 等待数据到来 */
		g_event.wait_event();

		// filter depth image过滤
		filterSpeckles(g_depth, -16, 50, 20);
		// convert 16 bit depth image to 8 bit
		g_depth.convertTo(depth, CV_8UC1);
		/* 显示深度图 */
		imshow("depth", depth);
		waitKey(1);
		/* 获得灰度图 */
		g_imleft.copyTo(image);
		imshow("image", image);
//		for(int d=0;d<CAMERA_PAIR_NUM;d++)
//		{
////			namedWindow(greyimage[d]);
//			imshow(greyimage[4],g_iml[4]);
//			moveWindow(greyimage[d],50+320*d,200);
//		}

/*        if( !paused )
        {
			vector<Mat> ims(3);
			ims[0] = image;
			ims[1] = ims[2] = depth;
			merge(ims, imcolor);//合并成imcolor
			// 如果鼠标框出了目标，则进行跟踪及效果显示 
	
			//sprintf(image_name, "%s%d%s", "IMAGE", ++a, ".jpg");//保存的图片名  
			//if((a%10)==0){imwrite(image_name,depth);}

            if( trackObject )
            {
				int ch[] = {0,1};
                if( trackObject < 0 )//trackObject初始化为0,或者按完键盘的'c'键后也为0，当鼠标单击松开后为-1
                {
					//此处的构造函数roi用的是Mat hue的矩阵头，且roi的数据指针指向hue，即共用相同的数据，select为其感兴趣的区域
                    Mat roi(imcolor, selection);
//calcHist()函数第一个参数为输入矩阵序列，第2个参数表示输入的矩阵数目，第3个参数表示将被计算直方图维数通道的列表，第4个参数表示可选的掩码函数
//第5个参数表示输出直方图，第6个参数表示直方图的维数，第7个参数为每一维直方图数组的大小，第8个参数为每一维直方图bin的边界
                    calcHist(&roi, 1, &ch[0], Mat(), hist, 2, &hsize[0], &phranges[0]);//将roi的0通道计算直方图并通过mask放入hist中，hsize为每一维直方图
                    normalize(hist, hist, 0, 255, CV_MINMAX);//将hist矩阵进行数组范围归一化，都归一化到0~255


					if(hist_of_object.empty())
						hist_of_object = hist;
					else
						hist_of_object = (1-hist_update_ratio)*hist_of_object + hist_update_ratio*hist;

                    trackWindow = selection;
                    trackObject = 1;
                }

                calcBackProject(&imcolor, 1, ch, hist_of_object, backproj, phranges);
				//RotatedRect CamShift(InputArray probImage, Rect& window, TermCriteria criteria)。
				//其中probImage为输入图像直方图的反向投影图，window为要跟踪目标的初始位置矩形框，criteria为算法结束条件。函数返回一个有方向角度的矩阵。
				//该函数的实现首先是利用meanshift算法计算出要跟踪的中心，然后调整初始窗口的大小位置和方向角度。在camshift内部调用了meanshift算法计算目标的重心。
                RotatedRect trackBox = CamShift(backproj, trackWindow, //trackWindow为鼠标选择的区域，TermCriteria为确定迭代终止
                                    TermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ));
				
				if (trackWindow.area() <= 1)//是通过max_num_of_trees_in_the_forest  
                {
                    int cols = backproj.cols, rows = backproj.rows, r = (MIN(cols, rows) + 5)/6;
                    trackWindow = Rect(trackWindow.x - r, trackWindow.y - r,
                                       trackWindow.x + r, trackWindow.y + r) &
                                  Rect(0, 0, cols, rows);//Rect函数为矩阵的偏移和大小，即第一二个参数为矩阵的左上角点坐标，第三四个参数为矩阵的宽和高

                }
				if( trackWindow.area() <= 1 )

					break;

                ellipse( image, trackBox, Scalar(0,0,255), 3, CV_AA );
            }
			else if( trackObject < 0 )
			{
				paused = false;
			}
        }
*/
/*        if( selectObject && selection.width > 0 && selection.height > 0 )
        {
            Mat roi(image, selection);
            bitwise_not(roi, roi);
        }*/

//        imshow( winDemoName, image );
//		vWriter<<image;
        char c = (char)waitKey(40);
		/* 按q键退出 */
        if( c == 27 || c=='q')
		{
            break;
		}
		else if(c=='a')
		{
		sprintf(image_name, "%s%d  %f%s", "路径", ++a,zhangaijuli, ".jpg");//保存的图片名  
		imwrite(image_name,image);
		}
  //      switch(c)
  //      {
  //      case 'c':
  //          trackObject = 0;            
  //          break;
  //      case 'p':
		//case ' ':
  //          paused = !paused;
  //          break;
  //      default:
  //          ;
  //      }
    }
	/* 停止数据传输 */
	err_code = stop_transfer();
	RELEASE_IF_ERR(err_code);
	/* 确保停止 */
	sleep(100000);
	/* 释放传输线程 */
	err_code = release_transfer();
	RETURN_IF_ERR(err_code);

    return 0;
}
