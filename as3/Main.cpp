
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
	Point p1, p2, p3, pc1, pc2, pc3, n1, n2, n3;
};

class Tuple {
public:
	Point p1, p2;
};


//****************************************************
// Global Variables
//****************************************************
Viewport	viewport;

GLfloat stepSize;
std::vector<BPatch*> bPatches;
int numPatches;

bool adaptive = false;

// Wired Mode or Filled Mode
bool wired = false;
bool smooth = true;

GLfloat yRot = 0.0;
GLfloat xRot = 0.0;
GLfloat xTran = 0.0;
GLfloat yTran = 0.0;

GLfloat scaleValue = 1.0;
GLfloat maxX = 10;
GLfloat maxY = 10;


//****************************************************
// reshape viewport if the window is resized
//****************************************************
void myReshape(int w, int h) {
	viewport.w = w;
	viewport.h = h;

	glViewport (0,0,viewport.w,viewport.h);
	glMatrixMode(GL_PROJECTION);

	glLoadIdentity();
	glOrtho(-maxX, maxX, -maxY, maxY, -100, 100);
	gluLookAt(0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0);
}

//****************************************************
// Simple init function
//****************************************************

GLfloat diffuseM[]={0.3, 0.3, 0.8, 1.0};
GLfloat ambientM[]={0.2, 0.2, 0.2, 1.0};
GLfloat specularM[]={1.0, 1.0, 1.0, 1.0};
GLfloat shininessM[] = {100.0};

GLfloat light0_pos[]={0.0, 0.0, 10.0, 1.0};

void initScene(){
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear to black, fully transparent
	myReshape(viewport.w,viewport.h);

	glEnable(GL_NORMALIZE);
	glEnable(GL_DEPTH_TEST);

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambientM);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuseM);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularM);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininessM);

	//Enable Light Source Number Zero
	glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
}

//*********************************************w
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
	GLfloat len = sqrt(pow(p.x, 2.0) + pow(p.y, 2.0) + pow(p.z, 2.0));
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
	return crossProduct(subtractPoint(p3, p1), subtractPoint(p2, p1));
}

Point midPoint(Point p1, Point p2) {
	Point r;
	r.x = (p1.x + p2.x)/2.0;
	r.y = (p1.y + p2.y)/2.0;
	r.z = (p1.z + p2.z)/2.0;
	return r;
}

GLfloat distancePoint(Point p1, Point p2) {
	return sqrt(pow(p1.x-p2.x, 2.0) + pow(p1.y-p2.y, 2.0) + pow(p1.z-p2.z, 2.0));
}

Tuple bernstein(GLfloat u, BCurve curve){
	Point a, b, c, d, e, p, pd;
	Tuple output;

	a = addPoint(multiplyPoint((1.0-u), curve.p1), multiplyPoint(u, curve.p2));
	b = addPoint(multiplyPoint((1.0-u), curve.p2), multiplyPoint(u, curve.p3));
	c = addPoint(multiplyPoint((1.0-u), curve.p3), multiplyPoint(u, curve.p4));

	d = addPoint(multiplyPoint((1.0-u), a), multiplyPoint(u, b));
	e = addPoint(multiplyPoint((1.0-u), b), multiplyPoint(u, c));

	p = addPoint(multiplyPoint((1.0-u), d), multiplyPoint(u, e));
	pd = multiplyPoint(3.0, subtractPoint(e, d));

	output.p1 = p;
	output.p2 = pd;
	return output;
}

Tuple patchPoint(GLfloat u, GLfloat v, BPatch patch) {
	BCurve vcurve, ucurve;
	Point p, dPdv, dPdu, n;

	BCurve hc1, hc2, hc3, hc4;

	hc1.p1 = patch.c1.p1;
	hc1.p2 = patch.c2.p1;
	hc1.p3 = patch.c3.p1;
	hc1.p4 = patch.c4.p1;

	hc2.p1 = patch.c1.p2;
	hc2.p2 = patch.c2.p2;
	hc2.p3 = patch.c3.p2;
	hc2.p4 = patch.c4.p2;

	hc3.p1 = patch.c1.p3;
	hc3.p2 = patch.c2.p3;
	hc3.p3 = patch.c3.p3;
	hc3.p4 = patch.c4.p3;

	hc4.p1 = patch.c1.p4;
	hc4.p2 = patch.c2.p4;
	hc4.p3 = patch.c3.p4;
	hc4.p4 = patch.c4.p4;

	vcurve.p1 = bernstein(u, patch.c1).p1;
	vcurve.p2 = bernstein(u, patch.c2).p1;
	vcurve.p3 = bernstein(u, patch.c3).p1;
	vcurve.p4 = bernstein(u, patch.c4).p1;

	ucurve.p1 = bernstein(v, hc1).p1;
	ucurve.p2 = bernstein(v, hc2).p1;
	ucurve.p3 = bernstein(v, hc3).p1;
	ucurve.p4 = bernstein(v, hc4).p1;

	Tuple temp1 = bernstein(v, vcurve);
	Tuple temp2 = bernstein(u, ucurve);
	p = temp1.p1;
	dPdv = temp1.p2;
	dPdu = temp2.p2;

	n = crossProduct(dPdu, dPdv);
	Tuple output;
	output.p1 = p;
	output.p2 = n;
	return output;
}

