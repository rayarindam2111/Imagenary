#include <iostream>
using namespace std;
#include "opencv2\core\core.hpp"
#include "opencv2\highgui\highgui.hpp"
#include "opencv2\imgproc\imgproc.hpp"
using namespace cv;
#include<Windows.h>

POINT t1;
int thresh=10;
int clickThreshR=0;
int clickThreshB=0;
bool nomove=true;
bool clickon=true;
int mouseon=0;
int screenHeight=GetSystemMetrics(SM_CYSCREEN)+200;
int screenWidth=GetSystemMetrics(SM_CXSCREEN)+300;
int camWidth=752;
int camHeight=416;


void thresFunction(int x,void* data)
{
	*(int*)data = x;
}

void LeftClick()
{  
  INPUT Input={0};
  // left down 
  Input.type= INPUT_MOUSE;
  Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
  ::SendInput(1,&Input,sizeof(INPUT));
  // left up
  ::ZeroMemory(&Input,sizeof(INPUT));
  Input.type      = INPUT_MOUSE;
  Input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
  ::SendInput(1,&Input,sizeof(INPUT));
}

void RightClick()
{
	INPUT Input={0};
  // Right down 
  Input.type= INPUT_MOUSE;
  Input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
  ::SendInput(1,&Input,sizeof(INPUT));
  // Right up
  ::ZeroMemory(&Input,sizeof(INPUT));
  Input.type      = INPUT_MOUSE;
  Input.mi.dwFlags  = MOUSEEVENTF_RIGHTUP;
  ::SendInput(1,&Input,sizeof(INPUT));
}

void detectClickR(double curArea, double lastArea)
{
	if((curArea-lastArea)>clickThreshR)
	{	clickon=false;
		cout<<"Click!!! LEFT"<<endl;
		if (mouseon) 
		{
			LeftClick();
			clickon=true;
		}
	}
	else
		cout<< "No Click! LEFT"<<endl;
}
void detectClickB(double curArea, double lastArea)
{
	if((curArea-lastArea)>clickThreshB)
	{	clickon=false;
		cout<<"Click!!! RIGHT"<<endl;
		if (mouseon) 
		{
			RightClick();
			clickon=true;
		}
	}
	else
		cout<< "No Click! RIGHT"<<endl;
}
void leftX()
{   
	nomove=false;
	cout << "Left" <<endl;
	if(mouseon) SetCursorPos(t1.x-mouseon,t1.y);
}
void rightX()
{   
	nomove=false;
	cout << "Right" <<endl;
	if(mouseon) SetCursorPos(t1.x+mouseon,t1.y);
}
void upY()
{   nomove=false;
	cout << "Up" <<endl;
	if(mouseon) SetCursorPos(t1.x,t1.y+mouseon);
}
void downY()
{   
	nomove=false;
	cout << "Down" <<endl;
	if(mouseon) SetCursorPos(t1.x,t1.y-mouseon);
}


void checkmotionR(double curValX,double curValY, double lastValX,double lastValY)
{
	if(mouseon && clickon)
	{
		if(abs(curValX-lastValX)>thresh)
			SetCursorPos((curValX/camWidth)*screenWidth,(curValY/camHeight)*screenHeight);
		if(abs(curValY-lastValY)>thresh)
			SetCursorPos((curValX/camWidth)*screenWidth,(curValY/camHeight)*screenHeight);
	}
}

/*
void checkmotion(double curValX,double curValY, double lastValX,double lastValY)
{
	nomove=true;
	GetCursorPos(&t1);
	if(curValX>lastValX+thresh)
		 {rightX();}
	if(curValY>lastValY+thresh)
		{downY();}
	if(curValX<lastValX-thresh)
		{leftX();}
	if(curValY<lastValY-thresh)
		{upY();}
	if(nomove)
		cout<< "No mov" <<endl;
}
*/

