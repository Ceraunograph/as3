
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
// Helper Classes
//****************************************************

class Viewport {
public:
	int w, h; // width and height
};

class Point {
public:
	GLfloat x, y, z;
	void multiplyPoint(GLfloat s, Point p){
		x = s * p.x;
		y = s * p.y;
		z = s * p.z;	
	}

	void addPointTwoArg(Point p1, Point p2){
		x = p1.x + p2.x;
		y = p1.y + p2.y;
		z = p1.z + p2.z;
	}

	void addPointOneArg(Point p1){
		x = x + p1.x;
		y = y + p1.y;
		z = z + p1.z;
	}

	void subtractPointTwoArg(Point p1, Point p2){
		x = p1.x - p2.x;
		y = p1.y - p2.y;
		z = p1.z - p2.z;
	}

	void subtractPointOneArg(Point p1){
		x = x - p1.x;
		y = y - p1.y;
		z = z - p1.z;
	}
};

class BCurve {
public:
	Point p1, p2, p3, p4;

	void Bernstein(GLfloat u,Point p){
		Point a, b, c, d;

		a.multiplyPoint(pow(u, 3.0), p1);
		b.multiplyPoint(3.0*pow(u, 2.0)*(1.0-u), p2);
		c.multiplyPoint(3.0*u*pow((1.0-u), 2.0), p3);
		d.multiplyPoint(pow((1-u),3), p4);

		p.addPointTwoArg(a, b);
		p.addPointOneArg(c);
		p.addPointOneArg(d);
	}
};

class BPatch {
public:
	BCurve c1, c2, c3, c4;
};

//****************************************************
// Global Variables
//****************************************************
Viewport	viewport;

GLfloat stepSize;
std::vector<BPatch*> bPatches;
int numPatches;

// Wired Mode or Filled Mode
bool wired = false;
bool smooth = false;

//Light Source Information
//Light Zero
GLfloat diffuse0[]={0.5, 0.0, 0.0, 1.0};
GLfloat ambient0[]={0.0, 0.0, 0.0, 1.0};
GLfloat specular0[]={1.0, 0.0, 0.0, 1.0};
GLfloat light0_pos[]={1.0, 0.0, 3,0, 1.0};

//****************************************************
// reshape viewport if the window is resized
//****************************************************
void myReshape(int w, int h) {
	viewport.w = w;
	viewport.h = h;

	glViewport (0,0,viewport.w,viewport.h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-1, 1, -1, 1);
	//gluOrtho2D(0, viewport.w, 0, viewport.h);
	//glOrtho(-1, 1, -1, 1, 1, -1);
}

//****************************************************
// Simple init function
//****************************************************

void initScene(){
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear to black, fully transparent
	myReshape(viewport.w,viewport.h);

	glEnable(GL_NORMALIZE);

	//Enable Light Source Number Zero
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular0);
}


//*********************************************
// Helper Methods
//*********************************************






void drawPolygon(Point old1, Point old2, Point old3, Point old4, Point new1, Point new2, Point new3, Point new4){
	glBegin(GL_POLYGON);
	//glNormal3f();
	glVertex3f(old1.x, old1.y, old1.z);
	glVertex3f(new1.x, new1.y, new1.z);
	glVertex3f(new2.x, new2.y, new2.z);
	glVertex3f(old2.x, old2.y, old2.z);
	glEnd();

	glBegin(GL_POLYGON);
	//glNormal3f();
	glVertex3f(old2.x, old2.y, old2.z);
	glVertex3f(new2.x, new2.y, new2.z);
	glVertex3f(new3.x, new3.y, new3.z);
	glVertex3f(old3.x, old3.y, old3.z);
	glEnd();

	glBegin(GL_POLYGON);
	//glNormal3f();
	glVertex3f(old3.x, old3.y, old3.z);
	glVertex3f(new3.x, new1.y, new3.z);
	glVertex3f(new4.x, new2.y, new4.z);
	glVertex3f(old4.x, old4.y, old4.z);
	glEnd();
}


void curveTraversal(BPatch patch){
	Point old1;
	Point old2;
	Point old3;
	Point old4;
	Point new1;
	Point new2;
	Point new3;
	Point new4;

	GLfloat old_u = 0.0;

	patch.c1.Bernstein(old_u, old1);
	patch.c2.Bernstein(old_u, old2);
	patch.c3.Bernstein(old_u, old3);
	patch.c4.Bernstein(old_u, old4);
	for (GLfloat u = 0.0; u < 1.0; u += stepSize){
		GLfloat new_u = old_u + stepSize;

		if (new_u > 1.0){
			new_u = 1.0;
		}

		patch.c1.Bernstein(new_u, new1);
		patch.c2.Bernstein(new_u, new2);
		patch.c3.Bernstein(new_u, new3);
		patch.c4.Bernstein(new_u, new4);

		drawPolygon(old1, old2, old3, old4, new1, new2, new3, new4);

		old1 = new1;
		old2 = new2;
		old3 = new3;
		old4 = new4;
		
		old_u = new_u; 

	}
}

void uniformTesselation(){
	for(int i = 0; i < bPatches.size(); i ++){
		curveTraversal(*(bPatches.at(i)));
}

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
// function that does the actual drawing of stuff
//***************************************************
void myDisplay() {
	glClear(GL_COLOR_BUFFER_BIT);				// clear the color buffer
	glMatrixMode(GL_MODELVIEW);			        // indicate we are specifying camera transformations
	glLoadIdentity();				        // make sure transformation is "zero'd"


	glBegin(GL_POLYGON);
	glVertex3f(0.3f, -0.8f, -0.1f);
	glVertex3f(0.3f, 0.5f, -0.1f);
	glVertex3f(0.6f, 0.5f, 0.0f);
	glVertex3f(0.6f, -0.8f, 0.0f);
	glEnd();


	glBegin(GL_POLYGON);
	glVertex3f(-0.3f, 0.5f, -0.1f);
	glVertex3f(0.3f, 0.0f, -0.1f);
	glVertex3f(0.5f, 0.0f, 0.0f);
	glEnd();

	glFlush();
	glutSwapBuffers();					// swap buffers (we earlier set double buffer)
}


void keyboard(unsigned char key, int x, int y) {
	switch (key)  {
	case 32: // Space key
		exit (0);
		break;
	case 115: //s key
		if (smooth){
			glEnable(GL_FLAT);
			glShadeModel(GL_FLAT);
			smooth = false;
		}else{
			glEnable(GL_SMOOTH);
			glShadeModel(GL_SMOOTH);
			smooth = true;
		}
		break;
	case 119: //w key
		if (!wired){
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
			wired = true;
		}else{
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
			wired = false;
		}
		break;
	}
}

void myFrameMove() {
	//nothing here for now
#ifdef _WIN32
	Sleep(10);                                   //give ~10ms back to OS (so as not to waste the CPU)
#endif
	glutPostRedisplay(); // forces glut to call the display function (myDisplay())
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
	glutIdleFunc(myFrameMove);	
	glutKeyboardFunc(keyboard);
	glutMainLoop();							// infinite loop that will keep drawing and resizing
	// and whatever else

	return 0;
}

