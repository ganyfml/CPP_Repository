// =============================================================================
// VIZA654/CSCE646 at Texas A&M University
// Homework 0
// Created by Anton Agana based from Ariel Chisholm's template
// 05.23.2011
//
// This file is supplied with an associated makefile. Put both files in the same
// directory, navigate to that directory from the Linux shell, and type 'make'.
// This will create a program called 'pr01' that you can run by entering
// 'homework0' as a command in the shell.
//
// If you are new to programming in Linux, there is an
// excellent introduction to makefile structure and the gcc compiler here:
//
// http://www.cs.txstate.edu/labs/tutorials/tut_docs/Linux_Prog_Environment.pdf
//
// =============================================================================

#include <cstdlib>
#include <iostream>
#include <GL/glut.h>
#include <vector>

#include <fstream>
#include <cassert>
#include <sstream>
#include <string>
#include <cmath>
#include <vector>

using namespace std;

// =============================================================================
// These variables will store the input ppm image's width, height, and color
// =============================================================================
int width, height;
unsigned char *pixmap;


// =============================================================================
// setPixels()
//
// This function stores the RGB values of each pixel to "pixmap."
// Then, "glutDisplayFunc" below will use pixmap to display the pixel colors.
// =============================================================================
void setPixels()
{
   for(int y = 0; y < height ; y++) {
     for(int x = 0; x < width; x++) {
       int i = (y * width + x) * 3; 
       if(pow(x-300,2)/15 + pow(y-200,2)/30 < 1000 || (pow(x-210,2) + pow(y-350,2) <5000) || (pow(x-450,2) + pow(y-250,2) <8000) ||  (pow(x-150,2) + pow(y-150,2) <3000))
		{
		    pixmap[i++] = 0xF1;
			pixmap[i++] = 0xE7; //Do you know what "0xFF" represents? Google it!
			pixmap[i] = 0x9F; //Learn to use the "0x" notation to your advantage.
		}
       else
       {
		    pixmap[i++] = 0xDD;
			pixmap[i++] = 0x67; //Do you know what "0xFF" represents? Google it!
			pixmap[i] = 0x73; //Learn to use the "0x" notation to your advantage.
		}
     }
   }
}

void draw_test()
{
	Vector2d normal[4]; 	//Normals
	normal[0] = Vector2d(3,18);
	normal[1] = Vector2d(-4,2);
	normal[2] = Vector2d(-2,-5);
	normal[3] = Vector2d(10,-3);
	
	Vector2d point[4]; 	//points on the line
	point[0] = Vector2d(500,350);
	point[1] = Vector2d(200,320);
	point[2] = Vector2d(120,150);
	point[3] = Vector2d(350,80);
	

	
	int samples = 3; 	// number of samples
	
	Vector3d iColor(100,100,75); 	//inside color
	Vector3d oColor(255,200,179); 	//outside color

   for(int y = 0; y < height ; y++) 
	{
		for(int x = 0; x < width; x++) 
		{

			Vector3d tColor(0,0,0); 	// reset color

		 float randx = (1.0*rand())/RAND_MAX;
		 float randy = (1.0*rand())/RAND_MAX;

			for(int m=0; m < samples; m++)
			{
				for(int n=0; n < samples; n++)
				{
				float newX = x + ((float)m/samples) + randx/samples;
				float newY = y + ((float)n/samples) + randy/samples;

				Vector2d p = Vector2d(newX,newY);
       				// convex function
			 	if(((normal[0]*(p-point[0]))<=0) && 
					((normal[1]*(p-point[1]))<=0) && 
					((normal[2]*(p-point[2]))<=0) && 
					((normal[3]*(p-point[3]))<=0))
					{
						tColor.x += iColor.x;
						tColor.y += iColor.y; 
						tColor.z += iColor.z;
					}
				else 
					{
						tColor.x += oColor.x;
						tColor.y += oColor.y; 
						tColor.z += oColor.z;
					}	
    			}
  		}

		int i = (y * width + x) * 3;
		pixmap[i++] = tColor.x/pow(samples, 2);
		pixmap[i++] = tColor.y/pow(samples, 2); 
		pixmap[i] = tColor.z/pow(samples, 2);	
	}
	
}}

bool function(int x, int y)
{
	if(y < 2*x && y > 4*(x-320) && (y > (-0.7)*x+300) && (y < (-0.2)*x + 480))
		return 1;
	else return 0;
	
	if((((y > 0.9*x && y < 2*(x-50)) || ((y < 0.2*x + 280) && (y > -0.3*x + 320)) ) && (y < -3.5*x + 1400)) || ((y >= -3.5*x + 1400 )&& y > 0.9*x && (y < 0.2*x + 280)))
		return 1;
		
	if((x <= 300 && y < (exp(0.018 * x) + 100)) || (x > 440 && y < 1/(tan(0.0000007*(x-400)) * 100)) || (x <= 440 && x > 300 && y < (0.1* x + 300)))
		return 0;
}

// =============================================================================
// OpenGL Display and Mouse Processing Functions.
//
// You can read up on OpenGL and modify these functions, as well as the commands
// in main(), to perform more sophisticated display or GUI behavior. This code
// will service the bare minimum display needs for most assignments.
// =============================================================================
static void windowResize(int w, int h)
{   
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0,(w/2),0,(h/2),0,1); 
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity() ;
}
static void windowDisplay(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glRasterPos2i(0,0);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, pixmap);
  glFlush();
}
static void processMouse(int button, int state, int x, int y)
{
  if(state == GLUT_UP)
  exit(0);               // Exit on mouse click.
}
static void init(void)
{
  glClearColor(1,1,1,1); // Set background color.
}

// =============================================================================
// main() Program Entry
// =============================================================================
int main(int argc, char *argv[])
{

  //initialize the global variables
  width = 640;
  height =  480;
  pixmap = new unsigned char[width * height * 3];  //Do you know why "3" is used?

  setPixels();


  // OpenGL Commands:
  // Once "glutMainLoop" is executed, the program loops indefinitely to all
  // glut functions.  
  glutInit(&argc, argv);
  glutInitWindowPosition(100, 100); // Where the window will display on-screen.
  glutInitWindowSize(width, height);
  glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
  glutCreateWindow("Homework Zero");
  init();
  glutReshapeFunc(windowResize);
  glutDisplayFunc(windowDisplay);
  glutMouseFunc(processMouse);
  glutMainLoop();

  return 0; //This line never gets reached. We use it because "main" is type int.
}

