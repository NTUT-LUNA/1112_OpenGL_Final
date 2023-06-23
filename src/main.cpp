#include <iostream>
#include <math.h>
#include <stdlib.h>
// gl tools
#include "gltools.h" // OpenGL toolkit
#include "math3d.h"  // 3D Math Library
#include "glframe.h" // Frame class
// opencv
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
// obj reader
#include "ObjParser.h"

typedef unsigned char uchar;

void SetupRC();
void ShutdownRC(void);
void DrawGround(void);
void DrawInhabitants(GLint);
void DisplayFunc(void);
void SpecialFunc(int, int, int);
void IdleFunc(void);
void TimerFunc(int);
void ReshapeFunc(int, int);

#define	NUM_BARRELS      36
GLFrame frameCamera, barrels[NUM_BARRELS], fishes[(NUM_BARRELS - 6) * 2];

// Light and material data
M3DMatrix44f mShadowMatrix;

GLfloat fLightPos[4] = { -100.0f, 100.0f, 50.0f, 1.0f };  // Point source
GLfloat fNoLight[] = { 0.0f, 0.0f, 0.0f, 0.0f };
GLfloat fLowLight[] = { 0.25f, 0.25f, 0.25f, 1.0f };
GLfloat fMidLight[] = { 0.5f, 0.5f, 0.5f, 1.0f };
GLfloat fHighLight[] = { 0.75f, 0.75f, 0.75f, 1.0f };
GLfloat fBrightLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };

GLfloat fBackground[] = { 0.f, 0.5f, 0.75f, 1.0f }; // grey-blue background

#define GROUND_TEXTURE  3
#define TORUS_TEXTURE   1
#define SPHERE_TEXTURE  2
#define NUM_TEXTURES    3

#define DOLPHIN_TEXTURE 4
#define BARREL_TEXTURE  1
#define FISH_TEXTURE	5

#define EXTRA_TEXTURES  3
#define TOTAL_TEXTURES  NUM_TEXTURES + EXTRA_TEXTURES

GLuint textures[TOTAL_TEXTURES];
const char* szTextureFiles[] = { "./tga/grass.tga", "./tga/wood.tga", "./tga/orb.tga"};

ObjParser* dolphin;
ObjParser* seaweed;
ObjParser* barrel;
ObjParser* fish;

