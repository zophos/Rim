#include <windows.h>
#include <GL/gl.h>
#include <glut.h>
#include <shmemutex-dynamic.h>
#include "getopt.h"


typedef struct{
    long draw;
    float viewing_angle;
    float focal_point[3];
    float fixation_point[3];
} GlViewDrawingPrams;

class GlViewEnv{
public:
    GLubyte *bits;
    GLuint width;
    GLuint height;
    size_t buf_size;
    ShMemutexHandle shm;
    ShMemutexHandle cmd;
    char common_name[MAX_PATH];
    GlViewDrawingPrams drawing_params;

    GlViewEnv()
        :width(640),
         height(480),
         buf_size(0),
         shm(NULL),
         cmd(NULL),
         bits(NULL)
    {
        strncpy(this->common_name,"Glview",MAX_PATH);
        this->drawing_params.draw=0;
    }

    ~GlViewEnv()
    {
        if(this->cmd){
            shmemutex_close(this->cmd);
            this->cmd=NULL;
        }
        
        if(this->shm){
            shmemutex_close(this->shm);
            this->shm=NULL;
        }

        if(this->bits){
            delete [] this->bits;
            this->bits=NULL;
        }
    }

    GlViewEnv *initialize()
    {
        char cmd_name[MAX_PATH];
        strncpy(cmd_name,this->common_name,MAX_PATH);
        strncat(cmd_name,"_cmd",MAX_PATH);

        this->buf_size=this->width*this->height*3;
        this->bits=new GLubyte[this->buf_size];
        this->shm=shmemutex_open(this->common_name,this->buf_size,0);
        this->cmd=shmemutex_open(cmd_name,1024,0);

        return this;
    }
};
GlViewEnv env;


GLfloat LIGHT_POSITION[]={0.25,1.0,0.25,0.0};
GLfloat LIGHT_DIFFUSE[]={1.0,1.0,1.0};
GLfloat LIGHT_AMBIENT[]={0.25,0.25,0.25};
GLfloat LIGHT_SPECULAR[]={1.0,1.0,1.0};

GLfloat MAT_DIFFUSE[]={1.0,0.0,0.0};
GLfloat MAT_AMBIENT[]={0.25,0.25,0.25};
GLfloat MAT_SPECULAR[]={1.0,1.0,1.0};
GLfloat MAT_SHININESS[]={32.0};


void drawxyzaxis(double w,int col)
{
    glBegin(GL_LINES);
    glColor3ub(col,0,0);
    glVertex3d(-w,  0, 0);
    glVertex3d( w,  0, 0);
    glColor3ub(0,col,0);
    glVertex3d( 0, -w, 0);
    glVertex3d( 0,  w, 0);
    glColor3ub(0,0,col);
    glVertex3d( 0, 0, -w);
    glVertex3d( 0, 0,  w);
    glEnd();
}


void on_display(void)
{
    GLint oldMatrixMode;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glGetIntegerv(GL_MATRIX_MODE, &oldMatrixMode);

    ////////////////////////////////////////////////////////////////////
    //
    // draw background bitmap
    //
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    glViewport(0,0,env.width,env.height);
    gluOrtho2D(0.0,
               (double)(env.width-1),
               0.0,
               (double)(env.height-1));

    //
    // flip bitmap image
    //
    glRasterPos2i(0,env.height-1);
    glPixelZoom(1.0,-1.0);

    glDrawPixels(env.width,
                 env.height,
                 GL_BGR_EXT,
                 GL_UNSIGNED_BYTE,
                 env.bits);
    
    glPopMatrix();

    glClear(GL_DEPTH_BUFFER_BIT);
    //
    //
    //


    ////////////////////////////////////////////////////////////////////
    //
    // draw tea pod
    //
    if(env.drawing_params.draw){

        glPushMatrix();
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        
        glMaterialfv(GL_FRONT,GL_DIFFUSE,MAT_DIFFUSE);
        glMaterialfv(GL_FRONT,GL_AMBIENT,MAT_AMBIENT);
        glMaterialfv(GL_FRONT,GL_SPECULAR,MAT_SPECULAR);
        glMaterialfv(GL_FRONT,GL_SHININESS,MAT_SHININESS);
        
        
        glViewport(0,0,env.width,env.height);
        gluPerspective(env.drawing_params.viewing_angle,
                       640.0/480.0,
                       200.0,600.0);
        gluLookAt(env.drawing_params.focal_point[0],
                  env.drawing_params.focal_point[1],
                  env.drawing_params.focal_point[2],
                  env.drawing_params.fixation_point[0],
                  env.drawing_params.fixation_point[1],
                  env.drawing_params.fixation_point[2],
                  0,1,0);
        
        //drawxyzaxis(120.0,.75);
        glutSolidTeapot(20.0);
        
        glPopMatrix();
    }
    //
    //
    //

    glMatrixMode(oldMatrixMode);

    glClear(GL_DEPTH_BUFFER_BIT);

    glutSwapBuffers();
}

void on_key(unsigned char key,int x,int y)
{
    if(key==27){
        env.~GlViewEnv();
        exit(0);
    }
    else
        glutPostRedisplay();
}

void on_idle(void)
{
    shmemutex_read(env.shm,
                   env.bits,
                   env.buf_size,
                   -1);
    shmemutex_read(env.cmd,
                   &env.drawing_params,
                   sizeof(GlViewDrawingPrams),
                   -1);

    glutPostRedisplay();
}


void usage(char *program_name)
{
    // Fixme
}

int main(int argc, char **argv)
{
    int ret;

    glutInit(&argc, argv);

    while((ret=getopt(argc,argv,"w:h:n:"))!=-1){
        switch(ret){
        case 'w':
            env.width=atoi(optarg);
            if(env.width<=0){
                usage(argv[0]);
                exit(-1);
            }
            break;
        case 'h':
            env.height=atoi(optarg);
            if(env.height<=0){
                usage(argv[0]);
                exit(-1);
            }
            break;
        case 'n':
            strncpy(env.common_name,optarg,MAX_PATH);
            if(env.common_name[0]=='0'){
                usage(argv[0]);
                exit(-1);
            }
            break;
        default:
            usage(argv[0]);
            exit(-1);
        }
    }

    env.initialize();
    
    glutInitWindowSize(env.width,env.height);
    glutInitDisplayMode(GLUT_DOUBLE |
                        GLUT_RGBA |
                        GLUT_DEPTH);
    
    glutCreateWindow(argv[0]);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glFrontFace(GL_CW);
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glShadeModel(GL_SMOOTH);

    glutDisplayFunc(on_display);
    glutKeyboardFunc(on_key);
    glutIdleFunc(on_idle);
    
    glutMainLoop();

    return 0;
}
