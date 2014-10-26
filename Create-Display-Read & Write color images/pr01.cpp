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

#include <fstream>
#include <cassert>
#include <sstream>
#include <string>
#include <cmath>

using namespace std;

// =============================================================================
// These variables will store the input ppm image's width, height, and color
// =============================================================================
int width, height;
unsigned char *pixmap;
const char* RED = "red";
const char* GREEN = "green";
const char* BLUE = "blue";
const char* ALL = "all";
const char* CIRCLE = "circle";



// =============================================================================
// setPixels()
//
// This function stores the RGB values of each pixel to "pixmap."
// Then, "glutDisplayFunc" below will use pixmap to display the pixel colors.
// =============================================================================
bool inside_circle(int x, int y)
{
	int function_value = pow((x - 320), 2) + pow((y - 240), 2);
	return (function_value < 40000);	
}

void setPixels(int value)
{
	if(value == 0 || value == 1|| value == 2)
	{
		for(int y = 0; y < height ; y++) 
		{
			for(int x = 0; x < width; x++) 
			{
			   int i = (y * width + x) * 3; 
			   pixmap[i++] = 255 * (value == 0);
			   pixmap[i++] = 255 * (value == 1); //Do you know what "0xFF" represents? Google it!
			   pixmap[i] = 255 * (value == 2); //Learn to use the "0x" notation to your advantage.
			}
		}
		return;		
	}
	
	else if(value == 3)
	{
		for(int y = 0; y < height ; y++) 
		{
			for(int x = 0; x < width; x++) 
			{
				int i = (y * width + x) * 3; 
				pixmap[i++] = 255 * ((y > 0.5 * height && x < 0.5 * width) || (y < 0.5 * height && x > 0.5 * width));
				pixmap[i++] = 255 * (x > 0.5 * width);
				pixmap[i] = 255 * (y <0.5 * height && x < 0.5*width);
			}
		}
		return;
	}	

	else if(value == 4)
	{
		for(int y = 0; y < height ; y++) 
		{
			for(int x = 0; x < width; x++) 
			{
				int i = (y * width + x) * 3; 
				pixmap[i++] = 255 * inside_circle(x, y);
				pixmap[i++] = 255 * inside_circle(x, y);
				pixmap[i] = 255 * (!inside_circle(x, y));
			}
		}
	}
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

int translate_input(char* input)
{
	stringstream ss;
	string input_string;
	ss << input;
	ss >> input_string; 
	if(input_string.compare(RED) == 0)		return 0;
	else if(input_string.compare(GREEN) == 0)	return 1;
	else if(input_string.compare(BLUE) == 0)	return 2;
	else if(input_string.compare(ALL) == 0)	return 3;
	else if(input_string.compare(CIRCLE) == 0)	return 4;
	else return -1;
}

int main(int argc, char *argv[])
{

  //initialize the global variables
  width = 640;
  height = 480;
  pixmap = new unsigned char[width * height * 3];  //Do you know why "3" is used?

  int imag = translate_input(argv[1]);
  setPixels(imag);


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

