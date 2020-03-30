/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields with Shaders.
  C++ starter code

  Student username: beiyouzh
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"
#include <vector>
#include <iostream>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position
int screenshot_index = 0;
int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
typedef enum {ONE, TWO, THREE, FOUR} DISPLAY_MODE;
DISPLAY_MODE displayMode = ONE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };
int uindex = 0;
int controlPoint = 0;
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";
float eye[3] = {0, 0, 0};
float lookat[3] = {0, 0, -1};
float up[3] = {0, 1, 0};
ImageIO * heightmapImage;

//vbo and ebo
GLuint Buffer;
//vao
GLuint pointVertexArray;
int sizeTri;
int sizeLine;
int sizePoint;
int fps = 60;
int time_count = 0;
float u = 0.0f;
OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;
// represents one control point along the spline 
struct Point 
{
  double x;
  double y;
  double z;
};

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline 
{
  int numControlPoints;
  Point * points;
};
// the spline array 
Spline * splines;
Point tangent;
Point normal;
Point binormal;
// total number of splines 
int numSplines;
int splineNum = 0;
vector<Point> splineCoord;
vector<Point> tangentCoord;
int loadSplines(char * argv) 
{
  char * cName = (char *) malloc(128 * sizeof(char));
  FILE * fileList;
  FILE * fileSpline;
  int iType, i = 0, j, iLength;

  // load the track file 
  fileList = fopen(argv, "r");
  if (fileList == NULL) 
  {
    printf ("can't open file %s\n", argv);
    exit(1);
  }

  // stores the number of splines in a global variable 
  fscanf(fileList, "%d", &numSplines);

  splines = (Spline*) malloc(numSplines * sizeof(Spline));

  // reads through the spline files 
  for (j = 0; j < numSplines; j++) 
  {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL) 
    {
      printf ("can't open file %s\n", cName);
      exit(1);
    }

    // gets length for spline file
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    // allocate memory for all the points
    splines[j].points = (Point *)malloc(iLength * sizeof(Point));
    splines[j].numControlPoints = iLength;

    // saves the data to the struct
    while (fscanf(fileSpline, "%lf %lf %lf", 
	   &splines[j].points[i].x, 
	   &splines[j].points[i].y, 
	   &splines[j].points[i].z) != EOF) 
    {
      i++;
    }
  }

  free(cName);

  return 0;
}

