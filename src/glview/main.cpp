#include <windows.h>
#include <math.h>
#include <GL/gl.h>
#include <glut.h>
#include <shmemutex-dynamic.h>
#include "getopt.h"

GLfloat MAT_AMBIENT[]={0.45,0.45,0.45};
GLfloat MAT_SPECULAR[]={1.0,1.0,1.0};
GLfloat MAT_SHININESS[]={32.0};

GLfloat WATER_COLOR[]={0.3,1.0,1.0};
GLfloat WATER_AMBIENT[]={0.2,0.5,0.55};
GLfloat WATER_SPECULAR[]={0.7,0.7,1.0};
GLfloat WATER_SHININESS[]={64.0};
GLfloat WATER_EMISSION[]={0.0,0.1,0.1};

typedef struct{
    float draw;
    float viewing_angle;
    float focal_point[3];
    float opt_axis[3];
    float y_axis[3];
    float mat_diffuse[3];
    float water_width;
    float padding;
} GlViewDrawingPrams;

class GlViewEnv{
public:
    GLubyte *bits;
    GLuint width;
    GLuint height;
    GLfloat aspect_ratio;
    size_t buf_size;
    ShMemutexHandle shm;
    ShMemutexHandle cmd;
    char common_name[MAX_PATH];
    GlViewDrawingPrams drawing_params;

    GlViewEnv()
        :width(640),
         height(480),
         aspect_ratio(640.0/480.0),
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

        float w=(float)this->width;
        float h=(float)this->height;
        this->aspect_ratio=w/h;

        return this;
    }
};
GlViewEnv env;



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
        
        glViewport(0,0,env.width,env.height);
        gluPerspective(env.drawing_params.viewing_angle,
                       env.aspect_ratio,
                       10.0,1200.0);
        gluLookAt(env.drawing_params.focal_point[0],
                  env.drawing_params.focal_point[1],
                  env.drawing_params.focal_point[2],
                  env.drawing_params.opt_axis[0],
                  env.drawing_params.opt_axis[1],
                  env.drawing_params.opt_axis[2],
                  env.drawing_params.y_axis[0],
                  env.drawing_params.y_axis[1],
                  env.drawing_params.y_axis[2]);

        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();


        glMaterialfv(GL_FRONT,GL_DIFFUSE,env.drawing_params.mat_diffuse);
        glMaterialfv(GL_FRONT,GL_AMBIENT,MAT_AMBIENT);
        glMaterialfv(GL_FRONT,GL_SPECULAR,MAT_SPECULAR);
        glMaterialfv(GL_FRONT,GL_SHININESS,MAT_SHININESS);
        glutSolidTeapot(20.0);


        //
        // an good feature :)
        //
        if(env.drawing_params.water_width>0.0){

            //
            // move to outlet
            //
            glTranslated(32.5,9.0,0.0);

            glPushMatrix();

            GLUquadricObj *quad=gluNewQuadric();
            gluQuadricDrawStyle(quad,GLU_FILL);
            gluQuadricNormals(quad,GLU_SMOOTH);
            
            
            glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,WATER_COLOR);
            glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,WATER_AMBIENT);
            glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,WATER_SPECULAR);
            glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,WATER_SHININESS);
            glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,WATER_EMISSION);
            
            //
            // rotate same direction as camera Y axis.
            //
            GLfloat rot[16]={1.0,0.0,0.0,0.0,
                             0.0,1.0,0.0,0.0,
                             0.0,0.0,1.0,0.0,
                             0.0,0.0,0.0,1.0};
            rot[8]=-env.drawing_params.y_axis[0];
            rot[9]=-env.drawing_params.y_axis[1];
            rot[10]=-env.drawing_params.y_axis[2];
            glMultMatrixf(rot);

            //
            // show water
            //
            gluCylinder(quad,
                        env.drawing_params.water_width,
                        env.drawing_params.water_width,
                        500,128,1);
            
            gluDeleteQuadric(quad);
            glPopMatrix();
        }
        
        
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
    //
    // Press ESC to quit
    //
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
    
    glutCreateWindow(env.common_name);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

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
