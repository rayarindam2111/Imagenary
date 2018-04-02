#include "opencv2\core\core.hpp"
#include "opencv2\highgui\highgui.hpp"
#include "opencv2\imgproc\imgproc.hpp"
using namespace cv;
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;
#include<Windows.h>
#include<math.h>
#define maxNoOfKeys 33

namespace
{	
	Mat frame,frameBorder,frameFingers;
	vector<vector<Point>> contsBorder,contsFinger;
	vector<Vec4i> hierarchy;
	Rect rec,tempRec;
	POINT tempPoint;
	int iterations=0,ITER=20;
	int firstTime = 1;
	int RLg=230,RHg=255,GLg=230,GHg=254,BLg=42,BHg=255;
	int RLf=231,RHf=255,GLf=0,GHf=169,BLf=0,BHf=243;
	int resolutionX=800,resolutionY=448;
	float virtualWidth=0,virtualHeight=0;
	float theta=0;
	int modifierOn=0,lastModifier=0;
}


class POINTEDGE
{
public:
	int x;
	int y;
	int flag;
	long long int dist(int isTotalDist)
	{
		return (isTotalDist*x*x+y*y);
	}
	void clear()
	{
		x=0;
		y=0;
		flag=0;
	}
};


class MAPPING
{
public:
	char key[20],keycode[6];
	float posx,posy,limX,limY;
	int modifier;
	WORD keyCode()
	{return (WORD)strtoul(keycode, NULL, 0);}
	float posX()
	{return (posx/100)*virtualWidth;}
	float posY()
	{return (posy/100)*virtualHeight;}
};

	POINTEDGE cornerPoints[4],checkTops[4],temp;
	MAPPING keyInfo[maxNoOfKeys];

	/*
	0 -> TopLeft
	1 -> BottomLeft
	2 -> TopRight
	3 -> BottomRight
	*/

POINT rotate_point(float cx,float cy,float angle,POINT p)
{
	float s = sin(angle);
	float c = cos(angle);

	// translate point back to origin:
	p.x -= cx;
	p.y -= cy;

	// rotate point
	float xnew = p.x * c + p.y * s;
	float ynew = -p.x * s + p.y * c;

	// store point:
	p.x = xnew;
	p.y = ynew;

	return p;
}

void readMapping()
{
	ifstream inFile("mapping.txt");
	if (!inFile)
	{
		cerr << "Mapping File not found!" << endl;
	}
 
  string line;
  int i=0;
  while (getline(inFile, line))
  {
	if (!i) {i=1;continue;}
    istringstream iss(line);
    iss >> keyInfo[i].key;
	iss >> keyInfo[i].keycode;
	iss >> keyInfo[i].modifier;
	iss >> keyInfo[i].posx;
	iss >> keyInfo[i].posy;
	iss >> keyInfo[i].limX;
	iss >> keyInfo[i].limY;
    i++;
  }

  inFile.close();
}

void turnOffModifiers()
{
	INPUT ip;
	ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;
    ip.ki.wVk = 0x10; 
	ip.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &ip, sizeof(INPUT));
	ip.ki.wVk = 0x11; 
	ip.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &ip, sizeof(INPUT));
	ip.ki.wVk = 0x12; 
	ip.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &ip, sizeof(INPUT));
}

void sendKeypress (WORD kCode, int modifier)
{  
	INPUT ip;
	ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0; // hardware scan code for key
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;
 
    // Press the key
    ip.ki.wVk = kCode; // virtual-key code
    ip.ki.dwFlags = 0; // 0 for key press
    SendInput(1, &ip, sizeof(INPUT));
	Sleep(2);
	if (!modifier)
	{
		ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
		SendInput(1, &ip, sizeof(INPUT));
		if(lastModifier)
			turnOffModifiers();
	}
}


void checkKey(float disth, float distv)
{
	for(int i=1;i<maxNoOfKeys;i++)
	{
		if((disth>=keyInfo[i].posX()-keyInfo[i].limX)&&(disth<=keyInfo[i].posX()+keyInfo[i].limX)&&(distv>=keyInfo[i].posY()-keyInfo[i].limY)&&(distv<=keyInfo[i].posY()+keyInfo[i].limY))
		{
			sendKeypress(keyInfo[i].keyCode(),keyInfo[i].modifier);
			cout<< keyInfo[i].key<<" key pressed!!!!"<<endl;
			lastModifier=keyInfo[i].modifier;
			break;
		}
	}

}

void thresFunction(int x,void* data)
{
	*(int*)data = x;
}

