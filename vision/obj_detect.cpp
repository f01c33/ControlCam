#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include <opencv2/dnn.hpp>
#include <opencv2/dnn/shape_utils.hpp>

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <math.h>
#include <cmath>
#include <random>

#include <sys/time.h>
#include <unistd.h>

#define CVUI_IMPLEMENTATION
#include "cvui.h"

using namespace std;
using namespace cv;
using namespace cv::dnn;

#include "qsort.h"

String window_name = "follow_thing";

Point correct = Point(640.0/2.0,480.0/2.0); //will be the center of the user-selected box
Mat tshld,frame,UI;

CascadeClassifier face_cascade;
Point detect_haar() {
//    Detect faces
    std::vector<Rect> faces;
    face_cascade.detectMultiScale(frame, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) );
    if(faces.size() == 0) return Point(640.0/2.0,480.0/2.0);
    return Point(faces[0].x + faces[0].width*0.5, faces[0].y + faces[0].height*0.5);
}

long long int cont_hit;
Point detect(mask r){
    //    Mat frame = imread(filename,CV_LOAD_IMAGE_COLOR);
    
    /* int lowr=0  , lowg=0  , lowb=75,
    highr=75 ,highg=255,highb=185,; */

    // threshold
    inRange(frame,r.low,r.high,tshld);
    int w = frame.size().width, h = frame.size().height;
    long long int col_sum=0,line_sum=0;
    cont_hit = 0;
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

    if(cont_hit == 0){
        return Point(0,0);
    }
    return Point(col_sum/cont_hit,line_sum/cont_hit);
}

dnn::Net net;
vector<String> classNamesVec;
Point detect_yolo(float confidenceThreshold, string follow_class){
    //! [Prepare blob]
    Mat inputBlob = blobFromImage(frame, 1 / 255.F, Size(416, 416), Scalar(), true, false); //Convert Mat to batch of images
    //! [Prepare blob]

    //! [Set input blob]
    net.setInput(inputBlob, "data");                   //set the network input
    //! [Set input blob]

    //! [Make forward pass]
    Mat detectionMat = net.forward("detection_out");   //compute output
    //! [Make forward pass]

 //    vector<double> layersTimings;
 //    double tick_freq = getTickFrequency();
 //    double time_ms = net.getPerfProfile(layersTimings) / tick_freq * 1000;
 //    putText(frame, format("FPS: %.2f ; time: %.2f ms", 1000.f / time_ms, time_ms),
 //            Point(20, 20), 0, 0.5, Scalar(0, 0, 255));
    for (int i = 0; i < detectionMat.rows; i++){
        const int probability_index = 5;
        const int probability_size = detectionMat.cols - probability_index;
        float *prob_array_ptr = &detectionMat.at<float>(i, probability_index);

        size_t objectClass = max_element(prob_array_ptr, prob_array_ptr + probability_size) - prob_array_ptr;
        float confidence = detectionMat.at<float>(i, (int)objectClass + probability_index);

        if (confidence > confidenceThreshold){
            float x_center = detectionMat.at<float>(i, 0) * frame.cols;
            float y_center = detectionMat.at<float>(i, 1) * frame.rows;
            float width = detectionMat.at<float>(i, 2) * frame.cols;
            float height = detectionMat.at<float>(i, 3) * frame.rows;
            Point p1(cvRound(x_center - width / 2), cvRound(y_center - height / 2));
            Point p2(cvRound(x_center + width / 2), cvRound(y_center + height / 2));
            Rect object(p1, p2);

            Scalar object_roi_color(0, 255, 0);

 //            if (object_roi_style == "box")
 //            {
 //                rectangle(frame, object, object_roi_color);
 //            }
 //            else
 //            {
                Point p_center(cvRound(x_center), cvRound(y_center));
                line(UI, object.tl(), p_center, object_roi_color, 1);
 //            }

            String className = objectClass < classNamesVec.size() ? classNamesVec[objectClass] : cv::format("unknown(%d)", objectClass);
            String label = format("%s: %.2f", className.c_str(), confidence);
            int baseLine = 0;
            Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
            rectangle(UI, Rect(p1, Size(labelSize.width, labelSize.height + baseLine)),
                      object_roi_color, FILLED);
            putText(UI, label, p1 + Point(0, labelSize.height),
                    FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0,0,0));

            if(className == follow_class){
                return Point(x_center,y_center);
            }
        }
    }
    return Point(640/2,480/2);
}

