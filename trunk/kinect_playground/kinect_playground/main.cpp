#include <iostream>
#include <string>
#include <GL/glut.h>
#include <XnCppWrapper.h>

using namespace std;

/* Defines */
#define WINDOW_SIZE_X 800
#define WINDOW_SIZE_Y 600

/* Structs */
struct Position {
	int x;
	int y;
	int z;
	int fx;
	int fy;
	int fz;
	bool leftclick;
	bool rightclick;
} pos = { 0, 0, 0, -1, -1, -1, false, false };

/* Globals */
static int win;
XnStatus nRetVal;
xn::Context context;
xn::DepthGenerator depth;
xn::ImageGenerator image;
xn::UserGenerator user;
bool calibration_flag = false;
XnUserID cUser[2] = { 0, 0 };
XnUInt16 cnUsers = 0;	/* Auf 1 Benutzer beschränken */
XnSkeletonJointPosition rhand;
XnSkeletonJointPosition torso;



bool checkError(string message, XnStatus nRetVal) {
	if(nRetVal != XN_STATUS_OK) {
		cout << message << ": " << xnGetStatusString(nRetVal) << endl;
		return false;
	}
	return true;
}

void _stdcall skel_cal_start(xn::SkeletonCapability& skeleton, XnUserID user, void* pCookie) {
	static bool output_once = false;

	if(output_once==false) {
		cout << "Kalibrationspose wurde erkannt. Bitte warten." << endl;
		output_once = true;
	}
}

void _stdcall skel_cal_end(xn::SkeletonCapability& skeleton, XnUserID user, XnBool bSuccess, void* pCookie) {
	static bool output_once1 = false, output_once2 = false;

	if(bSuccess) {
		if(output_once1==false) {
			cout << "Kalibration abgeschlossen. Viel Spass!" << endl;
			output_once1 = true;
			output_once2 = false;
		}
	}
	else {
		if(output_once2==false) {
			cout << "Kalibration fehlgeschlagen. Neuer Versuch!" << endl;
			output_once2 = true;
			output_once1 = false;
			calibration_flag = true;
		}
	}
}

void glut_keyboard(unsigned char key, int x, int y) {
	switch(key) {
		case 'c':
		case 'C':
			calibration_flag = true;
			break;
	}
}

void glut_display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//gluPerspective(45.0, WINDOW_SIZE_X/WINDOW_SIZE_Y, 1, 100);
	glOrtho(0, WINDOW_SIZE_X, 0, WINDOW_SIZE_Y, 1, 100);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -50);

	if(pos.leftclick)
		glColor3f(1.0, 0.0, 0.0);
	else
		glColor3f(1.0, 1.0, 1.0);
	glPointSize(5);
	glBegin(GL_POINTS);
		glVertex3f(pos.x, -pos.y, 1);
	glEnd();



	glutSwapBuffers();
}