// This function does any needed initialization on the rendering context
void SetupRC()
{
    int i, iBarrel;
	Vec3f offset;
	GLbyte* pBytes;
	GLint iWidth, iHeight, iComponents;
	GLenum eFormat;
	M3DVector4f pPlane;
	M3DVector3f vPoints[3] = {
		{ 0.0f, -0.4f, 0.0f },
        { 10.0f, -0.4f, 0.0f },
        { 5.0f, -0.4f, -5.0f } 
	};

	// Read objs
	std::cout << "dolphin" << std::endl;
	dolphin = new ObjParser("./obj/dolphin.obj");

	std::cout << "seaweed" << std::endl;
	seaweed = new ObjParser("./obj/seaweed.obj");

	std::cout << "barrel" << std::endl;
	barrel = new ObjParser("./obj/barrel.obj");

	std::cout << "fish" << std::endl;
	fish = new ObjParser("./obj/fish.obj");

    // background color
    glClearColor(fBackground[0], fBackground[1], fBackground[2], fBackground[3]);

    // Clear stencil buffer with zero, increment by one whenever anybody draws into it. 
	// When stencil function is enabled, only write where stencil value is zero. 
	// This prevents the transparent shadow from drawing over itself
    glStencilOp(GL_INCR, GL_INCR, GL_INCR);
    glClearStencil(0);
    glStencilFunc(GL_EQUAL, 0x0, 0x01);

    // Cull backs of polygons
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE_ARB);

    // Setup light parameters
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, fNoLight);
    glLightfv(GL_LIGHT0, GL_AMBIENT, fLowLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, fBrightLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, fBrightLight);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Calculate shadow matrix
    m3dGetPlaneEquation(pPlane, vPoints[0], vPoints[1], vPoints[2]);
    m3dMakePlanarShadowMatrix(mShadowMatrix, pPlane, fLightPos);

    // Mostly use material tracking
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT, GL_SPECULAR, fBrightLight);
    glMateriali(GL_FRONT, GL_SHININESS, 128);

    // Randomly place the barrels and fishes
	for (iBarrel = 0; iBarrel < NUM_BARRELS; iBarrel++)
	{
		float x, y;
		x = ((float)((rand() % 400) - 200) * 0.1f);
		y = ((float)((rand() % 400) - 200) * 0.1f);
		// Pick a random location between -20 and 20 at .1 increments
		barrels[iBarrel].SetOrigin(x, 0.0, y);
		fishes[iBarrel * 2].SetOrigin(x, 0.0, y); // 0, 2, 4, 6, 8
		fishes[iBarrel * 2 + 1].SetOrigin(x, 0.0, y); // 1, 3, 5, 7, 9
	}
	// direction indicators
	barrels[30].SetOrigin( 0.0,  0.5,  0.0); // origin
	barrels[31].SetOrigin(-1.0,  0.0, -1.0); // top left
	barrels[32].SetOrigin( 1.0,  0.0, -1.0); // top right
	barrels[33].SetOrigin(-1.0,  0.0,  1.0); // bottom right
	barrels[34].SetOrigin( 1.0,  0.0,  1.0); // bottom left
	barrels[35].SetOrigin( 0.0,  0.0, -0.5); // front

    // Set up texture maps
    glEnable(GL_TEXTURE_2D);
    glGenTextures(TOTAL_TEXTURES, textures); // 註冊一個大小為NUM_TEXTURES的陣列讓openGL儲存材質，名稱為textures
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // 設定openGL材質紋理的參數和材質的組合模式

    for (i = 0; i < NUM_TEXTURES; i++)
    {
        glBindTexture(GL_TEXTURE_2D, textures[i]);

        // Load this texture map
        pBytes = gltLoadTGA(szTextureFiles[i], &iWidth, &iHeight, &iComponents, &eFormat);
        gluBuild2DMipmaps(GL_TEXTURE_2D, iComponents, iWidth, iHeight, eFormat, GL_UNSIGNED_BYTE, pBytes);
        free(pBytes);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

	// Load the texture objects
	cv::Mat seaImage = cv::imread("./texture/sea.jpg"); // 利用openCV讀取圖片檔案
	if (seaImage.empty()) {
		std::cout << "Sea floor empty\n";
	}
	else {
		// 將讀取進來的圖片檔案當作材質存進textures中
		cv::flip(seaImage, seaImage, 0);
		//glGenTextures(1, &textures[3]);
		glBindTexture(GL_TEXTURE_2D, textures[3]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, seaImage.cols, seaImage.rows, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, seaImage.ptr());
	}

	cv::Mat fishImage_1 = cv::imread("./texture/fish1.png");
	if (fishImage_1.empty()) {
		std::cout << "Fish1 skin empty\n";
	}
	else {
		// 將讀取進來的圖片檔案當作材質存進textures中
		cv::flip(fishImage_1, fishImage_1, 0);
		glBindTexture(GL_TEXTURE_2D, textures[4]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fishImage_1.cols, fishImage_1.rows, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, fishImage_1.ptr());
	}

	cv::Mat fishImage_2 = cv::imread("./texture/black_rect.jpg");
	if (fishImage_2.empty()) {
		std::cout << "Fish2 skin empty\n";
	}
	else {
		// 將讀取進來的圖片檔案當作材質存進textures中
		cv::flip(fishImage_2, fishImage_2, 0);
		glBindTexture(GL_TEXTURE_2D, textures[5]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fishImage_2.cols, fishImage_2.rows, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, fishImage_2.ptr());
	}
}

// Do shutdown for the rendering context
void ShutdownRC(void)
{
	glDeleteTextures(TOTAL_TEXTURES, textures); // Delete the textures
}