std::random_device rd; // função random() de [0,1)
std::mt19937 don(rd());
std::uniform_real_distribution<> ran(0, 255);
std::uniform_real_distribution<> r01(0, 1);
///
/// Generates an individual on the memory postition specified
///
void gen_individual(individual* ind) {
    ind->r.low = Scalar(ran(don),ran(don),ran(don));
    ind->r.high = Scalar(ran(don),ran(don),ran(don));
}
///
/// Sets the value for fitness on the indivitual
///
void set_fitness(individual* b) {
    //  for(int i = 0; i < NAMBULANCIAS; i++){
    //    Ambulancia[i].FixarBase(b->amb[i].x,b->amb[i].y);
    //  }
    //  b->fitness = Simulacao();
    // std::cout << b->fitness << std::endl;

    //    cout << b->r.high << b->r.low << "//";
    //    cout.flush();

#define dist(k,l) pow(pow(k.x-l.x,2.0)+pow(k.y-l.y,2.0),0.5)
    Point tmp = detect(b->r);
    b->fitness = pow(dist(tmp,correct),2)/pow(cont_hit,0.33);
}

///
/// Prints the internal state of the population, as well as it's generation and size
///
void view_gen(individual* ind, int population_size, int gen_n) {
    std::cout << "\ngeneration #" << gen_n << std::endl;
    for (int i = 0; i < population_size; i++) {
        std::cout << i << ": " << ind[i].fitness << ", " << ind[i].r.high << ", " << ind[i].r.low;
        //    for(int j = 0; j < NAMBULANCIAS; j++){
        //      std::cout << "(" << j << ":"<< ind[i].amb[j].x << "," << ind[i].amb[j].y << ")";
        //    }
        std::cout << std::endl;
    }
}

///
/// When a population enters the reproductive phase of life the best rated individuals
/// mate with the mid-rated individuals, the worse individuals are killed and a new individual
/// takes his place, a ruthless society
///
void mate(individual* ind, int population_size) {
    int i = 0, j = 0, good_limit = ((int)population_size * 0.3);
    double rnd;
    for (i = good_limit; i < population_size - good_limit; i++) {
        rnd = r01(don);
        if (rnd > 0.6) {
            //      for(int k = 0; k < NAMBULANCIAS; k++){
            rnd = r01(don);
            ind[i].r.high = ind[i].r.high*rnd + ind[j].r.high*(1.0-rnd);
            //        ind[i].amb[k].x = ind[i].amb[k].x*rnd + ind[j].amb[k].x*(1.0-rnd);
            rnd = r01(don);
            ind[i].r.low = ind[i].r.low*rnd + ind[j].r.low*(1.0-rnd);
            //        ind[i].amb[k].y = ind[i].amb[k].y*rnd + ind[j].amb[k].y*(1.0-rnd);
            //      }
            j++;
            if (j > good_limit) {
                j = 0;
            }
        }
    }
    for (i = population_size - good_limit; i < population_size; i++) {
        gen_individual(&ind[i]);
    }
}

///
/// Run the genetic algorithm with a certain population size for given generations,
/// the generations are printed on the standard output as they happen.
///
struct mask gen_alg(int population_size, int generations) {
    //    individual indiv[population_size];
    individual* indiv = (individual*) malloc(sizeof(*indiv)*population_size);
    int gen_n = 0;
    for (int i = 0; i < population_size; i++) {
        gen_individual(&indiv[i]);
    }
    while (gen_n++ < generations) {
        for (int i = 0; i < population_size; i++) {
            //            cout << i << endl;
            //            cout.flush();
            set_fitness(&indiv[i]);
        }
        quick_sort(&indiv[0], population_size);
        // do the killing and the mating here
        mate(indiv, population_size);
        // if(!(gen_n%50)){
        //   view_gen(indiv, population_size, gen_n);
        // }
    }
    view_gen(indiv, population_size, gen_n);
    struct mask ret = indiv[0].r;
    free(indiv);
    return ret;
}

struct mask calibrate(){
 //    Mat frame;
    bool has_clicked;
    while(true){
 //        frame = imread("tmp1.png",CV_LOAD_IMAGE_COLOR);
        cvui::update();
        if (!has_clicked){
            cvui::text(frame, 10, 20, "click on the center of the object to begin calibration");
        }else{
            cvui::text(frame,10,20,"calculating stuff");
        }
        imshow(window_name,frame);
        if(waitKey(30) == 27){
            return (struct mask){};
        }
        if(has_clicked) break;
        if (cvui::mouse(cvui::LEFT_BUTTON, cvui::DOWN)) {
            //            std::cout << "Left mouse button just went down." << std::endl;
            correct = cvui::mouse();
            cout << correct;
            cout.flush();
            //            break;
            has_clicked = true;
        }
    }
    return gen_alg(20,30);
}