void drawPolygon(Tuple t1, Tuple t2, Tuple t3, Tuple t4){
	glBegin(GL_POLYGON);
	glNormal3f(t1.p2.x, t1.p2.y, t1.p2.z);
	glVertex3f(t1.p1.x, t1.p1.y, t1.p1.z);

	glNormal3f(t2.p2.x, t2.p2.y, t2.p2.z);
	glVertex3f(t2.p1.x, t2.p1.y, t2.p1.z);

	glNormal3f(t4.p2.x, t4.p2.y, t4.p2.z);
	glVertex3f(t4.p1.x, t4.p1.y, t4.p1.z);

	glNormal3f(t3.p2.x, t3.p2.y, t3.p2.z);
	glVertex3f(t3.p1.x, t3.p1.y, t3.p1.z);
	glEnd();
}

void drawTriangle(Triangle tri){
	glBegin(GL_POLYGON);
	glNormal3f(tri.n1.x, tri.n1.y, tri.n1.z);
	glVertex3f(tri.p1.x, tri.p1.y, tri.p1.z);

	glNormal3f(tri.n2.x, tri.n2.y, tri.n2.z);
	glVertex3f(tri.p2.x, tri.p2.y, tri.p2.z);

	glNormal3f(tri.n3.x, tri.n3.y, tri.n3.z);
	glVertex3f(tri.p3.x, tri.p3.y, tri.p3.z);
	glEnd();
}

