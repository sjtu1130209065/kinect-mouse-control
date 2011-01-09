#include <iostream>
#include <string>
#include <GL/glut.h>
#include <XnCppWrapper.h>
#include <windows.h>

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
} posr = { 0, 0, 0, -1, -1, -1, false, false }, posl = { 0, 0, 0, -1, -1, -1, false, false };

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
XnSkeletonJointPosition rhand, lhand;
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

	if(posr.leftclick)
		glColor3f(1.0, 0.0, 0.0);
	else
		glColor3f(1.0, 1.0, 1.0);
	glPointSize(5);
	glBegin(GL_POINTS);
		glVertex3f(posr.x, -posr.y, 1);
	glEnd();



	glutSwapBuffers();
}

void LeftClick () {  
	INPUT    Input={0};
	// left down 
	Input.type      = INPUT_MOUSE;
	Input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
	::SendInput(1,&Input,sizeof(INPUT));

	// left up
	::ZeroMemory(&Input,sizeof(INPUT));
	Input.type      = INPUT_MOUSE;
	Input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
	::SendInput(1,&Input,sizeof(INPUT)); 
}

void glut_idle() {
	static int round_coords = 0;
	static int roundr_x=300;
	static int roundr_y=200;
	static int roundr_z=600;
	static int roundl_x=300;
	static int roundl_y=200;
	static bool output_once = false;
	static bool leftclick_old = false;

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
			if(posr.fx==-1) 
				posr.fx=torso.position.X;
			if(posr.fy==-1) 
				posr.fy=torso.position.Y;
			if(posr.fz==-1) 
				posr.fz=torso.position.Z;

			if(posr.fx!=-1) {
				if(abs(posr.fx-torso.position.X)>=70) {
					posr.fx=torso.position.X;
					cout << "Relative Posaenderung X" << endl;
				}
			}
			if(posr.fy!=-1) {
				if(abs(posr.fy-torso.position.Y)>=70) {
					posr.fy=torso.position.Y;
					cout << "Relative Posaenderung Y" << endl;
				}
			}
			if(posr.fz!=-1) {
				if(abs(posr.fz-torso.position.Z)>=70) {
					posr.fz=torso.position.Z;
					cout << "Relative Posaenderung Z" << endl;
				}
			}
		}

		skeleton.GetSkeletonJointPosition(cUser[0], XN_SKEL_RIGHT_HAND, lhand);
		if(lhand.fConfidence) {
			roundl_x = (lhand.position.X) * 0.2f + roundl_x * 0.8f;
			roundl_y = (lhand.position.Y) * 0.2f + roundl_y * 0.8f;

			posl.x = posr.fx-roundl_x;
			posl.y = posr.fy-roundl_y;
			if(posl.x<=-400)
				posr.leftclick = true;
			else
				posr.leftclick = false;

			if(posr.leftclick && leftclick_old!=posr.leftclick) {
				LeftClick();
			}
			leftclick_old=posr.leftclick;
			

			cout <<	"x: " <<
					posl.x <<
					"\ty: " <<
					posl.y << 
					"\tLeft: " <<
					posr.leftclick <<
					endl;
		}

		skeleton.GetSkeletonJointPosition(cUser[0], XN_SKEL_LEFT_HAND, rhand);
		if(rhand.fConfidence) {
			roundr_x = ((rhand.position.X-150)*3) * 0.1f + roundr_x * 0.9f;
			roundr_y = ((rhand.position.Y-350)*3) * 0.1f + roundr_y * 0.9f;
			roundr_z = rhand.position.Z * 0.5f + roundr_z * 0.5f;

//			posr.x = posr.fx-roundr_x+400;	/* punkt im ogl bild */

// 			posr.x = (posr.fx-roundr_x+50)*4;
// 			posr.y = (posr.fy-roundr_y+430)*4;
			posr.x = posr.fx-roundr_x;
			posr.y = posr.fy-roundr_y;
			posr.z = posr.fz-roundr_z;

/*			if(posr.z>=400)
				posr.leftclick = true;
			else
				posr.leftclick = false;*/

			SetCursorPos(posr.x, posr.y);


// 			cout <<	"X: " <<
// 				rhand.position.X << 
// 				"\t Y: " <<
// 				rhand.position.Y <<
// 				"\t Z: " <<
// 				rhand.position.Z <<
// 				"\t\t x: " <<
// 				posr.x <<
// 				"\t y: " <<
// 				posr.y <<
// 				"\t z: " <<
// 				posr.z <<
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