int initTexture(const char * imageFilename, GLuint textureHandle)
{
  // read the texture image
  ImageIO img;
  ImageIO::fileFormatType imgFormat;
  ImageIO::errorType err = img.load(imageFilename, &imgFormat);

  if (err != ImageIO::OK) 
  {
    printf("Loading texture from %s failed.\n", imageFilename);
    return -1;
  }

  // check that the number of bytes is a multiple of 4
  if (img.getWidth() * img.getBytesPerPixel() % 4) 
  {
    printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
    return -1;
  }

  // allocate space for an array of pixels
  int width = img.getWidth();
  int height = img.getHeight();
  unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

  // fill the pixelsRGBA array with the image pixels
  memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
  for (int h = 0; h < height; h++)
    for (int w = 0; w < width; w++) 
    {
      // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
      pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
      pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
      pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
      pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

      // set the RGBA channels, based on the loaded image
      int numChannels = img.getBytesPerPixel();
      for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
        pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
    }

  // bind the texture
  glBindTexture(GL_TEXTURE_2D, textureHandle);

  // initialize the texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

  // generate the mipmaps for this texture
  glGenerateMipmap(GL_TEXTURE_2D);

  // set the texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // query support for anisotropic texture filtering
  GLfloat fLargest;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
  printf("Max available anisotropic samples: %f\n", fLargest);
  // set anisotropic texture filtering
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

  // query for any errors
  GLenum errCode = glGetError();
  if (errCode != 0) 
  {
    printf("Texture initialization error. Error code: %d.\n", errCode);
    return -1;
  }
  
  // de-allocate the pixel array -- it is no longer needed
  delete [] pixelsRGBA;

  return 0;
}

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
int scale = 2;
  int ww = windowWidth * scale;
  int hh = windowHeight * scale;
  unsigned char * screenshotData = new unsigned char[ww * hh * 3];
  glReadPixels(0, 0, ww, hh, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  unsigned char * screenshotData1 = new unsigned char[windowWidth * windowHeight * 3];
  for (int h = 0; h < windowHeight; h++) {
    for (int w = 0; w < windowWidth; w++) {
      int h1 = h * scale;
      int w1 = w * scale;
      screenshotData1[(h * windowWidth + w) * 3] = screenshotData[(h1 * ww + w1) * 3];
      screenshotData1[(h * windowWidth + w) * 3 + 1] = screenshotData[(h1 * ww + w1) * 3 + 1];
      screenshotData1[(h * windowWidth + w) * 3 + 2] = screenshotData[(h1 * ww + w1) * 3 + 2];
    }
  }

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData1);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
  delete [] screenshotData1;
}
void computeCrossProduct(Point a, Point b, Point &c)
{
  c.x = a.y*b.z - b.y*a.z;
  c.y = a.z*b.x - b.z*a.x;
  c.z = a.x*b.y - b.x*a.y;
}
void normalize(Point &v)
{
    double mag = sqrt(pow(v.x,2) + pow(v.y,2) + pow(v.z,2));
    if (mag>=0)
    {
      v.x = v.x / mag;
      v.y = v.y / mag;
      v.z = v.z / mag;
    }
    else
    {
      v.x = 0;
      v.y = 0;
      v.z = 0;
    }
}
// computes the normal and binormal given the tangent at a point
void computeNormal(Point tangent, Point &normal, Point &binormal)
{
  //cout<<"tangent" << tangent.x << " " << tangent.y << " "<< tangent.z <<"\n";
  computeCrossProduct(binormal, tangent, normal);
    //cout<<"noraml" << normal.x << normal.y << normal.z <<"\n";
  normalize(normal);
  computeCrossProduct(tangent, normal, binormal);
  normalize(binormal);
}
void setCameraAttributes(int i)
{
  computeNormal(tangentCoord[i], normal, binormal);

  eye[0] = splineCoord[i].x + 0.05 * normal.x;
  eye[1] = splineCoord[i].y + 0.05 * normal.y;
  eye[2] = splineCoord[i].z + 0.05 * normal.z;

  lookat[0] = splineCoord[i+1].x + 0.05 * normal.x;
  lookat[1] = splineCoord[i+1].y + 0.05 * normal.y;
  lookat[2] = splineCoord[i+1].z + 0.05 * normal.z;

  up[0] = normal.x;
  up[1] = normal.y;
  up[2] = normal.z;
}
void displayFunc()
{
  // render some stuff...
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  setCameraAttributes(uindex);
  uindex++;
  u += 0.05;
  if (u>1)
  {
    u = 0;
    controlPoint++;
    if (controlPoint>splines[splineNum].numControlPoints-3)
    {
      controlPoint = 0;
      splineNum++;
      if (splineNum>=numSplines)
      {
        printf("return here");
        return;
      }
    }
  }
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  matrix.LookAt(eye[0], eye[1], eye[2], lookat[0], lookat[1], lookat[2], up[0], up[1], up[2]);
  matrix.Translate(landTranslate[0], landTranslate[1],landTranslate[2]);
  matrix.Rotate(landRotate[0], 1, 0, 0);
  matrix.Rotate(landRotate[1], 0, 1, 0);
  matrix.Rotate(landRotate[2], 0, 0, 1);
  matrix.Scale(landScale[0], landScale[1],landScale[2]);
  float m[16];
  matrix.GetMatrix(m);

  float p[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(p);
  //
  // bind shader
  pipelineProgram->Bind();

  // set variable
  pipelineProgram->SetModelViewMatrix(m);
  pipelineProgram->SetProjectionMatrix(p);
  
  //get mode varible
  GLint loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode");
  glUniform1f(loc,false);
  glBindVertexArray(pointVertexArray);
  glDrawArrays(GL_LINES, 0, sizePoint);
  
  glutSwapBuffers(); 
}

void idleFunc()
{
  // do some stuff... 

  // save screen shot as a rate of 15fps
  time_count++;
  if(time_count > fps/4){
    time_count = 0;
    char text[70];
    sprintf(text,"%.3d", screenshot_index);
    screenshot_index++;
    //saveScreenshot(text);
  }
  // make the screen update 
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  matrix.Perspective(54.0f, (float)w / (float)h, 0.01f, 1000.0f);
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the landscape
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;
    default:
      controlState = ROTATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;
    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;
    case 't':
      controlState = TRANSLATE;
    break;
    case 'x':
      // take a screenshot
      saveScreenshot("screenshot.jpg");
    break;
    case '1':
     displayMode = ONE;
    break;
    case '2':
     displayMode = TWO;
    break;
    case '3':
     displayMode = THREE;
     break;
    case '4':
     displayMode = FOUR;
     break;
  }
}
Point TangentInterpolation(float u, Point p1, Point p2,Point p3,Point p4){
  float s = 0.5f;
  float uu = u*u;
  float q1 = -s*3.0f*uu + 2.0f*s*2.0f*u - s;
  float q2 = (2.0f-s)*3.0f*uu + (s-3)*2.0f*u;
  float q3 = (s-2)*3.0f*uu + (3-2*s)*2.0f*u  + s;
  float q4 = s*3.0f*uu - s*2.0f*u;
  Point newPoint;
  newPoint.x = p1.x*q1 +  p2.x*q2 + p3.x * q3 + p4.x * q4;
  newPoint.y = p1.y*q1 +  p2.y*q2 + p3.y * q3 + p4.y * q4;
  newPoint.z = p1.z*q1 +  p2.z*q2 + p3.z * q3 + p4.z * q4;
  return newPoint;
}
//functions that calculate the desired point given
Point PointInterpolation(float u, Point p1, Point p2,Point p3,Point p4){
  float s = 0.5f;
  float uu = u*u;
  float uuu = uu*u;
  float q1 = -s*uuu + 2.0f*s*uu - s*u;
  float q2 = (2.0f-s)*uuu + (s-3)*uu  + 1.0f;
  float q3 = (s-2)*uuu + (3-2*s)*uu  + s*u;
  float q4 = s*uuu - s*uu;

  Point newPoint;
  newPoint.x = p1.x*q1 +  p2.x*q2 + p3.x * q3 + p4.x * q4;
  newPoint.y = p1.y*q1 +  p2.y*q2 + p3.y * q3 + p4.y * q4;
  newPoint.z = p1.z*q1 +  p2.z*q2 + p3.z * q3 + p4.z * q4;
  return newPoint;
}
void initScene(int argc, char *argv[])
{
  // load the image from a jpeg disk file to main memory

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  std::vector<float> pos;
  std::vector<float> color;
  //initialize buffer
  for(int i = 0; i < splines[0].numControlPoints-3;i++ ){
    //print each point;
    Point p1 = splines[0].points[i];
    Point p2 = splines[0].points[i+1];
    Point p3 = splines[0].points[i+2];
    Point p4 = splines[0].points[i+3];
    for(float u = 0.0f; u <= 1.0f; u+= 0.01f){
      Point newP = PointInterpolation(u,p1, p2, p3, p4);
      Point newTan =  TangentInterpolation(u, p1, p2, p3, p4);
      splineCoord.push_back(newP);
      normalize(newTan);
      tangentCoord.push_back(newTan);
      pos.push_back(newP.x);
      pos.push_back(newP.y);
      pos.push_back(newP.z);
      //push the same point twice to ensure the line draws correctly
      if(i==0 && u==0.0f){
        continue;
      }
      if(i==splines[0].numControlPoints-4 && u == 1.0f){
        continue;
      }
      pos.push_back(newP.x);
      pos.push_back(newP.y);
      pos.push_back(newP.z);
    }
  }
  for(int j = 0; j < pos.size()/3 -1; j++){
    color.push_back(0);
    color.push_back(1);
    color.push_back(0);
    color.push_back(1);
  }
  binormal.x = 0.1 - splineCoord[0].x;
  binormal.y = -1 - splineCoord[0].y;
  binormal.z = -0.1 - splineCoord[0].z;
  glGenBuffers(1, &Buffer);
  glBindBuffer(GL_ARRAY_BUFFER, Buffer);
  glBufferData(GL_ARRAY_BUFFER, (pos.size() + color.size()) * sizeof(float),
  NULL, GL_STATIC_DRAW); // init buffer’s size, but don’t assign any data
  // upload position data
  glBufferSubData(GL_ARRAY_BUFFER, 0,
  pos.size() * sizeof(float), pos.data());
  // upload color data
  glBufferSubData(GL_ARRAY_BUFFER, pos.size() * sizeof(float),
  color.size() * sizeof(float), color.data()); 
  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  //point vao
  glGenVertexArrays(1, &pointVertexArray);
  glBindVertexArray(pointVertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, Buffer);

  GLuint loc =
  glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)(sizeof(float)*pos.size()));
  glEnable(GL_DEPTH_TEST); 
  sizePoint = color.size();
  std::cout << "GL error: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc<2)
  {  
    printf ("usage: %s <trackfile>\n", argv[0]);
    exit(0);
  }

  // load the splines from the provided filename
  loadSplines(argv[1]);

  printf("Loaded %d spline(s).\n", numSplines);
  for(int i=0; i<numSplines; i++)
    printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

  // tells glut to use a particular display function to redraw 
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}