void createBars()
{
	createTrackbar("RHg","Ref_Thres",&RHg,255,thresFunction,&RHg);
	createTrackbar("RLg","Ref_Thres",&RLg,255,thresFunction,&RLg);
	createTrackbar("GHg","Ref_Thres",&GHg,255,thresFunction,&GHg);
	createTrackbar("GLg","Ref_Thres",&GLg,255,thresFunction,&GLg);
	createTrackbar("BHg","Ref_Thres",&BHg,255,thresFunction,&BHg);
	createTrackbar("BLg","Ref_Thres",&BLg,255,thresFunction,&BLg);

	createTrackbar("RHf","Finger_Thres",&RHf,255,thresFunction,&RHf);
	createTrackbar("RLf","Finger_Thres",&RLf,255,thresFunction,&RLf);
	createTrackbar("GHf","Finger_Thres",&GHf,255,thresFunction,&GHf);
	createTrackbar("GLf","Finger_Thres",&GLf,255,thresFunction,&GLf);
	createTrackbar("BHf","Finger_Thres",&BHf,255,thresFunction,&BHf);
	createTrackbar("BLf","Finger_Thres",&BLf,255,thresFunction,&BLf);

	createTrackbar("FirstTime","Options",&firstTime,1,thresFunction,&firstTime);
	createTrackbar("KeySpeed","Options",&ITER,30,thresFunction,&ITER);
}

/*
void detectEdges(float x,float y,int fingNo)
{
	float ah,bh,ch,av,bv,cv,disth,distv;

	ah=cornerPoints[1].x-cornerPoints[0].x;
	bh=cornerPoints[0].y-cornerPoints[1].y;
	ch=(((-bh)*(cornerPoints[0].x))-((ah)*(cornerPoints[0].y)));
	disth=(abs(ah*x+bh*y+ch))/(sqrt(ah*ah+bh*bh));

	av=cornerPoints[2].x-cornerPoints[0].x;
	bv=cornerPoints[0].y-cornerPoints[2].y;
	cv=(((-bv)*(cornerPoints[0].x))-((av)*(cornerPoints[0].y)));
	distv=(abs(av*x+bv*y+cv))/(sqrt(av*av+bv*bv));

	checkKey(disth,distv);

}
*/

void checkCornerPoints(POINTEDGE* p,int max,int isTotalDist)
{
	POINTEDGE temp;
	for (int c = 0 ; c < ( max - 1 ); c++)
	{
    for (int d = 0 ; d < max - c - 1; d++)
		{
		  if (p[d].dist(isTotalDist) > p[d+1].dist(isTotalDist))
		  {
		    temp   = p[d];
		    p[d]   = p[d+1];
		    p[d+1] = temp;
		  }
		}
	}
}