const String keys =
        "{help h usage ? |               | print this message }"
        "{c cam          | 0             | select camera to use }"
        "{f factor       | 10000         | select camera to use }"
        "{d debug        | true          | show window and options }"
        "{m method       | cen           | <cen>ter of thresholds (fast) <haar> cascade (medium) <yolo> neural net (slow)}"
        "{ctl tlow       | (124, 31, 166)      | optional, the R,G,B values for the low values of the threshold method}"
        "{cth thigh      | (173, 157, 168)  | optional, the R,G,B values for the high values of the threshold method}"
        "{hm haar_model  | haar_face.xml | optional, what cascade model to follow, default is faces}"
        "{ycfg            | darknet/cfg/yolo.cfg | darknet model configuration }"
        "{ymodel          | darknet/yolo.weights | darknet model weights }"
        "{ynames    | darknet/data/coco.names | darknet file with class names }"
        "{yconf min_confidence | 0.25 | darknet minimal threshold}"
        "{fliph | true | flip image horizontaly}"
        "{flipv | true | flip image verticaly}"
        ;//what else should this have?
          

struct timespec wt[] = {{0, 0}}; //a struct global que vai ser utilizada para dar a pausa

void std_sleep(int waitTime) { //usleep via nanosleep
    wt[0].tv_nsec = waitTime * 1000;
    nanosleep(wt, NULL);
}

Scalar static inline tup_to_scalar(string s){
    int v[3] = {0,0,0};
    int c1,c2;
    c1 = s.find(',');
    c2 = s.find_last_of(',');
    v[0] = stoi(s.substr(1,c1-1));
    v[1] = stoi(s.substr(c1+1,c2-c1-1));
    v[2] = stoi(s.substr(c2+1,s.length()-c2-2));
    return Scalar(v[0],v[1],v[2]);
}

// bool matIsEqual(const cv::Mat Mat1, const cv::Mat Mat2)
// {
//   if( Mat1.dims == Mat2.dims && 
//     Mat1.size == Mat2.size && 
//     Mat1.elemSize() == Mat2.elemSize())
//   {
//     if( Mat1.isContinuous() && Mat2.isContinuous())
//     {
//       return 0==memcmp( Mat1.ptr(), Mat2.ptr(), Mat1.total()*Mat1.elemSize());
//     }
//     else
//     {
//       const cv::Mat* arrays[] = {&Mat1, &Mat2, 0};
//       uchar* ptrs[2];
//       cv::NAryMatIterator it( arrays, ptrs, 2);
//       for(unsigned int p = 0; p < it.nplanes; p++, ++it)
//         if( 0!=memcmp( it.ptrs[0], it.ptrs[1], it.size*Mat1.elemSize()) )
//           return false;

//       return true;
//     }
//   }

//   return false;
// }

/*todo:
//    -camera as a parameter
//    -toggle following of object
//    -toggle viewing final processed image
//    -manual control of the camera
//    -calibration button, which asks for an object center and runs the genalg for a while to get good thresholds
//    -fix snapshot for all cameras
//    -small menu for chosing the detection method to follow
//    -centroid and threshold
//        -it's choices as comand line parameters
//    -face detection/other haar,
//        -it's choices as cli params
//    -use yolo/darknet,
//        -it's choices as cli params
    -use background detection to start object detection?

    -option to invert lateraly and verticaly, and change the commands accordingly
    -fix following
     -not lose current frames
      - separate capture into a thread 
      - get newest frame from thread
     -dynamicaly adapt camera speed?
      -detect left-right up-down time
      -use said speed to correctly send one or close to commands

*/