int main(int argc, char** argv)
{   
	VideoCapture cam(0);
	Moments momentR,momentB;
	Mat frame,frameR,frameB;
	double xmeanCurR,ymeanCurR,xmeanLastR=0,ymeanLastR=0,xmeanCurB,ymeanCurB,xmeanLastB=0,ymeanLastB=0;
	double curAreaR,lastAreaR=0,curAreaB,lastAreaB=0;
	
	int RLr=0,RHr=0,GLr=0,GHr=0,BLr=0,BHr=0;
	int RLb=0,RHb=0,GLb=0,GHb=0,BLb=0,BHb=0;

	namedWindow("WinR");namedWindow("WinB");
	namedWindow("RedThreshold");
	namedWindow("BlueThreshold");
	namedWindow("Adjustments");

	CV_Assert(cam.isOpened());
	
	cam.set(CV_CAP_PROP_FRAME_WIDTH,camWidth);
	cam.set(CV_CAP_PROP_FRAME_HEIGHT,camHeight);

	
	/* FOR CAMERA PROBLEM */
	do
	{cam.read(frame);
	}while(frame.empty());
	/* FOR CAMERA PROBLEM */

	int sizethreshinit = 150;

	createTrackbar("pxThresh","Adjustments",NULL,255,thresFunction,&thresh);
	createTrackbar("sizeThreshR","Adjustments",&sizethreshinit,2000,thresFunction,&clickThreshR);
	createTrackbar("sizeThreshB","Adjustments",&sizethreshinit,2000,thresFunction,&clickThreshB);
	createTrackbar("MouseMove","Adjustments",NULL,100,thresFunction,&mouseon);

	createTrackbar("R_HIGHr","RedThreshold",&RHr,255,thresFunction,&RHr);
	createTrackbar("R_LOWr","RedThreshold",&RLr,255,thresFunction,&RLr);
	createTrackbar("G_HIGHr","RedThreshold",&GHr,255,thresFunction,&GHr);
	createTrackbar("G_LOWr","RedThreshold",&GLr,255,thresFunction,&GLr);
	createTrackbar("B_HIGHr","RedThreshold",&BHr,255,thresFunction,&BHr);
	createTrackbar("B_LOWr","RedThreshold",&BLr,255,thresFunction,&BLr);

	createTrackbar("R_HIGHb","BlueThreshold",&RHb,255,thresFunction,&RHb);
	createTrackbar("R_LOWb","BlueThreshold",&RLb,255,thresFunction,&RLb);
	createTrackbar("G_HIGHb","BlueThreshold",&GHb,255,thresFunction,&GHb);
	createTrackbar("G_LOWb","BlueThreshold",&GLb,255,thresFunction,&GLb);
	createTrackbar("B_HIGHb","BlueThreshold",&BHb,255,thresFunction,&BHb);
	createTrackbar("B_LOWb","BlueThreshold",&BLb,255,thresFunction,&BLb);

	while(true)
	{
		cam.read(frame); // Main camera feed
				
		// INITIAL IMAGE OPERATION
		GaussianBlur(frame,frame,Size(11,11),2); // Smoothing
		inRange(frame,Scalar(CV_RGB(RLr,GLr,BLr)),Scalar(CV_RGB(RHr,GHr,BHr)),frameR);
						// Red marker separated
		inRange(frame,Scalar(CV_RGB(RLb,GLb,BLb)),Scalar(CV_RGB(RHb,GHb,BHb)),frameB);
						// Blue marker separated
		dilate(frameR,frameR,getStructuringElement(MORPH_ELLIPSE,Size(4,4)));
		dilate(frameB,frameB,getStructuringElement(MORPH_ELLIPSE,Size(4,4)));
		// INITIAL IMAGE OPERATION


		// PARAMETER CALCULATION FOR RED MARKER
		momentR = moments(frameR,true);
		xmeanCurR = momentR.m10/momentR.m00;
		ymeanCurR = momentR.m01/momentR.m00;
		curAreaR = momentR.m00;
		// PARAMETER CALCULATION FOR RED MARKER

		// PARAMETER CALCULATION FOR BLUE MARKER
		momentB = moments(frameB,true);
		xmeanCurB = momentB.m10/momentB.m00;
		ymeanCurB = momentB.m01/momentB.m00;
		curAreaB = momentB.m00;
		// PARAMETER CALCULATION FOR BLUE MARKER


		// MAIN MOUSE EVENTS
		checkmotionR(xmeanCurR,ymeanCurR,xmeanLastR,ymeanLastR);
		detectClickR(curAreaR,lastAreaR);
		detectClickB(curAreaB,lastAreaB);
		// MAIN MOUSE EVENTS
		
		// UPDATIONS
		xmeanLastR=xmeanCurR;
		ymeanLastR=ymeanCurR;
		lastAreaR = curAreaR;

		lastAreaB = curAreaB;	// FOR RIGHT CLICK 'SIZETHRESH'
		// UPDATIONS

		// SHOW WINDOWS
		imshow("WinR",frameR);waitKey(10);
		imshow("WinB",frameB);waitKey(10);
		// SHOW WINDOWS
	}
	return 0;
}