void subdivideTriangle(Triangle tri, BPatch patch) {
	Triangle* t1 = new Triangle;
	Triangle* t2 = new Triangle;
	Triangle* t3 = new Triangle;
	Triangle* t4 = new Triangle;

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

	Point* midNorm1 = new Point;
	Point* midNorm2 = new Point;
	Point* midNorm3 = new Point;

	*midPoint1 = midPoint(tri.p1, tri.p2);
	*midPoint2 = midPoint(tri.p2, tri.p3);
	*midPoint3 = midPoint(tri.p1, tri.p3);

	*midPara1 = midPoint(tri.pc1, tri.pc2);
	*midPara2 = midPoint(tri.pc2, tri.pc3);
	*midPara3 = midPoint(tri.pc1, tri.pc3);

	Tuple temp1 = patchPoint(midPara1->x, midPara1->y, patch);
	Tuple temp2 = patchPoint(midPara2->x, midPara2->y, patch);
	Tuple temp3 = patchPoint(midPara3->x, midPara3->y, patch);

	*midReal1 = temp1.p1;
	*midReal2 = temp2.p1;
	*midReal3 = temp3.p1;

	*midNorm1 = temp1.p2;	
	*midNorm2 = temp2.p2;
	*midNorm3 = temp3.p2;

	if (distancePoint(*midReal1, *midPoint1) > stepSize) {
		edge1 = true;
	}
	if (distancePoint(*midReal2, *midPoint2) > stepSize) {
		edge2 = true;
	}
	if (distancePoint(*midReal3, *midPoint3) > stepSize) {
		edge3 = true;
	}

	if (edge1 && edge2 && edge3) {
		//New Triangle 1
		t1->p1 = tri.p1;
		t1->p2 = *midReal1;
		t1->p3 = *midReal3;

		t1->pc1 = tri.pc1;
		t1->pc2 = *midPara1;
		t1->pc3 = *midPara3;

		t1->n1 = tri.n1;
		t1->n2 = *midNorm1;
		t1->n3 = *midNorm3;

		//New Triangle 2
		t2->p1 = *midReal1;
		t2->p2 = tri.p2;
		t2->p3 = *midReal2;

		t2->pc1 = *midPara1;
		t2->pc2 = tri.pc2;
		t2->pc3 = *midPara2;

		t2->n1 = *midNorm1;
		t2->n2 = tri.n2;
		t2->n3 = *midNorm2;

		//New Triangle 3
		t3->p1 = *midReal3;
		t3->p2 = *midReal2;
		t3->p3 = tri.p3;

		t3->pc1 = *midPara3;
		t3->pc2 = *midPara2;
		t3->pc3 = tri.pc3;

		t3->n1 = *midNorm3;
		t3->n2 = *midNorm2;
		t3->n3 = tri.n3;

		//New Triangle 4
		t4->p1 = *midReal1;
		t4->p2 = *midReal2;
		t4->p3 = *midReal3;

		t4->pc1 = *midPara1;
		t4->pc2 = *midPara2;
		t4->pc3 = *midPara3;

		t4->n1 = *midNorm1;
		t4->n2 = *midNorm2;
		t4->n3 = *midNorm3;

		subdivideTriangle(*t1, patch);
		subdivideTriangle(*t2, patch);
		subdivideTriangle(*t3, patch);
		subdivideTriangle(*t4, patch);

	} else if (edge1 && edge3) {
		//New Triangle 1
		t1->p1 = tri.p1;
		t1->p2 = *midReal1;
		t1->p3 = *midReal3;

		t1->pc1 = tri.pc1;
		t1->pc2 = *midPara1;
		t1->pc3 = *midPara3;

		t1->n1 = tri.n1;
		t1->n2 = *midNorm1;
		t1->n3 = *midNorm3;

		//New Triangle 2
		t2->p1 = *midReal3;
		t2->p2 = *midReal1;
		t2->p3 = tri.p3;

		t2->pc1 = *midPara3;
		t2->pc2 = *midPara1;
		t2->pc3 = tri.pc3;

		t2->n1 = *midNorm3;
		t2->n2 = *midNorm1;
		t2->n3 = tri.n3;

		//New Triangle 3
		t3->p1 = *midReal1;
		t3->p2 = tri.p2;
		t3->p3 = tri.p3;

		t3->pc1 = *midPara1;
		t3->pc2 = tri.pc2;
		t3->pc3 = tri.pc3;

		t3->n1 = *midNorm1;
		t3->n2 = tri.n2;
		t3->n3 = tri.n3;

		subdivideTriangle(*t1, patch);
		subdivideTriangle(*t2, patch);
		subdivideTriangle(*t3, patch);

	} else if (edge2 && edge3) {
		//New Triangle 1
		t1->p1 = tri.p1;
		t1->p2 = tri.p2;
		t1->p3 = *midReal3;

		t1->pc1 = tri.pc1;
		t1->pc2 = tri.pc2;
		t1->pc3 = *midPara3;

		t1->n1 = tri.n1;
		t1->n2 = tri.n2;
		t1->n3 = *midNorm3;

		//New Triangle 2
		t2->p1 = tri.p2;
		t2->p2 = *midReal2;
		t2->p3 = *midReal3;

		t2->pc1 = tri.pc2;
		t2->pc2 = *midPara2;
		t2->pc3 = *midPara3;

		t2->n1 = tri.n2;
		t2->n2 = *midNorm2;
		t2->n3 = *midNorm3;

		//New Triangle 3
		t3->p1 = *midReal3;
		t3->p2 = *midReal2;
		t3->p3 = tri.p3;

		t3->pc1 = *midPara3;
		t3->pc2 = *midPara2;
		t3->pc3 = tri.pc3;

		t3->n1 = *midNorm3;
		t3->n2 = *midNorm2;
		t3->n3 = tri.n3;

		subdivideTriangle(*t1, patch);
		subdivideTriangle(*t2, patch);
		subdivideTriangle(*t3, patch);

	} else if (edge1 && edge2) {
		//New Triangle 1
		t1->p1 = tri.p1;
		t1->p2 = *midReal1;
		t1->p3 = *midReal2;

		t1->pc1 = tri.pc1;
		t1->pc2 = *midPara1;
		t1->pc3 = *midPara2;

		t1->n1 = tri.n1;
		t1->n2 = *midNorm1;
		t1->n3 = *midNorm2;

		//New Triangle 2
		t2->p1 = *midReal1;
		t2->p2 = tri.p2;
		t2->p3 = *midReal2;

		t2->pc1 = *midPara1;
		t2->pc2 = tri.pc2;
		t2->pc3 = *midPara2;

		t2->n1 = *midNorm1;
		t2->n2 = tri.n2;
		t2->n3 = *midNorm2;

		//New Triangle 3
		t3->p1 = tri.p1;
		t3->p2 = *midReal2;
		t3->p3 = tri.p3;

		t3->pc1 = tri.pc1;
		t3->pc2 = *midPara2;
		t3->pc3 = tri.pc3;

		t3->n1 = tri.n1;
		t3->n2 = *midNorm2;
		t3->n3 = tri.n3;

		subdivideTriangle(*t1, patch);
		subdivideTriangle(*t2, patch);
		subdivideTriangle(*t3, patch);

	} else if (edge1) {
		//New Triangle 1
		t1->p1 = tri.p1;
		t1->p2 = *midReal1;
		t1->p3 = tri.p3;

		t1->pc1 = tri.pc1;
		t1->pc2 = *midPara1;
		t1->pc3 = tri.pc3;

		t1->n1 = tri.n1;
		t1->n2 = *midNorm1;
		t1->n3 = tri.n3;

		//New Triangle 2
		t2->p1 = *midReal1;
		t2->p2 = tri.p2;
		t2->p3 = tri.p3;

		t2->pc1 = *midPara1;
		t2->pc2 = tri.pc2;
		t2->pc3 = tri.pc3;

		t2->n1 = *midNorm1;
		t2->n2 = tri.n2;
		t2->n3 = tri.n3;

		subdivideTriangle(*t1, patch);
		subdivideTriangle(*t2, patch);

	} else if (edge2) {
		//New Triangle 1
		t1->p1 = tri.p1;
		t1->p2 = tri.p2;
		t1->p3 = *midReal2;

		t1->pc1 = tri.pc1;
		t1->pc2 = tri.pc2;
		t1->pc3 = *midPara2;

		t1->n1 = tri.n1;
		t1->n2 = tri.n2;
		t1->n3 = *midNorm2;

		//New Triangle 2
		t2->p1 = tri.p1;
		t2->p2 = *midReal2;
		t2->p3 = tri.p3;

		t2->pc1 = tri.pc1;
		t2->pc2 = *midPara2;
		t2->pc3 = tri.pc3;

		t2->n1 = tri.n1;
		t2->n2 = *midNorm2;
		t2->n3 = tri.n3;

		subdivideTriangle(*t1, patch);
		subdivideTriangle(*t2, patch);

	} else if (edge3) {
		//New Triangle 1
		t1->p1 = tri.p1;
		t1->p2 = tri.p2;
		t1->p3 = *midReal3;

		t1->pc1 = tri.pc1;
		t1->pc2 = tri.pc2;
		t1->pc3 = *midPara3; 

		t1->n1 = tri.n1;
		t1->n2 = tri.n2;
		t1->n3 = *midNorm3;

		//New Triangle 2
		t2->p1 = *midReal3;
		t2->p2 = tri.p2;
		t2->p3 = tri.p3;

		t2->pc1 = *midPara3;
		t2->pc2 = tri.pc2;
		t2->pc3 = tri.pc3;

		t2->n1 = *midNorm3;
		t2->n2 = tri.n2;
		t2->n3 = tri.n3;

		subdivideTriangle(*t1, patch);
		subdivideTriangle(*t2, patch);

	} else {
		drawTriangle(tri);
	}

	delete midPoint1;
	delete midPoint2;
	delete midPoint3;

	delete midPara1;
	delete midPara2;
	delete midPara3;

	delete midReal1;
	delete midReal2;
	delete midReal3;

	delete midNorm1;
	delete midNorm2;
	delete midNorm3;

	delete t1;
	delete t2;
	delete t3;
	delete t4;
}

