#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include <iostream>
#include <stdio.h>

using namespace std;
using namespace cv;

int main(){
	Mat frame;
	char comando1[1<<8];
	char comando2[1<<8];
	int c,cont=42;
	while(1){
		// curl 'http://192.168.1.100/snapshot.cgi?user=&pwd=' -H 'Authorization: Basic dXRmcHI6dXRmcHI=' -H 'Accept-Encoding: gzip, deflate' -H 'Accept-Language: en-US,en;q=0.8' -H 'Upgrade-Insecure-Requests: 1' -H 'User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/59.0.3071.115 Safari/537.36' -H 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8' -H 'Referer: http://192.168.1.100/snap.htm' -H 'Connection: keep-alive' --compressed
		// system("curl 'http://192.168.1.100/snapshot.cgi?user=utfpr&pwd=utfpr' -H 'Authorization: Basic dXRmcHI6dXRmcHI=' -H 'Referer: http://192.168.1.100/snap.htm' -H 'Connection: keep-alive' --compressed -s > tmp1.png");
		system("curl 'http://192.168.1.100/snapshot.cgi?user=utfpr&pwd=utfpr' -H 'Authorization: Basic dXRmcHI6dXRmcHI=' -H 'Referer: http://192.168.1.100/snap.htm' -H 'Connection: keep-alive' --compressed -s > tmp2.png");
		// printf("./ctrlcam cam2 -cmd=[n:screenshot,a:0,f:0] > tmp.png\n");
		// frame[0] = imread("tmp1.png",CV_LOAD_IMAGE_COLOR);
		frame = imread("tmp2.png",CV_LOAD_IMAGE_COLOR);
	    // imshow("c1",frame[0]);
	    imshow("c2",frame);
	    // moveWindow("c1",0,0);
	    // moveWindow("c2",640,0);
	    c = waitKey(500);
	    /*if(c == 'w'){
			system("./ctrlcam cam1 -cmd=[n:up]");
			system("./ctrlcam cam2 -cmd=[n:up]");
        }else if(c == 's'){
			system("./ctrlcam cam1 -cmd=[n:down]");
			system("./ctrlcam cam2 -cmd=[n:down]");
        }else if(c == 'a'){
			system("./ctrlcam cam1 -cmd=[n:left]");
			system("./ctrlcam cam2 -cmd=[n:left]");
        }else if(c == 'd'){
			system("./ctrlcam cam1 -cmd=[n:right]");
			system("./ctrlcam cam2 -cmd=[n:right]");
        }else*/ if(c == 'e'){
        	// sprintf(comando1,"mv tmp1.png c1_%i.png \0",cont);
        	// printf("%s\n",comando1);
        	sprintf(comando2,"mv tmp2.png c2_%i.png \0",cont);
        	// system(comando1);
        	system(comando2);
        	// printf("%s\n",comando2);
            // limiar2+=5;
            cont++;
        }else if(c == 27){
            break;
        }else{
        	continue;
        }
        // fflush(stdout);
	}
	return 0;
}