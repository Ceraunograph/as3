
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

#ifdef _WIN32
static DWORD lastTime;
#else
static struct timeval lastTime;
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
};

class BCurve {
public:
	Point p1, p2, p3, p4;
};

class BPatch {
public:
	BCurve c1, c2, c3, c4;
};

class Triangle {
public:
	Point p1, p2, p3, pc1, pc2, pc3;
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

GLfloat yRot = 0.0;
GLfloat xRot = 0.0;
GLfloat xTran = 0.0;
GLfloat yTran = 0.0;

GLfloat maxX = 1.0;
GLfloat minX = -1.0;
GLfloat maxY = 1.0;
GLfloat minY = -1.0;

//Light Source Information
//Light Zero
GLfloat diffuse0[]={0.8, 0.2, 0.2, 1.0};
GLfloat ambient0[]={1.0, 0.0, 0.0, 1.0};
GLfloat specular0[]={1.0, 1.0, 1.0, 1.0};
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
	glOrtho(minX, maxX, minY, maxY, -100, 100);
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

Point multiplyPoint(GLfloat s, Point p){
	Point r;
	r.x = p.x * s;
	r.y = p.y * s;
	r.z = p.z * s;
	return r;
}

Point addPoint(Point p1, Point p2){
	Point r;
	r.x = p1.x + p2.x;
	r.y = p1.y + p2.y;
	r.z = p1.z + p2.z;
	return r;
}

Point subtractPoint(Point p1, Point p2){
	Point r;
	r.x = p1.x - p2.x;
	r.y = p1.y - p2.y;
	r.z = p1.z - p2.z;
	return r;
}

Point normalize(Point p){
	Point r;
	GLfloat len = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
	r.x = p.x / len;
	r.y = p.y / len;
	r.z = p.z / len;
	return r;
}

Point crossProduct(Point p1, Point p2){
	Point r;
	r.x = p1.y * p2.z - p1.z * p2.y;
	r.y = p1.z * p2.x - p1.x * p2.z;
	r.z = p1.x * p2.y - p1.y * p2.x;
	return r;
}

Point getNormal(Point p1, Point p2, Point p3){
	return normalize(crossProduct(normalize(subtractPoint(p3, p1)), normalize(subtractPoint(p2, p1))));
}

Point midPoint(Point p1, Point p2) {
	Point r;
	r.x = (p1.x + p2.x)/2.0;
	r.y = (p1.y + p2.y)/2.0;
	r.z = (p1.z + p2.z)/2.0;
	return r;
}

GLfloat distancePoint(Point p1, Point p2) {
	return sqrt(pow(p1.x-p2.x, 2)+pow(p1.y-p2.y, 2) + pow(p1.z-p2.z, 2));
}

Point bernstein(GLfloat u, BCurve curve){
	Point a, b, c, d, r;

	a = multiplyPoint(pow(u, 3.0), curve.p1);
	b = multiplyPoint(3.0*pow(u, 2.0)*(1.0-u), curve.p2);
	c = multiplyPoint(3.0*u*pow((1.0-u), 2.0), curve.p3);
	d = multiplyPoint(pow((1.0-u),3.0), curve.p4);

	r = addPoint(addPoint(a, b), addPoint(c, d));
	return r;
}

Point patchPoint(GLfloat u, GLfloat v, BPatch patch) {
	BCurve c1;

	c1.p1 = bernstein(u, patch.c1);
	c1.p2 = bernstein(u, patch.c2);
	c1.p3 = bernstein(u, patch.c3);
	c1.p4 = bernstein(u, patch.c4);

	return bernstein(v, c1);
}

void drawPolygon(Point p1, Point p2, Point p3, Point p4){
	Point n;
	n = getNormal(p1, p2, p3);
	glBegin(GL_POLYGON);
	glNormal3f(n.x, n.y, n.z);
	glVertex3f(p1.x, p1.y, p1.z);
	glVertex3f(p2.x, p2.y, p2.z);
	glVertex3f(p4.x, p4.y, p4.z);
	glVertex3f(p3.x, p3.y, p3.z);
	glEnd();
}

void drawTriangle(Point p1, Point p2, Point p3){
	glBegin(GL_TRIANGLES);
	glVertex3f(p1.x, p1.y, p1.z);
	glVertex3f(p2.x, p2.y, p2.z);
	glVertex3f(p3.x, p3.y, p3.z);
	glEnd();
}

void subdivideTriangle(Triangle tri, BPatch patch, int depth) {
	Triangle* n1 = new Triangle;
	Triangle* n2 = new Triangle;
	Triangle* n3 = new Triangle;
	Triangle* n4 = new Triangle;

	bool edge1 = false;
	bool edge2 = false;
	bool edge3 = false;

	Point* midPoint1 = new Point;
	Point* midPoint2 = new Point;
	Point* midPoint3 = new Point;

	Point* midPara1 = new Point;
	Point* midPara2 = new Point;
	Point* midPara3 = new Point;

	Point* midReal1 = new Point;
	Point* midReal2 = new Point;
	Point* midReal3 = new Point;

	*midPoint1 = midPoint(tri.p1, tri.p2);
	*midPoint2 = midPoint(tri.p2, tri.p3);
	*midPoint3 = midPoint(tri.p1, tri.p3);

	*midPara1 = midPoint(tri.pc1, tri.pc2);
	*midPara2 = midPoint(tri.pc2, tri.pc3);
	*midPara3 = midPoint(tri.pc1, tri.pc3);

	*midReal1 = patchPoint(midPara1->x, midPara1->y,patch);
	*midReal2 = patchPoint(midPara2->x, midPara2->y,patch);
	*midReal3 = patchPoint(midPara3->x, midPara3->y,patch);

	if (distancePoint(*midReal1, *midPoint1) > stepSize) {
		edge1 = true;
	}
	if (distancePoint(*midReal2, *midPoint2) > stepSize) {
		edge2 = true;
	}
	if (distancePoint(*midReal3, *midPoint3) > stepSize) {
		edge3 = true;
	}

	if (depth > 5) {
		drawTriangle(tri.p1, tri.p2, tri.p3);
	} else if (edge1 && edge2 && edge3) {
		//New Triangle 1
		n1->p1 = tri.p1;
		n1->p2 = *midReal1;
		n1->p3 = *midReal3;

		n1->pc1 = tri.pc1;
		n1->pc2 = *midPara1;
		n1->pc3 = *midPara3;

		//New Triangle 2
		n2->p1 = *midReal1;
		n2->p2 = tri.p2;
		n2->p3 = *midReal2;

		n2->pc1 = *midPara1;
		n2->pc2 = tri.pc2;
		n2->pc3 = *midPara2;

		//New Triangle 3
		n3->p1 = *midReal3;
		n3->p2 = *midReal2;
		n3->p3 = tri.p3;

		n3->pc1 = *midPara3;
		n3->pc2 = *midPara2;
		n3->pc3 = tri.pc3;

		//New Triangle 4
		n4->p1 = *midReal1;
		n4->p2 = *midReal2;
		n4->p3 = *midReal3;

		n4->pc1 = *midPara1;
		n4->pc2 = *midPara2;
		n4->pc3 = *midPara3;

		subdivideTriangle(*n1, patch, depth+1);
		subdivideTriangle(*n2, patch, depth+1);
		subdivideTriangle(*n3, patch, depth+1);
		subdivideTriangle(*n4, patch, depth+1);

		delete midPoint1, midPoint2, midPoint3, midPara1, midPara2, midPara3, midReal1, midReal2, midReal3;
		delete n1, n2, n3, n4;
	} else if (edge1 && edge3) {
		//New Triangle 1
		n1->p1 = tri.p1;
		n1->p2 = *midReal1;
		n1->p3 = *midReal3;

		n1->pc1 = tri.pc1;
		n1->pc2 = *midPara1;
		n1->pc3 = *midPara3;

		//New Triangle 2
		n2->p1 = *midReal3;
		n2->p2 = *midReal1;
		n2->p3 = tri.p3;

		n2->pc1 = *midPara3;
		n2->pc2 = *midPara1;
		n2->pc3 = tri.pc3;

		//New Triangle 3
		n3->p1 = *midReal1;
		n3->p2 = tri.p2;
		n3->p3 = tri.p3;

		n3->pc1 = *midPara1;
		n3->pc2 = tri.pc2;
		n3->pc3 = tri.pc3;

		subdivideTriangle(*n1, patch, depth+1);
		subdivideTriangle(*n2, patch, depth+1);
		subdivideTriangle(*n3, patch, depth+1);

		delete midPoint1, midPoint2, midPoint3, midPara1, midPara2, midPara3, midReal1, midReal2, midReal3;
		delete n1, n2, n3, n4;
 	} else if (edge2 && edge3) {
		//New Triangle 1
		n1->p1 = tri.p1;
		n1->p2 = *midReal1;
		n1->p3 = tri.p3;

		n1->pc1 = tri.pc1;
		n1->pc2 = *midPara1;
		n1->pc3 = tri.pc3;

		//New Triangle 2
		n2->p1 = *midReal1;
		n2->p2 = tri.p2;
		n2->p3 = *midReal2;

		n2->pc1 = *midPara1;
		n2->pc2 = tri.pc2;
		n2->pc3 = *midPara2;

		//New Triangle 3
		n3->p1 = *midReal1;
		n3->p2 = *midReal2;
		n3->p3 = tri.p3;

		n3->pc1 = *midPara1;
		n3->pc2 = *midPara2;
		n3->pc3 = tri.pc3;

		subdivideTriangle(*n1, patch, depth+1);
		subdivideTriangle(*n2, patch, depth+1);
		subdivideTriangle(*n3, patch, depth+1);

		delete midPoint1, midPoint2, midPoint3, midPara1, midPara2, midPara3, midReal1, midReal2, midReal3;
		delete n1, n2, n3, n4;
	} else if (edge1 && edge2) {
		//New Triangle 1
		n1->p1 = tri.p1;
		n1->p2 = tri.p2;
		n1->p3 = *midReal2;

		n1->pc1 = tri.pc1;
		n1->pc2 = tri.pc2;
		n1->pc3 = *midPara2;

		//New Triangle 2
		n2->p1 = *midReal3;
		n2->p2 = *midReal2;
		n2->p3 = tri.p3;

		n2->pc1 = *midPara3;
		n2->pc2 = *midPara2;
		n2->pc3 = tri.pc3;

		//New Triangle 3
		n3->p1 = tri.p1;
		n3->p2 = *midReal2;
		n3->p3 = *midReal3;

		n3->pc1 = tri.pc1;
		n3->pc2 = *midPara2;
		n3->pc3 = *midPara3;

		subdivideTriangle(*n1, patch, depth+1);
		subdivideTriangle(*n2, patch, depth+1);
		subdivideTriangle(*n3, patch, depth+1);

		delete midPoint1, midPoint2, midPoint3, midPara1, midPara2, midPara3, midReal1, midReal2, midReal3;
		delete n1, n2, n3, n4;

	} else if (edge1) {
		//New Triangle 1
		n1->p1 = tri.p1;
		n1->p2 = tri.p2;
		n1->p3 = *midReal3;

		n1->pc1 = tri.pc1;
		n1->pc2 = tri.pc2;
		n1->pc3 = *midPara3;

		//New Triangle 2
		n2->p1 = *midReal2;
		n2->p2 = tri.p2;
		n2->p3 = tri.p3;

		n2->pc1 = *midPara2;
		n2->pc2 = tri.p2;
		n2->pc3 = tri.pc3;

		subdivideTriangle(*n1, patch, depth+1);
		subdivideTriangle(*n2, patch, depth+1);

		delete midPoint1, midPoint2, midPoint3, midPara1, midPara2, midPara3, midReal1, midReal2, midReal3;
		delete n1, n2, n3, n4;
	} else if (edge2) {
		//New Triangle 1
		n1->p1 = tri.p1;
		n1->p2 = tri.p2;
		n1->p3 = *midReal2;

		n1->pc1 = tri.pc1;
		n1->pc2 = tri.pc2;
		n1->pc3 = *midPara2;

		//New Triangle 2
		n2->p1 = tri.p1;
		n2->p2 = *midReal2;
		n2->p3 = tri.p3;

		n2->pc1 = tri.pc1;
		n2->pc2 = *midPara2;
		n2->pc3 = tri.pc3;

		subdivideTriangle(*n1, patch, depth+1);
		subdivideTriangle(*n2, patch, depth+1);

		delete midPoint1, midPoint2, midPoint3, midPara1, midPara2, midPara3, midReal1, midReal2, midReal3;
		delete n1, n2, n3, n4;
	} else if (edge3) {
		//New Triangle 1
		n1->p1 = tri.p1;
		n1->p2 = *midReal2;
		n1->p3 = tri.p3;

		n1->pc1 = tri.pc1;
		n1->pc2 = *midPara2;
		n1->pc3 = tri.pc3;

		//New Triangle 2
		n2->p1 = *midReal2;
		n2->p2 = tri.p2;
		n2->p3 = tri.p3;

		n2->pc1 = *midPara2;
		n2->pc2 = tri.pc2;
		n2->pc3 = tri.pc3;

		subdivideTriangle(*n1, patch, depth+1);
		subdivideTriangle(*n2, patch, depth+1);

		delete midPoint1, midPoint2, midPoint3, midPara1, midPara2, midPara3, midReal1, midReal2, midReal3;
		delete n1, n2, n3, n4;
	} else {
		drawTriangle(tri.p1, tri.p2, tri.p3);
		delete midPoint1, midPoint2, midPoint3, midPara1, midPara2, midPara3, midReal1, midReal2, midReal3;
		delete n1, n2, n3, n4;
	}
}

void curveTraversal(BPatch patch){

	Point p1, p2, p3, p4;

	GLfloat old_u, new_u, old_v, new_v;

	BCurve old_c, new_c;

	old_u = 0.0;

	old_c.p1 = bernstein(old_u, patch.c1);
	old_c.p2 = bernstein(old_u, patch.c2);
	old_c.p3 = bernstein(old_u, patch.c3);
	old_c.p4 = bernstein(old_u, patch.c4);

	for (GLfloat u = 0.0; u < 1.0; u += stepSize){
		new_u = old_u + stepSize;

		if (new_u > 1.0){
			new_u = 1.0;
		}

		new_c.p1 = bernstein(new_u, patch.c1);
		new_c.p2 = bernstein(new_u, patch.c2);
		new_c.p3 = bernstein(new_u, patch.c3);
		new_c.p4 = bernstein(new_u, patch.c4);

		old_v = 0.0;
		p1 = bernstein(old_v, old_c);
		p2 = bernstein(old_v, new_c);

		for (GLfloat v = 0.0; v < 1.0; v += stepSize) {
			new_v = old_v + stepSize;

			if (new_v > 1.0){
				new_v = 1.0;
			}

			p3 = bernstein(new_v, old_c);
			p4 = bernstein(new_v, new_c);

			drawPolygon(p1, p2, p3, p4);

			old_v = new_v;
		}

		old_c = new_c;
		old_u = new_u;
	}
}

void adaptiveTraversal(BPatch patch) {
	Triangle t1, t2;

	/* Triangle 1 real-world coordinates */
	t1.p1 = patch.c1.p1;
	t1.p2 = patch.c1.p4;
	t1.p3 = patch.c4.p1;

	/* Parametric Coordinates for triangle 1 (represented as point with z = 0.0) */
	t1.pc1.x = 0.0;
	t1.pc1.y = 0.0;
	t1.pc1.z = 0.0;

	t1.pc2.x = 1.0;
	t1.pc2.y = 0.0;
	t1.pc2.z = 0.0;

	t1.pc3.x = 0.0;
	t1.pc3.y = 1.0;
	t1.pc3.z = 0.0;


	/* Triangle 2 real-world coordinates */
	t2.p1 = patch.c4.p1;
	t2.p2 = patch.c1.p4;
	t2.p3 = patch.c4.p4;

	/* Parametric Coordinates for triangle 2 (represented as point with z = 0.0) */
	t2.pc1.x = 0.0;
	t2.pc1.y = 1.0;
	t2.pc1.z = 0.0;

	t2.pc2.x = 1.0;
	t2.pc2.y = 0.0;
	t2.pc2.z = 0.0;

	t2.pc3.x = 1.0;
	t2.pc3.y = 1.0;
	t2.pc3.z = 0.0;
	
	subdivideTriangle(t1, patch, 0);
	subdivideTriangle(t2, patch, 0);
	
}

void uniformTesselation(){
	for(unsigned int i = 0; i < bPatches.size(); i++){
		curveTraversal(*(bPatches.at(i)));
	}
}

void adaptiveTriangulation() {
	for(unsigned int i = 0; i < bPatches.size(); i++){
		adaptiveTraversal(*(bPatches.at(i)));
	}
}

//****************************************************
// File Parser
//****************************************************
void loadScene(std::string file) {
	numPatches = 0;
	int patchCount = 0;
	int curveCount = 0;

	GLfloat maxBoundaries = 0.0;

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

					for (int i = 0; i < 12; i++) {
						if (maxBoundaries < atof(splitline[i].c_str())) {
							maxBoundaries = atof(splitline[i].c_str());
						}
					}

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

		maxX = maxBoundaries;
		minX = -maxBoundaries;
		maxY = maxBoundaries;
		minY = -maxBoundaries;

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

	glOrtho(minX, maxX, minY, maxY, -100, 100);

	glRotatef(xRot, 1.0, 0.0, 0.0);
	glRotatef(yRot, 0.0, 1.0, 0.0);

	glTranslatef(xTran, 0.0, 0.0);
	glTranslatef(0.0, yTran, 0.0);

	//uniformTesselation();
	adaptiveTriangulation();

	glFlush();
	glutSwapBuffers();					// swap buffers (we earlier set double buffer)
}


void keyboard(unsigned char key, int x, int y) {
	switch (key)  {
	case 32: // Space key
		exit (0);
		break;
	case 43: //+ key
		maxX -= 0.2;
		minX += 0.2;
		maxY -= 0.2;
		minY += 0.2;
		break;
	case 45: // - key
		maxX += 0.2;
		minX -= 0.2;
		maxY += 0.2;
		minY -= 0.2;
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

void SpecialKeys(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_LEFT:
		if (!(glutGetModifiers() & GLUT_ACTIVE_SHIFT)) {
			xRot -= 10;
		} else {
			xTran -= 0.1;
		}
		break;
	case GLUT_KEY_RIGHT:
		if (!(glutGetModifiers() & GLUT_ACTIVE_SHIFT)) {
			xRot += 10;
		} else {
			xTran += 0.1;
		}
		break;
	case GLUT_KEY_UP:
		if (!(glutGetModifiers() & GLUT_ACTIVE_SHIFT)) {
			yRot += 10;
		} else {
			yTran += 0.1;
		}
		break;
	case GLUT_KEY_DOWN:
		if (!(glutGetModifiers() & GLUT_ACTIVE_SHIFT)) {
			yRot -= 10;
		} else {
			yTran -= 0.1;
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
	stepSize = atof(argv[2]);

	loadScene(filename);

	//This initializes glut
	glutInit(&argc, argv);

	//This tells glut to use a double-buffered window with red, green, and blue channels 
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

	// Initalize theviewport size
	viewport.w = 800;
	viewport.h = 800;

	//The size and position of the window
	glutInitWindowSize(viewport.w, viewport.h);
	glutInitWindowPosition(0,0);
	glutCreateWindow(argv[0]);

	initScene();							// quick function to set up scene

	glutDisplayFunc(myDisplay);				// function to run when its time to draw something
	glutReshapeFunc(myReshape);				// function to run when the window gets resized
	glutIdleFunc(myFrameMove);	
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(SpecialKeys);

	glutMainLoop();							// infinite loop that will keep drawing and resizing
	// and whatever else

	return 0;
}

