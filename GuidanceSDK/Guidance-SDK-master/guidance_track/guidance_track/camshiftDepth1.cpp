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

bool selectObject = false;//�����Ƿ���ѡҪ���ٵĳ�ʼĿ�꣬true��ʾ���������ѡ��
int trackObject = 0;//�������Ŀ����Ŀ
Point origin;//���ڱ������ѡ���һ�ε���ʱ���λ��
Rect selection;//���ڱ������ѡ��ľ��ο�
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


//Ȼ����һ���ص������������ĵ����ݵ���ʱ������Զ����øú�����
Mat g_imleft(HEIGHT, WIDTH, CV_8U);
Mat g_imright(HEIGHT, WIDTH, CV_8U);
Mat	g_depth(HEIGHT, WIDTH, CV_16SC1);
e_vbus_index selected_vbus = e_vbus4;  // ѡ������ͷ
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
		image_data *data=(image_data*)content;//������仰Ҳ��������д��image_data data;Ȼ����Ϻ�����仰
		//memcpy((char*)&data, content, sizeof(data));//memcpy�����Ĺ����Ǵ�Դsrc��ָ���ڴ��ַ����ʼλ�ÿ�ʼ����sizeof���ֽڵ�Ŀ��dest��ָ���ڴ��ַ����ʼλ���С�
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

	g_lock.leave();//g_lock�����̲߳��������йصĶ���
	g_event.set_event();//g_event�������ݶ�ȡ�¼������йصĶ��󣬶�����DJI_utility.h�о��ж���
	return 0;
}

//����ʼ�������Ļ�ֹͣ����Ȳ���ʧ��ʱ������Ϊ��0��int����ֵ������û�����ͨ����Щ���������ķ���ֵ���ж�ʧ�ܵ�ԭ����������궨������������
//�������ʧ�ܣ����ӡ���������ֱ����ֹ���򣻷�������ʱ��err_code����
#define RETURN_IF_ERR(err_code) { if( err_code ){ printf( "USB error code:%d in file %s %d\n", err_code, __FILE__, __LINE__ );}}
#define RELEASE_IF_ERR(err_code) { if( err_code ){ release_transfer(); printf( "USB error code:%d in file %s %d\n", err_code, __FILE__, __LINE__ );}}