void curveTraversal(BPatch patch){
	Tuple t1, t2, t3, t4;

	GLfloat old_u, new_u, old_v, new_v;
	old_u = 0.0;
	for (GLfloat u = 0.0; u < 1.0; u += stepSize) {
		new_u = old_u + stepSize;
		if (new_u > 1.0){
			new_u = 1.0;
		}
		old_v = 0.0;
		for (GLfloat v = 0.0; v < 1.0; v += stepSize) {
			new_v = old_v + stepSize;
			if (new_v > 1.0){
				new_v = 1.0;
			}
			t1 = patchPoint(old_u, old_v, patch);
			t2 = patchPoint(new_u, old_v, patch);
			t3 = patchPoint(old_u, new_v, patch);
			t4 = patchPoint(new_u, new_v, patch);

			drawPolygon(t1, t2, t3, t4);
			old_v = new_v;
		}
		old_u = new_u;
	}	
}

void adaptiveTraversal(BPatch patch) {
	Triangle* t1 = new Triangle;
	Triangle* t2 = new Triangle;

	/* Triangle 1 real-world coordinates */
	t1->p1 = patch.c1.p1;
	t1->p2 = patch.c1.p4;
	t1->p3 = patch.c4.p1;

	/* Parametric Coordinates for triangle 1 (represented as point with z = 0.0) */
	t1->pc1.x = 0.0;
	t1->pc1.y = 0.0;
	t1->pc1.z = 0.0;

	t1->pc2.x = 1.0;
	t1->pc2.y = 0.0;
	t1->pc2.z = 0.0;

	t1->pc3.x = 0.0;
	t1->pc3.y = 1.0;
	t1->pc3.z = 0.0;

	t1->n1 = patchPoint(t1->pc1.x, t1->pc1.y, patch).p2;
	t1->n2 = patchPoint(t1->pc2.x, t1->pc2.y, patch).p2;
	t1->n3 = patchPoint(t1->pc3.x, t1->pc3.y, patch).p2;


	/* Triangle 2 real-world coordinates */
	t2->p1 = patch.c4.p1;
	t2->p2 = patch.c1.p4;
	t2->p3 = patch.c4.p4;

	/* Parametric Coordinates for triangle 2 (represented as point with z = 0.0) */
	t2->pc1.x = 0.0;
	t2->pc1.y = 1.0;
	t2->pc1.z = 0.0;

	t2->pc2.x = 1.0;
	t2->pc2.y = 0.0;
	t2->pc2.z = 0.0;

	t2->pc3.x = 1.0;
	t2->pc3.y = 1.0;
	t2->pc3.z = 0.0;

	t2->n1 = patchPoint(t2->pc1.x, t2->pc1.y, patch).p2;
	t2->n2 = patchPoint(t2->pc2.x, t2->pc2.y, patch).p2;
	t2->n3 = patchPoint(t2->pc3.x, t2->pc3.y, patch).p2;

	subdivideTriangle(*t1, patch);
	subdivideTriangle(*t2, patch);
	delete t1, t2;
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
		inpfile.close();
	}
	maxX = maxY = maxBoundaries;
}