// Draw the ground as a series of triangle strips
void DrawGround(void)
{
	GLfloat iStrip, iRun;
	GLfloat fExtent = 20.0f;
	GLfloat fStep = 1.0f;
	GLfloat y = -0.4f;
	GLfloat s = 0.0f;
	GLfloat t = 0.0f;
	GLfloat texStep = 1.0f / (fExtent * .075f);

	glBindTexture(GL_TEXTURE_2D, textures[GROUND_TEXTURE]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	for (iStrip = -fExtent; iStrip <= fExtent; iStrip += fStep)
	{
		t = 0.0f;

		glBegin(GL_TRIANGLE_STRIP);
		for (iRun = fExtent; iRun >= -fExtent; iRun -= fStep)
		{
			glTexCoord2f(s, t);
			glNormal3f(0.0f, 1.0f, 0.0f); // All Point up
			glVertex3f(iStrip, y, iRun);

			glTexCoord2f(s + texStep, t);
			glNormal3f(0.0f, 1.0f, 0.0f); // All Point up
			glVertex3f(iStrip + fStep, y, iRun);

			t += texStep;
		}
		glEnd();

		s += texStep;
	}
}
//void DrawCustom(GLint nShadow)
//{
//	float ratio;
//	GLint i;
//	static GLfloat yRot = 0.0f; // Rotation angle for animation
//
//	if (nShadow == 0)
//	{
//		yRot += 0.5f;
//		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
//	}
//	else // Shadow color
//	{
//		glColor4f(0.00f, 0.00f, 0.00f, .6f);
//	}
//	// Draw the randomly located barrels
//	glBindTexture(GL_TEXTURE_2D, textures[BARREL_TEXTURE]);
//	for (i = 0; i < NUM_BARRELS; i++)
//	{
//		ratio = 0.05f;
//
//		glPushMatrix();
//		{
//			barrels[i].ApplyActorTransform();
//			glTranslatef(0.f, -0.4f, 0.f);
//			glScalef(ratio, ratio, ratio);
//			barrel->Draw(GL_TRIANGLES, true);
//		}
//		glPopMatrix();
//	}
//	// Draw the dolphin swim around seaweed (Object_B)
//	glPushMatrix();
//	{
//		glTranslatef(0.0f, 0.1f, -2.5f);
//		glRotatef(yRot, 0.0f, 1.0f, 0.0f);
//		glTranslatef(1.0f, 0.0f, 0.0f);
//
//		ratio = 0.005f;
//		glScalef(ratio, ratio, ratio);
//
//		glBindTexture(GL_TEXTURE_2D, textures[DOLPHIN_TEXTURE]);
//		dolphin->Draw(GL_TRIANGLES, true);
//	}
//	glPopMatrix();
//	// Draw the seaweed (Object_A)
//	glPushMatrix(); // no texture for this obj
//	{
//		glTranslatef(-0.1f, -0.43f, 2.3f);
//		//glRotatef(yRot, 0.0f, 1.0f, 0.0f);
//		//glTranslatef(0.f, 0.f, -2.5f);
//		//glPushMatrix();
//		//glPopMatrix();
//		//glTranslatef(-1.f, -4.3f, 23.f); // to origin
//		//glRotatef(yRot, 0.0f, 1.0f, 0.0f); // rotate along y-axis
//		//glTranslatef(0.f, 0.f, -20.f); // to dest
//
//		ratio = 0.1f;
//		glScalef(ratio, ratio, ratio);
//
//		if (nShadow == 0)
//		{
//			glColorMaterial(GL_FRONT, GL_SPECULAR);
//			glMaterialfv(GL_FRONT, GL_SPECULAR, fNoLight);
//			glColor4f(0.f, 1.f, 0.f, 1.f); // set seaweed to green
//		}
//		seaweed->Draw(GL_TRIANGLES, false);
//	}
//	glPopMatrix();
//}
void DrawCustom(GLint nShadow)
{
	float ratio;
	GLint i;
	static GLfloat yRot = 0.0f; // Rotation angle for animation
	// fish cos-wave
	GLfloat fCosWave;
	static int factor = 1; // need to be static since it flips
	static int cosWave = 0; // move portion
	static int counter = 0;
	// dolphin moves
	static GLfloat dx = 0.0f;
	static GLfloat dy = 0.0f;
	static GLfloat dz = 0.0f;
	static GLfloat dRot = 0.0f;
	static bool stop = false;
	static int moveCounter = 0;

	if (nShadow == 0)
	{
		yRot += 0.5f;
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else // Shadow color
	{
		glColor4f(0.00f, 0.00f, 0.00f, .6f);
	}

	// Draw the randomly located barrels (Object_A) and fishes (Object_B)
	for (i = 0; i < NUM_BARRELS; i++)
	{
		ratio = 0.05f;
	
		glBindTexture(GL_TEXTURE_2D, textures[BARREL_TEXTURE]);
		glPushMatrix(); // barrel
		{
			barrels[i].ApplyActorTransform();
			glScalef(ratio, ratio, ratio);
			glRotatef(-yRot * 2, 0.0f, 1.0f, 0.0f);
			glTranslatef(0.f, -8.f, 0.f);
			barrel->Draw(GL_TRIANGLES, true);
		}
		glPopMatrix();

		if (i >= NUM_BARRELS - 6) { continue; } // indicators wont have fishes

		ratio = 0.04f;
		fCosWave = ((float)cosWave) / 10.0f;

		glBindTexture(GL_TEXTURE_2D, textures[FISH_TEXTURE]);
		glPushMatrix(); // fish 1
		{
			fishes[i * 2].ApplyActorTransform();
			glScalef(ratio, ratio, ratio);
			glRotatef(-yRot * 1.2, 0.0f, 1.0f, 0.0f);
			glTranslatef(10.f - fCosWave, -2.f + fCosWave, 0.f); // higher, outer
			fish->Draw(GL_TRIANGLES, true);
		}
		glPopMatrix();

		glPushMatrix(); // fish 2
		{
			fishes[i * 2 + 1].ApplyActorTransform();
			glScalef(ratio, ratio, ratio);
			glRotatef(-yRot * 1.5, 0.0f, 1.0f, 0.0f);
			glTranslatef(8.f - fCosWave, -5.f + fCosWave, 3.f); // lower, insider
			fish->Draw(GL_TRIANGLES, true);
		}
		glPopMatrix();
	}

	// Draw the dolphin (Object_C) swim around seaweed
	glPushMatrix();
	{
		ratio = 0.005f;
		glScalef(ratio, ratio, ratio);

		glTranslatef(0.0f, 20.f, -500.f);
		glRotatef((yRot - dRot) * 2, 0.0f, 1.0f, 0.0f);
		glTranslatef(200.f + dx, 0.f + dy, 0.f + dz);

		glBindTexture(GL_TEXTURE_2D, textures[DOLPHIN_TEXTURE]);
		dolphin->Draw(GL_TRIANGLES, true);
	}
	glPopMatrix();

	// Draw the seaweed
	glPushMatrix(); // no texture for this obj
	{
		ratio = 0.1f;
		glScalef(ratio, ratio, ratio);

		glTranslatef(-1.f, -4.3f, 0.f);
		
		//glTranslatef(-1.f, -4.3f, 23.f); // to origin
		//glRotatef(yRot, 0.0f, 1.0f, 0.0f); // rotate along y-axis
		//glTranslatef(0.f, 0.f, -20.f); // to dest

		if (nShadow == 0) 
		{
			glColorMaterial(GL_FRONT, GL_SPECULAR);
			glMaterialfv(GL_FRONT, GL_SPECULAR, fNoLight);
			glColor4f(0.f, 1.f, 0.f, 1.f); // set seaweed to green
		}
		seaweed->Draw(GL_TRIANGLES, false);
	}
	glPopMatrix();

	if (nShadow != 0) { return; }
	if (stop) { dRot += 0.5f; }
	
	if (counter++ >= 5)
	{
		// reset counter
		counter = 0;
		// update swim counter
		moveCounter++;
		// update fishes cos-wave
		factor = (cosWave >= 10 || cosWave <= -10) ? -factor : factor;
		cosWave += factor;
		cosWave = std::min(10, std::max(-10, cosWave));
		fCosWave = ((float)cosWave) / 10.0f;
		//std::cout << "cosWave: " << cosWave << " factor: " << factor << std::endl; // debug
		//std::cout << "fCosWave: " << fCosWave << std::endl; // debug
	}

	if (moveCounter >= 20)
	{
		// reset swim counter
		moveCounter = 0;
		// check if dolphin need to stop
		stop = (rand() % 2 == 1);
		std::cout << "stop: " << stop << " dRot: " << dRot << std::endl;

		if (!stop)
		{
			dRot = 0.0f; // reset dRot for next time
			GLfloat threshold = 20.0f;
			dx += ((float)(rand() % 3) - 1.0f) * 2; // 0, 1, 2 --> -1, 0, 1
			dy += ((float)(rand() % 3) - 1.0f) * 2;
			dz += ((float)(rand() % 3) - 1.0f) * 2;
			dx = std::min(threshold, std::max(-threshold, dx));
			dy = std::min(threshold, std::max(-threshold, dy));
			dz = std::min(threshold, std::max(-threshold, dz));
			//std::cout << "dx: " << dx << " dy: " << dy << " dz: " << dz << std::endl;
		}
	}
}

// Called to draw scene
void DisplayFunc(void)
{
	// Clear the window with current clearing color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glShadeModel(GL_SMOOTH);

	glPushMatrix();
	{
		frameCamera.ApplyCameraTransform();

		// Position light before any other transformations
		glLightfv(GL_LIGHT0, GL_POSITION, fLightPos);

		// Draw the ground
		glColor3f(1.0f, 1.0f, 1.0f);
		DrawGround();

		// Draw shadows first
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_STENCIL_TEST);

		glPushMatrix();
		{
			glMultMatrixf(mShadowMatrix);
			//DrawInhabitants(1);
			DrawCustom(1);
		}
		glPopMatrix();

		glDisable(GL_STENCIL_TEST);
		glDisable(GL_BLEND);

		glEnable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);

		//DrawInhabitants(0); // Draw inhabitants normally
		DrawCustom(0); // Draw normally
	}
	glPopMatrix();

	glutSwapBuffers(); // Do the buffer Swap
}

