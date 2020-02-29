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

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

ImageIO * heightmapImage;

//vbo and ebo
GLuint pointVertexBuffer, ColorVertexBuffer, lineElementBuffer, triElementBuffer;
//vao
GLuint pointVertexArray, lineVertexArray, triVertexArray, smoothVertexArray;
int sizeTri;
int sizeLine;
int sizePoint;
int fps = 60;
int time_count = 0;
OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;

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

void displayFunc()
{
  // render some stuff...
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  matrix.LookAt(0, 0, 500, 0, 0, 0, 0, 1, 0);

  //matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  //matrix.LoadIdentity();
  matrix.Translate(landTranslate[0], landTranslate[1],landTranslate[2]);
  matrix.Rotate(landRotate[0], 1, 0, 0);
  matrix.Rotate(landRotate[1], 0, 1, 0);
  matrix.Rotate(landRotate[2], 0, 0, 1);
  if(displayMode == FOUR){
    matrix.Scale(landScale[0]*1.33, landScale[1]*1.33,landScale[2]*1.33);
  }
  else{
    matrix.Scale(landScale[0], landScale[1],landScale[2]);
  }
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
  //check which mode to display
  switch(displayMode){
    case ONE:
    glUniform1f(loc,false);
     glBindVertexArray(pointVertexArray);
     glDrawArrays(GL_POINTS, 0, sizePoint);
    break;
    case TWO:
     glUniform1f(loc,false);
     glBindVertexArray(lineVertexArray);
     glDrawElements(
     GL_LINES,      // mode
     sizeLine,    // count
     GL_UNSIGNED_INT,   // type
     (void*)0           // element array buffer offset
     );
    break;
    case THREE:
    glUniform1f(loc,false);
     glBindVertexArray(triVertexArray);
     glDrawElements(
     GL_TRIANGLES,      // mode
     sizeTri,    // count
     GL_UNSIGNED_INT,   // type
     (void*)0           // element array buffer offset
     );
    break;
    case FOUR:
    glUniform1f(loc,true);
    glBindVertexArray(smoothVertexArray);
     glDrawElements(
     GL_TRIANGLES,      // mode
     sizeTri,    // count
     GL_UNSIGNED_INT,   // type
     (void*)0           // element array buffer offset
     );
    break;
  }
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

void initScene(int argc, char *argv[])
{
  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  //initialize vertex data
  unsigned int size = heightmapImage->getWidth() * heightmapImage->getHeight();
  glm::vec3 point[size];
  glm::vec4 point_color[size];
  glm::vec3 left[size];
  glm::vec3 right[size];
  glm::vec3 up[size];
  glm::vec3 down[size];
  int index = 0;
  //point data
  for(int i = 0; i < heightmapImage->getWidth(); i++){
    for(int j = 0; j < heightmapImage->getHeight(); j++){
      float color_level = heightmapImage->getPixel(i, j, 0);
      //initialize point data
      point[index] = glm::vec3(i-128.0f, j-128.0f, color_level);
      point_color[index] = glm::vec4(color_level/256.0, color_level/256.0, color_level/256.0, 1.0);
      index++;
    }
  }
  
  //initialie buffer
  glGenBuffers(1, &pointVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, pointVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * size, point,
               GL_STATIC_DRAW);
  glGenBuffers(1, &ColorVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, ColorVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * size, point_color, GL_STATIC_DRAW);
  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  //point vao
  glGenVertexArrays(1, &pointVertexArray);
  glBindVertexArray(pointVertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, pointVertexBuffer);

  GLuint loc =
  glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glBindBuffer(GL_ARRAY_BUFFER, ColorVertexBuffer);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)0);
  //created indices of short
  std::vector<int> indices;
  int count = 0;
  for(int i = 0; i < heightmapImage->getWidth(); i++){
    for(int j = 0; j < heightmapImage->getHeight(); j++){
      if(i != heightmapImage->getWidth()-1){
        indices.push_back(count);
        indices.push_back(count+heightmapImage->getHeight());
      }
      if(j!= heightmapImage->getHeight()-1){
        indices.push_back(count);
        indices.push_back(count+1);
      }
      count++;
    }
  }
	glGenVertexArrays(1, &lineVertexArray);
    //create ebo object
	glBindVertexArray(lineVertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, pointVertexBuffer);
  loc =
  glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glBindBuffer(GL_ARRAY_BUFFER, ColorVertexBuffer);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)0);
  
  glGenBuffers(1, &lineElementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

  //create vao for triangle
  std::vector<int> tri_indices;
  count = 0;
  for(int i = 0; i < heightmapImage->getWidth()-1; i++){
    for(int j = 0; j < heightmapImage->getHeight()-1; j++){
      count = i*heightmapImage->getWidth() + j;
      tri_indices.push_back(count);
      tri_indices.push_back(count+1);
      tri_indices.push_back(count+1+heightmapImage->getHeight());
      tri_indices.push_back(count+heightmapImage->getHeight());
      tri_indices.push_back(count+1+heightmapImage->getHeight());
      tri_indices.push_back(count);
    }
  }
	glGenVertexArrays(1, &triVertexArray);
    //create ebo object
	glBindVertexArray(triVertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, pointVertexBuffer);
  loc =
  glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glBindBuffer(GL_ARRAY_BUFFER, ColorVertexBuffer);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)0);
  
  glGenBuffers(1, &triElementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tri_indices.size() * sizeof(unsigned int), &tri_indices[0], GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

  //create data
  index = 0;
  for(int i = 0; i < heightmapImage->getWidth(); i++){
    for(int j = 0; j < heightmapImage->getHeight(); j++){
      //initialize point data
      left[index] = i==0? glm::vec3(i-1 - 128.0f, j- 128.0f, heightmapImage->getPixel(i, j, 0)): 
          glm::vec3(i-1- 128.0f, j- 128.0f, heightmapImage->getPixel(i-1, j, 0));
      right[index] = i==heightmapImage->getWidth()-1? glm::vec3(i+1- 128.0f, j- 128.0f, heightmapImage->getPixel(i, j, 0)): 
          glm::vec3(i+1- 128.0f, j- 128.0f, heightmapImage->getPixel(i+1, j, 0));
      down[index] = j==0? glm::vec3(i- 128.0f, j-1- 128.0f, heightmapImage->getPixel(i, j, 0)): 
          glm::vec3(i- 128.0f, j-1- 128.0f, heightmapImage->getPixel(i, j-1, 0));
      up[index] = j==heightmapImage->getHeight()-1? glm::vec3(i- 128.0f, j+1- 128.0f, heightmapImage->getPixel(i, j, 0)): 
          glm::vec3(i- 128.0f, j- 128.0f+1, heightmapImage->getPixel(i, j+1, 0));
      index++;
    }
  }

  //create vbo fo each data
  GLuint leftBuffer, rightBuffer, upBuffer, downBuffer;
  glGenBuffers(1, &leftBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, leftBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * size, left,
               GL_STATIC_DRAW);

  glGenBuffers(1, &rightBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, rightBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * size, right,
               GL_STATIC_DRAW);

  glGenBuffers(1, &upBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, upBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * size, up,
               GL_STATIC_DRAW);

  glGenBuffers(1, &downBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, downBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * size, down,
               GL_STATIC_DRAW);
  //create vao for smoothing
  glGenVertexArrays(1, &smoothVertexArray);
	glBindVertexArray(smoothVertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, pointVertexBuffer);
  loc =
  glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
  glBindBuffer(GL_ARRAY_BUFFER, upBuffer);
  loc =
  glGetAttribLocation(pipelineProgram->GetProgramHandle(), "up");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
  glBindBuffer(GL_ARRAY_BUFFER, downBuffer);
  loc =
  glGetAttribLocation(pipelineProgram->GetProgramHandle(), "down");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
  glBindBuffer(GL_ARRAY_BUFFER, leftBuffer);
  loc =
  glGetAttribLocation(pipelineProgram->GetProgramHandle(), "left");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
  glBindBuffer(GL_ARRAY_BUFFER, rightBuffer);
  loc =
  glGetAttribLocation(pipelineProgram->GetProgramHandle(), "right");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glBindBuffer(GL_ARRAY_BUFFER, ColorVertexBuffer);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)0);
  
  glGenBuffers(1, &triElementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tri_indices.size() * sizeof(unsigned int), &tri_indices[0], GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

  glEnable(GL_DEPTH_TEST); //could be a problem


  sizePoint = size;
  sizeLine = indices.size();
  sizeTri = tri_indices.size();
  std::cout << "GL error: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

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