int main(int argc,char** argv)
{
	cout<<"Setting Variables..."<<endl;
	int i=0;
	cout<<"Done"<<endl;

	cout<<"Reading Camera..."<<endl;
	
	VideoCapture cam(0);
	CV_Assert(cam.isOpened());

	cam.set(CV_CAP_PROP_FRAME_WIDTH,resolutionX);
	cam.set(CV_CAP_PROP_FRAME_HEIGHT,resolutionY);

	do
	{
		cam >> frame;
	}while(frame.empty());

	cout<<"Done"<<endl;

	cout<<"Creating Windows..."<<endl;
	namedWindow("Keyboard");
	namedWindow("Ref_Thres");
	namedWindow("Finger_Thres");
	namedWindow("Options");
	namedWindow("Keyboard_Adjust");
	cout<<"Done"<<endl;

	cout<<"Creating Trackbars..."<<endl;
	createBars();
	cout<<"Done"<<endl;

	cout<<"Reading Mapping..."<<endl;
	readMapping();
	cout<<"Done"<<endl;

	cout<<"Starting Camera Feed..."<<endl;

	while(true)
	{
		cam >> frame;
		GaussianBlur(frame,frame,Size(5,5),1);
		if(firstTime)
		{
			iterations=0;
			inRange(frame,Scalar(CV_RGB(RLg,GLg,BLg)),Scalar(CV_RGB(RHg,GHg,BHg)),frameBorder);
			morphologyEx(frameBorder,frameBorder,MORPH_CLOSE,getStructuringElement(MORPH_ELLIPSE,Size(5,5)));
			dilate(frameBorder,frameBorder,getStructuringElement(MORPH_ELLIPSE,Size(5,5)));
			findContours(frameBorder,contsBorder,hierarchy,CV_RETR_EXTERNAL ,CV_CHAIN_APPROX_NONE );

			if(contsBorder.size()!=4)
			{	
				imshow("Keyboard_Adjust",frameBorder);
				waitKey(10);
				cout << "ALign Keyboard " << contsBorder.size() <<  endl;
				continue;
			}
			
			cout<< "Keyboard OK " << contsBorder.size() << endl;
	
			for (i=0;i<contsBorder.size();i++)
			{
				drawContours( frameBorder, contsBorder,i, Scalar(255), 2, 8, hierarchy,0, Point() );
				rec = boundingRect(contsBorder[i]);
				rectangle(frameBorder,rec,Scalar(255));
				cornerPoints[i].x = rec.x;
				cornerPoints[i].y = rec.y;
			}

			checkCornerPoints(cornerPoints,contsBorder.size(),1);
			virtualWidth=sqrt(((cornerPoints[0].x-cornerPoints[2].x)*(cornerPoints[0].x-cornerPoints[2].x))+((cornerPoints[0].y-cornerPoints[2].y)*(cornerPoints[0].y-cornerPoints[2].y)));
			virtualHeight=sqrt(((cornerPoints[0].x-cornerPoints[1].x)*(cornerPoints[0].x-cornerPoints[1].x))+((cornerPoints[0].y-cornerPoints[1].y)*(cornerPoints[0].y-cornerPoints[1].y)));
			theta=atan((float)(cornerPoints[2].y-cornerPoints[0].y)/(cornerPoints[2].x-cornerPoints[0].x));

			imshow("Keyboard_Adjust",frameBorder);
			waitKey(10);
			continue;
		}

		//////////////////////edit start

		inRange(frame,Scalar(CV_RGB(RLg,GLg,BLg)),Scalar(CV_RGB(RHg,GHg,BHg)),frameBorder);
		morphologyEx(frameBorder,frameBorder,MORPH_CLOSE,getStructuringElement(MORPH_ELLIPSE,Size(5,5)));
		dilate(frameBorder,frameBorder,getStructuringElement(MORPH_ELLIPSE,Size(5,5)));
		findContours(frameBorder,contsBorder,hierarchy,CV_RETR_EXTERNAL ,CV_CHAIN_APPROX_NONE );

		
			if(contsBorder.size()>4||contsBorder.size()<2)
			{	
				iterations=0;
				imshow("Keyboard_Adjust",frameBorder);
				waitKey(10);
				cout << "Re-Align Keyboard " << contsBorder.size() <<  endl;
				continue;
			}
			
			imshow("Keyboard_Adjust",frameBorder);
			waitKey(10);
			cout<< "Keyboard OK " << contsBorder.size() << endl;

			for(i=0;i<4;i++)
				checkTops[i].clear();
			
			for (i=0;i<contsBorder.size();i++)
			{
				drawContours( frameBorder, contsBorder,i, Scalar(255), 2, 8, hierarchy,0, Point() );
				rec = boundingRect(contsBorder[i]);
				rectangle(frameBorder,rec,Scalar(255));
				checkTops[i].x = rec.x;
				checkTops[i].y = rec.y;
				checkTops[i].flag = 1;
			}

			checkCornerPoints(checkTops,contsBorder.size(),0);

			for(i=0;i<4;i++)
				{
					if(checkTops[i].flag==0) continue;
					cornerPoints[0].x=checkTops[i].x;
					cornerPoints[0].y=checkTops[i].y;
					cornerPoints[2].x=checkTops[i+1].x;
					cornerPoints[2].y=checkTops[i+1].y;
					break;
				}
			if(cornerPoints[0].dist(1)>cornerPoints[2].dist(1))
			{
				temp=cornerPoints[0];
				cornerPoints[0]=cornerPoints[2];
				cornerPoints[2]=temp;
			}
	
		theta=atan((float)(cornerPoints[2].y-cornerPoints[0].y)/(cornerPoints[2].x-cornerPoints[0].x));
		//////////////////////edit stop



		inRange(frame,Scalar(CV_RGB(RLf,GLf,BLf)),Scalar(CV_RGB(RHf,GHf,BHf)),frameFingers);
		morphologyEx(frameFingers,frameFingers,MORPH_CLOSE,getStructuringElement(MORPH_ELLIPSE,Size(5,5)));
		dilate(frameFingers,frameFingers,getStructuringElement(MORPH_ELLIPSE,Size(5,5)));
		findContours(frameFingers,contsFinger,hierarchy,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_NONE );

		//draw key positions
		for(i=1;i<maxNoOfKeys;i++)
		{
			tempRec.x=keyInfo[i].posX()-keyInfo[i].limX;
			tempRec.y=keyInfo[i].posY()-keyInfo[i].limY;
			tempRec.height=2*keyInfo[i].limY;
			tempRec.width=2*keyInfo[i].limX;
			rectangle(frameFingers,tempRec,Scalar(255));
		}
			

		for (i=0;i<contsFinger.size();i++)
		{
			rec = boundingRect(contsFinger[i]);
			rectangle(frameFingers,rec,Scalar(255));
			tempPoint.x=rec.x+rec.width/2.0;
			tempPoint.y=rec.y+rec.height/2.0;
			tempPoint=rotate_point(cornerPoints[0].x,cornerPoints[0].y,theta,tempPoint);
			circle(frameFingers,Point2i(tempPoint.x,tempPoint.y),5,Scalar(255),15);
			if(iterations==ITER)
			{
				cout<< "Finger Pos "<<i<<": "<<tempPoint.x << " " <<tempPoint.y<<endl;
				checkKey(tempPoint.x,tempPoint.y);
			}
				
		}

		if(iterations==ITER)
				iterations=0;
		iterations++;

		imshow("Keyboard",frameFingers);
		waitKey(10);
	}

return 0;
}