// Respond to arrow keys by moving the camera frame of reference
void SpecialFunc(int key, int x, int y)
{
    if (key == GLUT_KEY_UP) { frameCamera.MoveForward(0.1f); }

    if (key == GLUT_KEY_DOWN) { frameCamera.MoveForward(-0.1f); }

	if (key == GLUT_KEY_LEFT) { frameCamera.RotateLocalY(0.1f); }

	if (key == GLUT_KEY_RIGHT) { frameCamera.RotateLocalY(-0.1f); }

    glutPostRedisplay(); // Refresh the Window
}

void IdleFunc(void)
{
	glutPostRedisplay();
}

void TimerFunc(int value)
{
    glutPostRedisplay(); // Redraw the scene with new coordinates
    glutTimerFunc(3, TimerFunc, 1);
}

void ReshapeFunc(int w, int h)
{
    GLfloat fAspect;

    // Prevent a divide by zero, when window is too short
    // (you cant make a window of zero width).
	if (h == 0) { h = 1; }

    glViewport(0, 0, w, h);

    fAspect = (GLfloat)w / (GLfloat)h;

    // Reset the coordinate system before modifying
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Set the clipping volume
    gluPerspective(35.0f, fAspect, 1.0f, 50.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowSize(800, 600);
	glutCreateWindow("110AEM002 Final Project OpenGL (Ocean)");
	glutReshapeFunc(ReshapeFunc);
	glutSpecialFunc(SpecialFunc);
	glutDisplayFunc(DisplayFunc);

	SetupRC();
	glutIdleFunc(IdleFunc);
	glutTimerFunc(33, TimerFunc, 0);

	glutMainLoop();

	ShutdownRC(); // Delete the textures

	return 0;
}