void glut_idle() {
	static int round_coords = 0;
	static int round_x=0;
	static int round_y=0;
	static bool output_once = false;

	/* Warten auf neue Daten von Kinect */
	nRetVal = context.WaitAndUpdateAll();
	checkError("Fehler beim Aktualisieren der Daten", nRetVal);

	cnUsers=2;
	user.GetUsers(cUser, cnUsers);
	if(cUser[0]==1 && output_once==false) {
		cout << "User 1 erkannt" << endl;
		output_once=true;
	}
	if(cnUsers>1) 
		cout << "Mehr als 1 User erkannt! Programm neu starten!" << endl;

	xn::SkeletonCapability skeleton = user.GetSkeletonCap();
	XnCallbackHandle hnd;
	skeleton.RegisterCalibrationCallbacks(skel_cal_start, skel_cal_end, 0, hnd);
	if(calibration_flag) {
		calibration_flag = false;
		cout << "Kalibration wird gestartet. Arme bitte im 90° Winkel nach oben halten und abwarten." << endl;
		skeleton.RequestCalibration(cUser[0], true);
	}

	if(skeleton.IsCalibrated(cUser[0])) {
		skeleton.SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
		skeleton.StartTracking(cUser[0]);
		skeleton.GetSkeletonJointPosition(cUser[0], XN_SKEL_TORSO, torso);
		if(torso.fConfidence) {
			if(pos.fx==-1) 
				pos.fx=torso.position.X;
			if(pos.fy==-1) 
				pos.fy=torso.position.Y;
			if(pos.fz==-1) 
				pos.fz=torso.position.Z;

			if(pos.fx!=-1) {
				if(abs(pos.fx-torso.position.X)>=50) {
					pos.fx=torso.position.X;
					cout << "Relative Posaenderung X" << endl;
				}
			}
			if(pos.fy!=-1) {
				if(abs(pos.fy-torso.position.Y)>=50) {
					pos.fy=torso.position.Y;
					cout << "Relative Posaenderung Y" << endl;
				}
			}
			if(pos.fz!=-1) {
				if(abs(pos.fz-torso.position.Z)>=50) {
					pos.fz=torso.position.Z;
					cout << "Relative Posaenderung Z" << endl;
				}
			}
		}

		skeleton.GetSkeletonJointPosition(cUser[0], XN_SKEL_LEFT_HAND, rhand);
		if(rhand.fConfidence) {
// 			if(pos.fx==-1) 
// 				pos.fx = rhand.position.X;
// 			if(pos.fy==-1) 
// 				pos.fy = rhand.position.Y;
// 			if(pos.fz==-1) 
// 				pos.fz = rhand.position.Z;

			pos.x = pos.fx-rhand.position.X+400;
			pos.y = pos.fy-rhand.position.Y;
			pos.z = pos.fz-rhand.position.Z;

			if(pos.z>=400)
				pos.leftclick = true;
			else
				pos.leftclick = false;

// 			cout <<	"X: " <<
// 				rhand.position.X << 
// 				"\t Y: " <<
// 				rhand.position.Y <<
// 				"\t Z: " <<
// 				rhand.position.Z <<
// 				"\t\t x: " <<
// 				pos.x <<
// 				"\t y: " <<
// 				pos.y <<
// 				"\t z: " <<
// 				pos.z <<
// 				endl;
		}
	}

	
	glutPostRedisplay();
}

int main(int argc, char **argv) {
	nRetVal = XN_STATUS_OK;

	/* Context initialisieren (Kameradaten) */
	nRetVal = context.Init();
	checkError("Fehler beim Initialisieren des Context", nRetVal)?0:exit(-1);


	
	/* Tiefengenerator */
	// Erstellen
	nRetVal = depth.Create(context);
	checkError("Fehler beim Erstellen des Tiefengenerators", nRetVal)?0:exit(-1);
	// Einstellen
	XnMapOutputMode outputModeDepth;
	outputModeDepth.nXRes = 640;
	outputModeDepth.nYRes = 480;
	outputModeDepth.nFPS = 30;
	nRetVal = depth.SetMapOutputMode(outputModeDepth);
	checkError("Fehler beim Konfigurieren des Tiefengenerators", nRetVal)?0:exit(-1);


	/* Imagegenerator */
	// Erstellen
	nRetVal = image.Create(context);
	checkError("Fehler beim Erstellen des Bildgenerators", nRetVal)?0:exit(-1);
	// Einstellen
	XnMapOutputMode outputModeImage;
	outputModeImage.nXRes = 640;
	outputModeImage.nYRes = 480;
	outputModeImage.nFPS = 30;
	nRetVal = image.SetMapOutputMode(outputModeImage);
	checkError("Fehler beim Konfigurieren des Bildgenerators", nRetVal)?0:exit(-1);	

	/* Usergenerator */
	// Erstellen
	nRetVal = user.Create(context);
	checkError("Fehler beim Erstellen des Usergenerators", nRetVal)?0:exit(-1);

	/* Starten der Generatoren */
	nRetVal = context.StartGeneratingAll();
	checkError("Fehler beim Starten der Generatoren", nRetVal)?0:exit(-1);



	/* Glut initialisieren */
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(WINDOW_SIZE_X, WINDOW_SIZE_Y);
	glutInitWindowPosition(300,150);
	win = glutCreateWindow("Kinect");
	glClearColor(0.0, 0.0, 0.0, 0.0); //Hintergrundfarbe: Hier ein leichtes Blau
	glEnable(GL_DEPTH_TEST);          //Tiefentest aktivieren
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);           //Backface Culling aktivieren



	glutDisplayFunc(glut_display);
	glutIdleFunc(glut_idle);
	glutKeyboardFunc(glut_keyboard);
	glutMainLoop();
	return 0;
}