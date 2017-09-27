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

Mat image;

bool selectObject = false;
int trackObject = 0;
Point origin;
Rect selection;
DJI_event g_event;
DJI_lock g_lock;
#define WIDTH 320
#define HEIGHT 240
#define IMAGE_SIZE (HEIGHT * WIDTH)

void canny(cv::Mat& img,cv::Mat& out)
{
	if(img.channels()==3)
		cv::cvtColor(img,out,CV_BGR2GRAY);
	cv::Canny(out,out,100,200);
	cv::threshold(out,out,128,255,cv::THRESH_BINARY_INV);
}

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
e_vbus_index selected_vbus = e_vbus4;  // select front vbus
string winDemoName = "Guidance Tracking Demo";

int my_callback( int data_type, int data_len, char *content )
{
	printf("enter callback..\n");
	g_lock.enter();
	if ( e_image == data_type && NULL != content )
	{
		printf("callback: type is image..\n");
		image_data data;
		memcpy((char*)&data, content, sizeof(data));//memcpy�����Ĺ����Ǵ�Դsrc(content)��ָ���ڴ��ַ����ʼλ�ÿ�ʼ����n���ֽڵ�Ŀ��dest��ָ���ڴ��ַ����ʼλ���С�
		memcpy(g_imleft.data, data.m_greyscale_image_left[selected_vbus], IMAGE_SIZE);
		memcpy(g_imright.data, data.m_greyscale_image_right[selected_vbus], IMAGE_SIZE);
		memcpy(g_depth.data, data.m_depth_image[selected_vbus], IMAGE_SIZE * 2);
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
	VideoWriter	vWriter("result2.avi", CV_FOURCC('M','J','P','G'), 25, Size(WIDTH, HEIGHT), false);
	 /* ���ٴ��� */
    Rect trackWindow;
	int hsize[] = {16, 16};
    float hranges[] = {0, 255};
	float dranges[] = {0, 255};
	
	const float* phranges[] = { hranges, dranges };

	selection = Rect(10,10,100,100);
	trackObject = 0;

    namedWindow( winDemoName, 0 );
	 /* �ȴ������ */
    //setMouseCallback( winDemoName, onMouse, 0 );

    Mat imcolor, mask, hist, hist_of_object, backproj;
    bool paused = false;
	float hist_update_ratio = 0.2f;

	// Connect to Guidance and subscribe data/* ��ʼ��Guidance���ݴ��� */
	reset_config();
	int err_code = init_transfer();
	RETURN_IF_ERR(err_code);
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

	Mat depth(HEIGHT, WIDTH, CV_8UC1);

	Mat result;//�Ҽӵ�
	/* ���ô����¼�ѭ������ */
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

		result=image.clone();

        if( !paused )
        {
			canny(result,result);
			namedWindow("result");
			imshow("result",result);
			vWriter<<result;
			//vector<Mat> ims(3);
			//ims[0] = image;
			//ims[1] = ims[2] = depth;
			//merge(ims, imcolor);//�ϲ���imcolor
			///* ����������Ŀ�꣬����и��ټ�Ч����ʾ */
   //         if( trackObject )
   //         {
			//	int ch[] = {0,1};
   //             if( trackObject < 0 )
   //             {
   //                 Mat roi(imcolor, selection);
			//		
   //                 calcHist(&roi, 1, &ch[0], Mat(), hist, 2, &hsize[0], &phranges[0]);
   //                 normalize(hist, hist, 0, 255, CV_MINMAX);

			//		if(hist_of_object.empty())
			//			hist_of_object = hist;
			//		else
			//			hist_of_object = (1-hist_update_ratio)*hist_of_object + hist_update_ratio*hist;

   //                 trackWindow = selection;
   //                 trackObject = 1;
   //             }

   //             calcBackProject(&imcolor, 1, ch, hist_of_object, backproj, phranges);
   //             RotatedRect trackBox = CamShift(backproj, trackWindow,
   //                                 TermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ));
			//	
			//	if (trackWindow.area() <= 1)
   //             {
   //                 int cols = backproj.cols, rows = backproj.rows, r = (MIN(cols, rows) + 5)/6;
   //                 trackWindow = Rect(trackWindow.x - r, trackWindow.y - r,
   //                                    trackWindow.x + r, trackWindow.y + r) &
   //                               Rect(0, 0, cols, rows);
   //             }
			//	if( trackWindow.area() <= 1 )
			//		break;

   //             ellipse( image, trackBox, Scalar(0,0,255), 3, CV_AA );
   //         }
			//else if( trackObject < 0 )
			//{
			//	paused = false;
			//}
        }

        //if( selectObject && selection.width > 0 && selection.height > 0 )
        //{
        //    Mat roi(image, selection);
        //    bitwise_not(roi, roi);
        //}

        imshow( winDemoName, image );
		//vWriter<<image;

        char c = (char)waitKey(10);
		/* ��q���˳� */
        if( c == 27 || c=='q')
		{
            break;
		}
        switch(c)
        {
        case 'c':
            trackObject = 0;            
            break;
        case 'p':
		case ' ':
            paused = !paused;
            break;
        default:
            ;
        }
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