#include <cv.h>
#include <highgui.h>
#include <string.h>
#include <shmemutex-dynamic.h>
#include "getopt.h"

#pragma comment(lib,"cv200d.lib")
#pragma comment(lib,"cxcore200d.lib")
#pragma comment(lib,"highgui200d.lib")
#pragma comment(lib,"shmemutex.lib")

void usage(char *program_name)
{
    // Fixme
}

int main(int argc,char *argv[])
{
    char common_name[MAX_PATH]="RimShot";
    int width=640,height=480,ret;

    while((ret=getopt(argc,argv,"w:h:n:"))!=-1){
        switch(ret){
        case 'w':
            width=atoi(optarg);
            if(width<=0){
                usage(argv[0]);
                exit(-1);
            }
            break;
        case 'h':
            height=atoi(optarg);
            if(height<=0){
                usage(argv[0]);
                exit(-1);
            }
            break;
        case 'n':
            strncpy(common_name,optarg,MAX_PATH);
            if(common_name[0]=='0'){
                usage(argv[0]);
                exit(-1);
            }
            break;
        default:
            usage(argv[0]);
            exit(-1);
        }
    }

    IplImage *image=cvCreateImage(cvSize(width,height),8,3);

    size_t lineWidth=width*3;
    size_t readSz=lineWidth*height;

    ShMemutexHandle shm=shmemutex_open(common_name,readSz,FALSE);

    cvNamedWindow(common_name,CV_WINDOW_AUTOSIZE);
	
    while(1){
        if(image->widthStep==lineWidth)
            shmemutex_read(shm,image->imageData,image->imageSize,-1);
        else{
            int i;
            char *src,*dst;
            
            shmemutex_lock(shm,-1);
            for(i=0,src=(char *)shmemutex_map_addr(shm),dst=image->imageData;
                i<image->height;
                i++,src+=lineWidth,dst+=image->widthStep)
                memcpy(dst,src,lineWidth);
            shmemutex_unlock(shm);
        }

        int c=cvWaitKey(10);
        if(c==27)
            break;

        cvShowImage(common_name,image);
    }

    shmemutex_close(shm);

    cvDestroyWindow(common_name);
    cvReleaseImage(&image);
}