//****************************************************
// function that does the actual drawing of stuff
//***************************************************
void myDisplay() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// clear the color buffer

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glScalef(scaleValue, scaleValue, scaleValue);

	glTranslatef(xTran, 0.0, 0.0);
	glTranslatef(0.0, yTran, 0.0);

	glRotatef(xRot, 1.0, 0.0, 0.0);
	glRotatef(yRot, 0.0, 1.0, 0.0);

	if (adaptive) {
		adaptiveTriangulation();
	} else {
		uniformTesselation();
	}

	glFlush();
	glutSwapBuffers();					// swap buffers (we earlier set double buffer)
}


void keyboard(unsigned char key, int x, int y) {
	switch (key)  {
	case 32: // Space key
		exit (0);
		break;
	case 61: //+ key
		scaleValue += 0.1;
		break;
	case 45: // - key
		scaleValue -= 0.1;
		break;
	case 115: //s key
		if (smooth){
			glShadeModel(GL_FLAT);
			smooth = false;
		}else{
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
			yRot += 10;
		} else {
			xTran -= 0.5;
		}
		break;
	case GLUT_KEY_RIGHT:
		if (!(glutGetModifiers() & GLUT_ACTIVE_SHIFT)) {
			yRot -= 10;
		} else {
			xTran += 0.5;
		}
		break;
	case GLUT_KEY_UP:
		if (!(glutGetModifiers() & GLUT_ACTIVE_SHIFT)) {
			xRot -= 10;
		} else {
			yTran += 0.5;
		}
		break;
	case GLUT_KEY_DOWN:
		if (!(glutGetModifiers() & GLUT_ACTIVE_SHIFT)) {
			xRot += 10;
		} else {
			yTran -= 0.5;
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

	if (argc > 3) {
		adaptive = true;
	}

	std::string filename = argv[1];
	stepSize = atof(argv[2]);
	loadScene(filename);

	//This initializes glut
	glutInit(&argc, argv);

	//This tells glut to use a double-buffered window with red, green, and blue channels 
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	// Initalize theviewport size
	viewport.w = 1000;
	viewport.h = 1000;

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

