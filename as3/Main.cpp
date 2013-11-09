
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#ifdef OSX
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#include <time.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define PI 3.14159265  // Should be used from mathlib
inline float sqr(float x) { return x*x; }

using namespace std;

//****************************************************
// Some Classes
//****************************************************

class Viewport;

class Viewport {
public:
	int w, h; // width and height
};

class Point {
public:
	GLfloat x, y, z;
};

class BCurve {
public:
	Point p1, p2, p3, p4;
};

class BPatch {
public:
	BCurve c1, c2, c3, c4;
};

//****************************************************
// Global Variables
//****************************************************
Viewport	viewport;
// Material
GLfloat stepSize;
std::vector<BPatch*> bPatches;
int numPatches;


//****************************************************
// Simple init function
//****************************************************
void initScene(){

}

//****************************************************
// File Parser
//****************************************************
void loadScene(std::string file) {
	numPatches = 0;
	int patchCount = 0;
	int curveCount = 0;

	std::ifstream inpfile(file.c_str());
	if(!inpfile.is_open()) {
		std::cout << "Unable to open file" << std::endl;
	} else {
		std::string line;

		while(inpfile.good()) {
			std::vector<std::string> splitline;
			std::string buf;

			std::getline(inpfile,line);
			std::stringstream ss(line);

			while (ss >> buf) {
				splitline.push_back(buf);
			}

			//Ignore blank lines
			if(splitline.size() == 0) {
				continue;
			}

			if(splitline.size() == 1) {
				numPatches = atoi(splitline[0].c_str());
				for (int i = 0; i < numPatches; i++) {
					BPatch* newPatch = new BPatch;
					bPatches.push_back(newPatch);
				}
			} else {
				if (patchCount < numPatches) {
					if (curveCount == 3) {

						Point* tempP1 = new Point;
						Point* tempP2 = new Point;
						Point* tempP3 = new Point;
						Point* tempP4 = new Point;

						tempP1->x = atof(splitline[0].c_str());
						tempP1->y = atof(splitline[1].c_str());
						tempP1->z = atof(splitline[2].c_str());

						tempP2->x = atof(splitline[3].c_str());
						tempP2->y = atof(splitline[4].c_str());
						tempP2->z = atof(splitline[5].c_str());

						tempP3->x = atof(splitline[6].c_str());
						tempP3->y = atof(splitline[7].c_str());
						tempP3->z = atof(splitline[8].c_str());

						tempP4->x = atof(splitline[9].c_str());
						tempP4->y = atof(splitline[10].c_str());
						tempP4->z = atof(splitline[11].c_str());

						BCurve* tempC = new BCurve;
						tempC->p1 = *tempP1;
						tempC->p2 = *tempP2;
						tempC->p3 = *tempP3;
						tempC->p4 = *tempP4;

						bPatches.at(patchCount)->c4 = *tempC;

						patchCount++;
						curveCount = 0;

					} else {

						Point* tempP1 = new Point;
						Point* tempP2 = new Point;
						Point* tempP3 = new Point;
						Point* tempP4 = new Point;

						tempP1->x = atof(splitline[0].c_str());
						tempP1->y = atof(splitline[1].c_str());
						tempP1->z = atof(splitline[2].c_str());

						tempP2->x = atof(splitline[3].c_str());
						tempP2->y = atof(splitline[4].c_str());
						tempP2->z = atof(splitline[5].c_str());

						tempP3->x = atof(splitline[6].c_str());
						tempP3->y = atof(splitline[7].c_str());
						tempP3->z = atof(splitline[8].c_str());

						tempP4->x = atof(splitline[9].c_str());
						tempP4->y = atof(splitline[10].c_str());
						tempP4->z = atof(splitline[11].c_str());

						BCurve* tempC = new BCurve;
						tempC->p1 = *tempP1;
						tempC->p2 = *tempP2;
						tempC->p3 = *tempP3;
						tempC->p4 = *tempP4;

						if (curveCount == 0) {
							bPatches.at(patchCount)->c1 = *tempC;
						} else if (curveCount == 1) {
							bPatches.at(patchCount)->c2 = *tempC;
						} else {
							bPatches.at(patchCount)->c3 = *tempC;
						}

						curveCount++;
					}
				}
			}

		}
		inpfile.close();
	}
}

//****************************************************
// reshape viewport if the window is resized
//****************************************************
void myReshape(int w, int h) {
	viewport.w = w;
	viewport.h = h;

	glViewport (0,0,viewport.w,viewport.h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, viewport.w, 0, viewport.h);
}


//*********************************************
// Helper Methods
//*********************************************

Point multiplyPoint(GLfloat s, Point p){
	p.x *= s;
	p.y *= s;
	p.z *= s;
	return p;
}

Point addPoint(Point p1, Point p2){
	p1.x += p2.x;
	p1.y += p2.y;
	p1.z += p2.z;
	return p1;
}

Point subtractPoint(Point p1, Point p2){
	p1.x -= p2.x;
	p1.y -= p2.y;
	p1.z -= p2.z;
	return p1;
}


//****************************************************
// A routine to set a pixel by drawing a GL point.  This is not a
// general purpose routine as it assumes a lot of stuff specific to
// this example.
//****************************************************

Point Bernstein(GLfloat u, BCurve curve){
	Point a, b, c, d, r;

	a = multiplyPoint(pow(u, 3.0), curve.p1);
	b = multiplyPoint(3.0*pow(u, 2.0)*(1.0-u), curve.p2);
	c = multiplyPoint(3.0*u*pow((1.0-u), 2.0), curve.p3);
    d = multiplyPoint(pow((1-u),3), curve.p4);

	r = addPoint(addPoint(a, b), addPoint(c, d));
	return r;
}

//****************************************************
// Draw a filled circle.  
//****************************************************
void bezier(float centerX, float centerY, float radius) {
	// Draw inner circle
	glColor3f(0.5, 0.5, 0.5);
	glBegin(GL_QUADS);

	glEnd();
}
//****************************************************
// function that does the actual drawing of stuff
//***************************************************
void myDisplay() {

	glClear(GL_COLOR_BUFFER_BIT);				// clear the color buffer

	glMatrixMode(GL_MODELVIEW);			        // indicate we are specifying camera transformations
	glLoadIdentity();				        // make sure transformation is "zero'd"

	// Start drawing
	//circle(viewport.w /2.0 , viewport.h / 2.0 , min(viewport.w, viewport.h) / 3.0);

	glFlush();
	glutSwapBuffers();					// swap buffers (we earlier set double buffer)
}


void keyboard(unsigned char key, int x, int y) {
	switch (key)  {
	case 32: // Space key
		exit (0);
		break;
	}
}
//****************************************************
// the usual stuff, nothing exciting here
//****************************************************
int main(int argc, char *argv[]) {

	std::string filename = argv[1];
	GLfloat subdivision = atof(argv[2]);

	loadScene(filename);

	//This initializes glut
	glutInit(&argc, argv);

	//This tells glut to use a double-buffered window with red, green, and blue channels 
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

	// Initalize theviewport size
	viewport.w = 400;
	viewport.h = 400;

	//The size and position of the window
	glutInitWindowSize(viewport.w, viewport.h);
	glutInitWindowPosition(0,0);
	glutCreateWindow(argv[0]);

	initScene();							// quick function to set up scene

	glutDisplayFunc(myDisplay);				// function to run when its time to draw something
	glutReshapeFunc(myReshape);				// function to run when the window gets resized
	glutKeyboardFunc(keyboard);
	glutMainLoop();							// infinite loop that will keep drawing and resizing
	// and whatever else

	return 0;
}

