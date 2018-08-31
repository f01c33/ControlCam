#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <map>

using namespace std;
using namespace cv;

std::queue<Mat> buffer;
std::mutex mtxCam;
std::atomic<bool> grabOn; //this is lock free

void GrabThread(VideoCapture *cap)
{
    Mat tmp;

    //To know how many memory blocks will be allocated to store frames in the queue.
    //Even if you grab N frames and create N x Mat in the queue
    //only few real memory blocks will be allocated
    //thanks to std::queue and cv::Mat memory recycling
    std::map<unsigned char*, int> matMemoryCounter;
    uchar * frameMemoryAddr;

    while (grabOn.load() == true) //this is lock free
    {
        //grab will wait for cam FPS
        //keep grab out of lock so that 
        //idle time can be used by other threads
        *cap >> tmp; //this will wait for cam FPS

        if (tmp.empty()) continue;

        //get lock only when we have a frame
        mtxCam.lock();
        //buffer.push(tmp) stores item by reference than avoid
        //this will create a new cv::Mat for each grab
        buffer.push(Mat(tmp.size(), tmp.type()));
        tmp.copyTo(buffer.back());
        frameMemoryAddr = buffer.front().data;
        mtxCam.unlock();
        //count how many times this memory block has been used
        matMemoryCounter[frameMemoryAddr]++; 

        bool show = true;
        if (show)
        {
            int font = CV_FONT_HERSHEY_PLAIN;
            putText(tmp, "THREAD FRAME", Point(10, 10), font, 1, Scalar(0, 255, 0));
            imshow("Image thread", tmp);
            waitKey(1);    //just for imshow
        }
    }
    std::cout << std::endl << "Number of Mat in memory: " << matMemoryCounter.size();
}

void ProcessFrame(const Mat &src)
{
    if(src.empty()) return;
    putText(src, "PROC FRAME", Point(10, 10), CV_FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 0));
    imshow("Image main", src);
}
int main() {
    Mat frame;
    VideoCapture cap;
    cap.open(0);
    if (!cap.isOpened()) //check if we succeeded
        return -1;

    grabOn.store(true);                //set the grabbing control variable
    thread t(GrabThread, &cap);          //start the grabbing thread
    int bufSize;
    while (true)
    {
        mtxCam.lock();                //lock memory for exclusive access
        bufSize = buffer.size();      //check how many frames are waiting 
        if (bufSize > 0)              //if some 
        {
            //reference to buffer.front() should be valid after 
            //pop because of Mat memory reference counting
            //but if content can change after unlock is better to keep a copy
            //an alternative is to unlock after processing (this will lock grabbing)
            buffer.front().copyTo(frame);   //get the oldest grabbed frame (queue=FIFO)
            buffer.pop();            //release the queue item
        }
        mtxCam.unlock();            //unlock the memory

        if (bufSize > 0)            //if a new frame is available
        {
            ProcessFrame(frame);    //process it
            bufSize--;
        }

        //if bufSize is increasing means that process time is too slow regards to grab time 
        //may be you will have out of memory soon
        cout << endl << "frame to process:" << bufSize;

        if (waitKey(33) >= 0)        //press any key to terminate
        {
            grabOn.store(false);    //stop the grab loop 
            t.join();               //wait for the grab loop

            cout << endl << "Flushing buffer of:" << bufSize << " frames...";
            while (!buffer.empty())    //flushing the buffer
            {
                frame = buffer.front();    
                ProcessFrame(frame);
                buffer.pop();
            }
            cout << "done"<<endl;
            break; //exit from process loop
        }
    }
    cout << endl << "Press Enter to terminate"; cin.get();
    return 0;
}