int main(int argc, const char* argv[]){
    //parse arguments and load configurations
    CommandLineParser parser(argc, argv, keys);
    parser.about("Object follower v0.0.1");

    if (parser.has("help") || !parser.has("cam")) {
        parser.printMessage();
        return 0;
    }
    if (!parser.check()){
        parser.printErrors();
        return 0;
    }
    String cam = parser.get<String>("cam");
    double factor = parser.get<double>("factor");
    bool debug = parser.get<bool>("debug");
    String method = parser.get<String>("method");
    String haar_mod = parser.get<String>("haar_model");
    String tlow = parser.get<String>("tlow");
    String thigh = parser.get<String>("thigh");
    bool flipv = parser.get<bool>("flipv");
    bool fliph = parser.get<bool>("fliph");
    int flipping;   
    if(fliph && flipv){
        flipping = -1;
    }else if(fliph){
        flipping = 0;
    }else if (flipv){
        flipping = 1;
    }else{
        flipping = 2;
    }
    float confidenceThreshold = parser.get<float>("min_confidence");

    struct mask m;
//    m.high = Scalar(75,255,185);
//    m.low = Scalar(0,0,75);

    if(method == "cen"){
        m.high = tup_to_scalar(thigh);
        m.low = tup_to_scalar(tlow);
    } else if (method == "haar"){
        face_cascade.load(haar_mod);
    } else if (method == "yolo"){ //snippets from github
        String modelConfiguration = parser.get<String>("ycfg");
        String modelBinary = parser.get<String>("ymodel");

        //! [Initialize network]
        /*dnn::Net */net = readNetFromDarknet(modelConfiguration, modelBinary);
        //! [Initialize network]
        if (net.empty()){
            cerr << "Can't load network by using the following files: " << endl;
            cerr << "cfg-file:     " << modelConfiguration << endl;
            cerr << "weights-file: " << modelBinary << endl;
            cerr << "Models can be downloaded here:" << endl;
            cerr << "https://pjreddie.com/darknet/yolo/" << endl;
            exit(-1);
        }
        ifstream classNamesFile(parser.get<String>("ynames").c_str());
        if (classNamesFile.is_open()){
            string className = "";
            while (getline(classNamesFile, className))
                classNamesVec.push_back(className);
        }

    }
    //open cam.json
    ifstream cfg("cameras/"+cam+".json");
    //get ip
    string line;
    size_t found;
    string ip;
    while(getline(cfg,line)){
        //        cout << line << endl;
        found = line.find("\"IP\":\"");
        if(found != string::npos){
            ip = /*cout << */line.substr(7,line.find("\",")-7); //len('"IP:"')
        }
        //        found = line.find("")
    }
    //    cout << debug << endl;
    string vid_stream;
    vid_stream = "http://"+ip+"/videostream.cgi?user=utfpr&pwd=utfpr";
    namedWindow(window_name);
    //    namedWindow("fds");
    cvui::init(window_name);
    //    cvui::init("fds");
//    face_cascade.load("haar_face.xml");
    //    String cam;
    bool follow = false;
    bool masked = false;
    VideoCapture cap;
    if(cam != "0"){
        cap = VideoCapture(vid_stream);
    }else{
        cap = VideoCapture(0);
    }
    int fps = 30;

    if(cap.get(CV_CAP_PROP_FPS) != 0){
        fps = cap.get(CV_CAP_PROP_FPS);
    }
    // Mat UI;
    int tick = 0;
    Point detected;
    while(true){
        if(!cap.grab()){
            cout << "wat1" << endl;
        }
        // if(!cap.grab()){
        //     cout << "wat2" << endl;
        // }
        // if(!cap.grab()){
        //     cout << "wat3" << endl;
        // }
        // if(!cap.grab()){
        //     cout << "wat4" << endl;
        // }
        if(!cap.retrieve(frame)){
            cout << "wat release" << endl;
        }
        
        if(flipping < 2){
            flip(frame,frame,flipping);
        }
        UI = frame.clone();
        // cap >> frame;
        
        if(tick == 0){
            if(method == "cen")
                detected = detect(m);
            else if (method == "haar")
                detected = detect_haar();
            else if (method == "yolo"){
                detected = detect_yolo(confidenceThreshold,"person");
                cout << detected << endl; cout.flush();
            }
        }
        //        Point detected = detect_face("tmp1.png");
        //* following the detection
        Point center(640/2.0,480/2.0);
        // calculate angle
        double angle = -atan2(detected.y-center.y,detected.x-center.x) * 180 / M_PI;
        // angle+=180;
        double distance = pow(pow(detected.y-center.y,2)+pow(detected.x-center.x,2),0.5)/3;
        //        double factor = 3e-4; // how much the camera will move, very experimental variable, maybe add it to the camera configuration
        char command[1<<8];
        command[0] = '\0';
        // cout << angle << ", " << distance << endl;

        if(masked){
            UI = tshld;
        }else{
            //            frame = imread("tmp1.png",CV_LOAD_IMAGE_COLOR);
        }
        // if(tick){    
            circle(UI,center,10,Scalar(200,0,0),1);
            circle(UI,detected,10,Scalar(100,0,100),1);
            // cout << detected;
        // }
        cvui::checkbox(UI, 10, 20, "follow", &follow);
        cvui::checkbox(UI, 10, 40, "view mask", &masked);
        if(cvui::button(UI,10, 60,"calibrate")){
            cout << "calibrate!";
            // cap >> UI;
            m = calibrate();
            //ask for input
            //run genalg
        }
        // cout << cap.get(CAP_PROP_POS_FRAMES) << ", " << cap.get(CAP_PROP_POS_MSEC) << endl;
        // cout.flush();
        cvui::update();
        //        imshow("fds",tshld);
        char c = '\0';
        //        if(debug){
        imshow(window_name,UI);
        c = waitKey(fps/10.0);
//                }else{
        //            std_sleep(30);
        //        }
        //            imshow(window_name,tshld);
        //            moveWindow(window_name,0,0);
        //        moveWindow("fds",640,0);
        
        //    90º +- 22.5 up
        //    -
        //  /   \ 45º+-22.5 upright
        // |  o  | 0º +-22.5 right
        //  \   / etc
        //    -
        double control_factor = factor*1e-4;
        switch(c){
        case 27:
        case 'q':
            exit(0);
        case 'w':
            sprintf(command,"./ctrlcam %s -cmd=[n:down,a:0,f:%F]",cam.c_str(),control_factor);
            break;
        case 's':
            sprintf(command,"./ctrlcam %s -cmd=[n:up,a:0,f:%F]",cam.c_str(),control_factor);
            break;
        case 'a':
            sprintf(command,"./ctrlcam %s -cmd=[n:right,a:0,f:%F]",cam.c_str(),control_factor);
            break;
        case 'd':
            sprintf(command,"./ctrlcam %s -cmd=[n:left,a:0,f:%F]",cam.c_str(),control_factor);
            break;
        default:
            //            cout << ".";
            break;
        }
#define MOV_FACT 3e-2
        if(follow && tick == 0){
            // tick = false;
            if(distance > 15){
                if(angle > 0-22.5 && angle <= 22.5){
                    sprintf(command,"./ctrlcam %s -cmd=[n:right,a:0,f:%F]",cam.c_str(),control_factor*distance*MOV_FACT);
                    //right
                }else if(angle > 22.5 && angle <= 90.0-22.5){
                    sprintf(command,"./ctrlcam %s -cmd=[n:upright,a:0,f:%F]",cam.c_str(),control_factor*distance*MOV_FACT);
                    //upright
                }else if(angle > 90.0-22.5 && angle <= 90.0+22.5){
                    sprintf(command,"./ctrlcam %s -cmd=[n:up,a:0,f:%F]",cam.c_str(),control_factor*distance*MOV_FACT);
                    //up
                }else if(angle > 90.0+22.5 && angle <= 180-22.5){
                    sprintf(command,"./ctrlcam %s -cmd=[n:upleft,a:0,f:%F]",cam.c_str(),control_factor*distance*MOV_FACT);
                    //upleft
                }else if(angle > 180.0-22.5 || angle <= -180+22.5){
                    sprintf(command,"./ctrlcam %s -cmd=[n:left,a:0,f:%F]",cam.c_str(),control_factor*distance*MOV_FACT);
                    //left
                }else if(angle > -180+22.5 && angle <= -90-22.5){
                    sprintf(command,"./ctrlcam %s -cmd=[n:downleft,a:0,f:%F]",cam.c_str(),control_factor*distance*MOV_FACT);
                    //downleft
                }else if(angle > -90.0-22.5 && angle <= -90+22.5){
                    sprintf(command,"./ctrlcam %s -cmd=[n:down,a:0,f:%F]",cam.c_str(),control_factor*distance*MOV_FACT);
                    //down
                }else if(angle > -45-22.5 && angle <= -45.0+22.5){
                    sprintf(command,"./ctrlcam %s -cmd=[n:downright,a:0,f:%F]",cam.c_str(),control_factor*distance*MOV_FACT);
                    //downright
                }
            }else{
                command[0]='\0'; command[1]='\0';
            }
        }
        if (command[0] != '\0'){
        system(command);
            cout << command;
            cout.flush();
            command[0] = '\0';
        }
        tick++;
        tick%=6;
    }
    cap.release();
    // following the detection */
    return 0;
}
