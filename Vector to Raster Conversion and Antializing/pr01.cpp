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
#include <string.h>

using namespace std;

// =============================================================================
// These variables will store the input ppm image's width, height, and color
// =============================================================================
int width, height, num_sample;
unsigned char *pixmap;

int inside[3] = { 0xF1, 0xE7, 0x9F };
int outside[3] = { 0xDD, 0x67, 0x73 };


// =============================================================================
// setPixels()
//
// This function stores the RGB values of each pixel to "pixmap."
// Then, "glutDisplayFunc" below will use pixmap to display the pixel colors.
// =============================================================================
bool function(double x, double y, int function_number)
{
	if(function_number == 0)
	{
		if(y < 2.0*x && y > 4.0*(x-320.0) && (y > (-0.7)*x+300.0) && (y < (-0.2)*x + 480))
			return 1;
		else return 0;
	}
	
	else if(function_number == 1)
	{
		if((((y > 0.9*x && y < 2*(x-50)) || ((y < 0.2*x + 280) && (y > -0.3*x + 320)) ) && (y < -3.5*x + 1400)) || ((y >= -3.5*x + 1400 )&& y > 0.9*x && (y < 0.2*x + 280)))
			return 1;
		else return 0;
	}
		
	else if(function_number == 2)
	{
		if((x <= 300 && y < (exp(0.018 * x) + 100)) || (x > 440 && y < 1/(tan(0.0000007*(x-400)) * 100)) || (x <= 440 && x > 300 && y < (0.1* x + 300)))
			return 1;
		else return 0;
	}
	
	else if(function_number == 3)
	{
		if(pow(x-300,2)/15 + pow(y-200,2)/30 < 1000 || (pow(x-210,2) + pow(y-350,2) <5000) || (pow(x-450,2) + pow(y-250,2) <8000) ||  (pow(x-150,2) + pow(y-150,2) <3000))
			return 1;
		else return 0;
	}
		
	else return 0;
}

vector<int>* draw_shadow(int x, int y)
{
	vector<int> ret(3,0);
	int inside_color[3] = { 0,255,255 };
	int outside_color[3] = { 0x5C, 0x69, 0x83};
	if(pow((x - 320), 2) + pow((y - 240), 2) < 40000)
	{
		double distance = pow((pow((x - 320),2) + pow((y - 240),2)),0.5);
		if(distance <200)
		{
			ret[0] = 255 - 210/200 * distance;
		}
		else  ret[0] = 0;
		ret[1] = inside_color[1];
		ret[2] = inside_color[2];
	}
	else{
		ret[0] = outside_color[0];
		ret[1] = outside_color[1];
		ret[2] = outside_color[2];
	}
	
	return new vector<int>(ret);
}

vector<int>* calculate_color(int x, int y, int function_number)
{
	if(function_number != 4)
	{
		double scale = 1/num_sample;
		double x_r = scale * ((double) rand() / (RAND_MAX));
		double y_r = scale * ((double) rand() / (RAND_MAX));
		vector<int> RGB(3,0);
		
		for(int i = 0; i < num_sample; i++)
		{
			double temp_x = (x - 1) + i * scale + x_r;
			double temp_y = (y - 1) + i * scale + y_r;
			if(function(temp_x, temp_y, function_number) == 1)
			{
				RGB[0] += inside[0];
				RGB[1] += inside[1];
				RGB[2] += inside[2];
			}
			else
			{
				RGB[0] += outside[0];
				RGB[1] += outside[1];
				RGB[2] += outside[2];
			}
		}
		
		RGB[0] = RGB[0] / num_sample;
		RGB[1] = RGB[1] / num_sample;
		RGB[2] = RGB[2] / num_sample;

		return new vector<int>(RGB);
	}
	
	else{
		return draw_shadow(x, y);
	}
}

void setPixels(int function_number)
{
	for(int y = 0; y < height ; y++) {
		for(int x = 0; x < width; x++)
		{
			int i = (y * width + x) * 3;
			vector<int> RGB_value = *calculate_color(x, y, function_number);
		    pixmap[i++] = RGB_value[0];
			pixmap[i++] = RGB_value[1];
			pixmap[i] = RGB_value[2];
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
int main(int argc, char *argv[])
{

  //initialize the global variables
  width = 640;
  height =  480;
  num_sample = 10;
  pixmap = new unsigned char[width * height * 3];  //Do you know why "3" is used?
  if(argc == 1)
  {
	  cout << "Need input!" << endl << "Program Exit!" << endl;
	  return 0;
  }
  
  if(strcmp(argv[1],"convex") == 0)	setPixels(0);
  else if(strcmp(argv[1],"star") == 0)	setPixels(1);
  else if(strcmp(argv[1],"function") == 0)	setPixels(2);
  else if(strcmp(argv[1],"blobby") == 0)	setPixels(3);
  else if(strcmp(argv[1],"shaded") == 0)	setPixels(4);
  else
  {
	  cout << "Invaild input" << endl;
	  return 0;
  }

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

