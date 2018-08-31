#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include <iostream>
#include <stdio.h>
#include <math.h>

using namespace std;
using namespace cv;

String window_name = "follow_thing";

Point detect(String filename){
    Mat frame = imread(filename,CV_LOAD_IMAGE_COLOR);
    Mat tshld;
    
    int  lowr=0  , lowg=0  , lowb=75,
    highr=75 ,highg=255,highb=185,c;

    // threshold
    inRange(frame,Scalar(lowb,lowg,lowr),Scalar(highb,highg,highr),tshld);
    int w = frame.size().width, h = frame.size().height;
    long long int cont_hit = 0,col_sum=0,line_sum=0;

    // calcula centróide do objeto detectado
    for(int i = 0; i < h; i++){
        for(int j = 0; j < w; j++){
            if(mean(tshld(Rect(j,i,1,1)))[0] == 255){
                cont_hit+=1;
                col_sum+=j;
                line_sum+=i;
            }
        }
    }

    // imshow(window_name,frame);
    // waitKey(0);
    // imshow(window_name,frame);
    // waitKey(0);
    // imshow(window_name,tshld);
    // waitKey(0);
    // imshow(window_name,tshld);
    // waitKey(0);

    return Point(col_sum/cont_hit,line_sum/cont_hit);
}

int main(int argc, const char** argv){
    while(1){
        system("curl 'http://192.168.1.100/snapshot.cgi?user=utfpr&pwd=utfpr' -H 'Authorization: Basic dXRmcHI6dXRmcHI=' -H 'Referer: http://192.168.1.100/snap.htm' -H 'Connection: keep-alive' --compressed -s > tmp1.png");
        // system("./ctrlcam cam1 -cmd=[n:snapshot] > tmp1.png");
        Point detected = detect("tmp1.png");
        Point center(640/2.0,480/2.0);

        double angle = -atan2(detected.y-center.y,detected.x-center.x) * 180 / M_PI;
        // angle+=180;
        double distance = pow(pow(detected.y-center.y,2)+pow(detected.x-center.x,2),0.5);
        double factor = 5e-4; // how much the camera will move
        char command[1<<8];
        cout << angle << ", " << distance << endl;

        Mat frame = imread("tmp1.png",CV_LOAD_IMAGE_COLOR);
        circle(frame,center,10,Scalar(200,0,0),1);
        circle(frame,detected,10,Scalar(0,0,200),2);
        // cout << detected;
        imshow("frame",frame);
        moveWindow("frame",0,0);
        char c = waitKey(100);

        // moveWindow("c2",640,0);
        
       //    90º +- 22.5 up
       //    - 
       //  /   \ 45º+-22.5 upright
       // |  o  | 0º +-22.5 right
       //  \   / etc
       //    - 
        
        // #define tau (2*M_PI)
        if(distance > 10){
            if(angle > 0-22.5 && angle <= 22.5){
                sprintf(command,"./ctrlcam cam1 -cmd=[n:right,a:0,f:%F]",factor*distance);
                //right
            }else if(angle > 22.5 && angle <= 90.0-22.5){
                sprintf(command,"./ctrlcam cam1 -cmd=[n:upright,a:0,f:%F]",factor*distance);
                //upright
            }else if(angle > 90.0-22.5 && angle <= 90.0+22.5){
                sprintf(command,"./ctrlcam cam1 -cmd=[n:up,a:0,f:%F]",factor*distance);
                //up
            }else if(angle > 90.0+22.5 && angle <= 180-22.5){
                sprintf(command,"./ctrlcam cam1 -cmd=[n:upleft,a:0,f:%F]",factor*distance);
                //upleft
            }else if(angle > 180.0-22.5 || angle <= -180+22.5){
                sprintf(command,"./ctrlcam cam1 -cmd=[n:left,a:0,f:%F]",factor*distance);
                //left
            }else if(angle > -180+22.5 && angle <= -90-22.5){
                sprintf(command,"./ctrlcam cam1 -cmd=[n:downleft,a:0,f:%F]",factor*distance);
                //downleft
            }else if(angle > -90.0-22.5 && angle <= -90+22.5){
                sprintf(command,"./ctrlcam cam1 -cmd=[n:down,a:0,f:%F]",factor*distance);
                //down
            }else if(angle > -45-22.5 && angle <= -45.0+22.5){
                sprintf(command,"./ctrlcam cam1 -cmd=[n:downright,a:0,f:%F]",factor*distance);
                //downright
            }
            system(command);
        }
    }
    return 0;
}