int main( int argc, const char** argv )
{
	if(argc>1)
		help();
	//* ׼��д����Ƶ */
//	VideoWriter	vWriter("result.avi", CV_FOURCC('M','J','P','G'), 25, Size(WIDTH, HEIGHT), false);
	 /* ���ٴ��� */
 //   Rect trackWindow;
	//int hsize[] = {16, 16};
 //   float hranges[] = {0, 255};
	//float dranges[] = {0, 255};
	//
	////const float* phranges[] = { hranges, dranges };

	//selection = Rect(10,10,100,100);
	//trackObject = 0;

  //  namedWindow( winDemoName, 0 );
	 ///* �ȴ������ */
  //  setMouseCallback( winDemoName, onMouse, 0 );

//    Mat imcolor, mask, hist, hist_of_object, backproj;
    bool paused = false;
//	float hist_update_ratio = 0.2f;

	// Connect to Guidance and subscribe data/* ��ʼ��Guidance���ݴ��� */
	reset_config();
	int err_code = init_transfer();
	RETURN_IF_ERR(err_code);

//	select_imu();//�ҼӵĶ�������
	select_obstacle_distance();//wojiade 
	//for(int d=0;d<CAMERA_PAIR_NUM;d++)
	//{
	//	/* ����selected_vbus���Ҷ�ͼ */  
	//	err_code = select_greyscale_image((e_vbus_index)d, true);
	//	RELEASE_IF_ERR(err_code);
	//	 /* ����selected_vbus�Ҳ�Ҷ�ͼ */ 
	//	err_code = select_greyscale_image((e_vbus_index)d, false);
	//	RELEASE_IF_ERR(err_code);
	//}
	/* ����selected_vbus���Ҷ�ͼ */  
	err_code = select_greyscale_image(selected_vbus, true);
	RELEASE_IF_ERR(err_code);
		/* ����selected_vbus�Ҳ�Ҷ�ͼ */ 
	err_code = select_greyscale_image(selected_vbus, false);
	RELEASE_IF_ERR(err_code);
	/* ����selected_vbus���ͼ */
	err_code = select_depth_image(selected_vbus);
	RELEASE_IF_ERR(err_code);
	
	/* �������ݵ���ʱ�Ļص�����*/
	err_code = set_sdk_event_handler(my_callback);
	RELEASE_IF_ERR(err_code);
	/* �������ݴ����߳� */
	err_code = start_transfer();
	RELEASE_IF_ERR(err_code);

	Mat image;

	Mat depth(HEIGHT, WIDTH, CV_8UC1);
	/* ���ô����¼�ѭ������ */

	string greyimage[]={"down","front","right","back","left"};

	int a=0;
	char image_name[13];  //��Ϊ����ͼƬ���ڴ�ռ�
    for( int times = 0; times < 30000; ++times )
    {
		/* �ȴ����ݵ��� */
		g_event.wait_event();

		// filter depth image����
		filterSpeckles(g_depth, -16, 50, 20);
		// convert 16 bit depth image to 8 bit
		g_depth.convertTo(depth, CV_8UC1);
		/* ��ʾ���ͼ */
		imshow("depth", depth);
		waitKey(1);
		/* ��ûҶ�ͼ */
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
			merge(ims, imcolor);//�ϲ���imcolor
			// ����������Ŀ�꣬����и��ټ�Ч����ʾ 
	
			//sprintf(image_name, "%s%d%s", "IMAGE", ++a, ".jpg");//�����ͼƬ��  
			//if((a%10)==0){imwrite(image_name,depth);}

            if( trackObject )
            {
				int ch[] = {0,1};
                if( trackObject < 0 )//trackObject��ʼ��Ϊ0,���߰�����̵�'c'����ҲΪ0������굥���ɿ���Ϊ-1
                {
					//�˴��Ĺ��캯��roi�õ���Mat hue�ľ���ͷ����roi������ָ��ָ��hue����������ͬ�����ݣ�selectΪ�����Ȥ������
                    Mat roi(imcolor, selection);
//calcHist()������һ������Ϊ����������У���2��������ʾ����ľ�����Ŀ����3��������ʾ��������ֱ��ͼά��ͨ�����б���4��������ʾ��ѡ�����뺯��
//��5��������ʾ���ֱ��ͼ����6��������ʾֱ��ͼ��ά������7������Ϊÿһάֱ��ͼ����Ĵ�С����8������Ϊÿһάֱ��ͼbin�ı߽�
                    calcHist(&roi, 1, &ch[0], Mat(), hist, 2, &hsize[0], &phranges[0]);//��roi��0ͨ������ֱ��ͼ��ͨ��mask����hist�У�hsizeΪÿһάֱ��ͼ
                    normalize(hist, hist, 0, 255, CV_MINMAX);//��hist����������鷶Χ��һ��������һ����0~255


					if(hist_of_object.empty())
						hist_of_object = hist;
					else
						hist_of_object = (1-hist_update_ratio)*hist_of_object + hist_update_ratio*hist;

                    trackWindow = selection;
                    trackObject = 1;
                }

                calcBackProject(&imcolor, 1, ch, hist_of_object, backproj, phranges);
				//RotatedRect CamShift(InputArray probImage, Rect& window, TermCriteria criteria)��
				//����probImageΪ����ͼ��ֱ��ͼ�ķ���ͶӰͼ��windowΪҪ����Ŀ��ĳ�ʼλ�þ��ο�criteriaΪ�㷨������������������һ���з���Ƕȵľ���
				//�ú�����ʵ������������meanshift�㷨�����Ҫ���ٵ����ģ�Ȼ�������ʼ���ڵĴ�Сλ�úͷ���Ƕȡ���camshift�ڲ�������meanshift�㷨����Ŀ������ġ�
                RotatedRect trackBox = CamShift(backproj, trackWindow, //trackWindowΪ���ѡ�������TermCriteriaΪȷ��������ֹ
                                    TermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ));
				
				if (trackWindow.area() <= 1)//��ͨ��max_num_of_trees_in_the_forest  
                {
                    int cols = backproj.cols, rows = backproj.rows, r = (MIN(cols, rows) + 5)/6;
                    trackWindow = Rect(trackWindow.x - r, trackWindow.y - r,
                                       trackWindow.x + r, trackWindow.y + r) &
                                  Rect(0, 0, cols, rows);//Rect����Ϊ�����ƫ�ƺʹ�С������һ��������Ϊ��������Ͻǵ����꣬�����ĸ�����Ϊ����Ŀ�͸�

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
		/* ��q���˳� */
        if( c == 27 || c=='q')
		{
            break;
		}
		else if(c=='a')
		{
		sprintf(image_name, "%s%d  %f%s", "·��", ++a,zhangaijuli, ".jpg");//�����ͼƬ��  
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
	/* ֹͣ���ݴ��� */
	err_code = stop_transfer();
	RELEASE_IF_ERR(err_code);
	/* ȷ��ֹͣ */
	sleep(100000);
	/* �ͷŴ����߳� */
	err_code = release_transfer();
	RETURN_IF_ERR(err_code);

    return 0;
}
