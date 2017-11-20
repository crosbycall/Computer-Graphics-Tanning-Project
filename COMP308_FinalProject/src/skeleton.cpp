//---------------------------------------------------------------------------
//
// Copyright (c) 2016 Taehyun Rhee, Joshua Scott, Ben Allen
//
// This software is provided 'as-is' for assignment of COMP308 in ECS,
// Victoria University of Wellington, without any express or implied warranty. 
// In no event will the authors be held liable for any damages arising from
// the use of this software.
//
// The contents of this file may not be copied or duplicated in any form
// without the prior permission of its owner.
//
//----------------------------------------------------------------------------

#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

#include "cgra_geometry.hpp"
#include "cgra_math.hpp"
#include "opengl.hpp"
#include "skeleton.hpp"

// If true, we will use the CQuat.hpp implementation for Quaternions
// If false, we will use the quat.hpp implementation for Quaternions
// Read QuaternionsIssue.txt for an explanation as to why both of these implementations exist
bool usingCQuat = true;

using namespace std;
using namespace cgra;
// using namespace qtr;

Skeleton::Skeleton(string filename, vec3 origin, vec3 rotation, vec3 scale) {
	bone b = bone();
	b.name = "root";
	b.freedom |= dof_rx;
	b.freedom |= dof_ry;
	b.freedom |= dof_rz;
	b.freedom |= dof_root;
	m_bones.push_back(b);
	readASF(filename);

	this->origin = origin;
	this->rotation = rotation;
	this->scale = scale;
	createDisplayList();
}

//-------------------------------------------------------------
// [Assignment 2] :
// You may need to revise this function for Completion/Challenge
//-------------------------------------------------------------
void Skeleton::renderSkeleton() {
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	//Actually draw the skeleton
	glTranslatef(origin.x, origin.y, origin.z);
	glRotatef(rotation.z, 0, 0, 1);
	glRotatef(rotation.y, 0, 1, 0);
	glRotatef(rotation.x, 1, 0, 0);
	glScalef(scale.x, scale.y, scale.z);
	renderBone(&m_bones[0]);

	// Clean up
	glPopMatrix();
}

void Skeleton::recursePropTransform(bone* b, vec3 rotation) {
	vec3 newRot = rotation + b->rotation;
	b->propTransform = newRot;

	for (int i = 0; i < b->children.size(); i++) {
		recursePropTransform(b->children[i], newRot);
	}
}


void Skeleton::calcPropTransforms() {
	recursePropTransform(&m_bones[0], vec3(0, 0, 0));

	for (bone b : m_bones) {
		//cout << b.propTransform << endl;
	}
}

void Skeleton::prepRecursive() {
	glMatrixMode(GL_MODELVIEW);
	glTranslatef(origin.x, origin.y, origin.z);
	glRotatef(rotation.z, 0, 0, 1);
	glRotatef(rotation.y, 0, 1, 0);
	glRotatef(rotation.x, 1, 0, 0);
	glScalef(scale.x, scale.y, scale.z);
	glLoadIdentity();
	bone* b = &m_bones[0];
	recursiveFindPos(b);

}

void Skeleton::recursiveFindPos(bone* b) {
	//current->jointPosition = b->jointPosition + vec3(b->length * b->boneDir.x, b->length * b->boneDir.y, b->length*b->boneDir.z);
	glPushMatrix();

	// Apply Basis Rotations (For Axes)
	glRotatef(b->basisRot.z, 0, 0, 1);
	glRotatef(b->basisRot.y, 0, 1, 0);
	glRotatef(b->basisRot.x, 1, 0, 0);

	// DO QUATERNION ROTATION HERE (BEFORE REVERTING BASIS ROTATION)
	if (editMode) { // The user is trying to edit the skeleton
		glRotatef(b->rotation.z, 0, 0, 1);
		glRotatef(b->rotation.y, 0, 1, 0);
		glRotatef(b->rotation.x, 1, 0, 0);
	}

	// Reverse Basis Rotations
	glRotatef(-b->basisRot.x, 1, 0, 0);
	glRotatef(-b->basisRot.y, 0, 1, 0);
	glRotatef(-b->basisRot.z, 0, 0, 1);

	// Rotate angle for our bone segment (from radians to degrees)
	//glRotatef(theta, -(b->boneDir.y), b->boneDir.x, 0);
	//glRotatef(theta, -(b->boneDir.y), b->boneDir.x, 0);

	//float theta = acos(dot(b->boneDir, vec3(0, 0, 1)));
	//vec3 bone_rot_axis = cross(b->boneDir, vec3(0, 0, 1));


	//cout << thetaTyler << " tyler theta" << endl;
	//cout << theta << " theta" << endl;
	//cout << bone_rot_axis << " bone rot" << endl;

	float matrix[16];
	//glLoadIdentity();
	glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
	vec4 one = vec4(matrix[0], matrix[1], matrix[2], matrix[3]);
	vec4 two = vec4(matrix[4], matrix[5], matrix[6], matrix[7]);
	vec4 three = vec4(matrix[8], matrix[9], matrix[10], matrix[11]);
	vec4 four = vec4(matrix[12], matrix[13], matrix[14], matrix[15]);
	mat4 _m = mat4(one, two, three, four);
	vec4 pos = _m*vec4(0, 0, 0, 1);

	//cout << b->length <<"  length of bone"<< endl;
	//cout << pos<<" pos" << endl;
	//cout << b->jointPosition;;
	b->jointPosition = vec3(pos.x,pos.y,pos.z);
	glTranslatef((b->length*3) * b->boneDir.x, (b->length*3) * b->boneDir.y, (b->length*3)*b->boneDir.z);
	
	
	// Translate to the end of the bone we just drew
	// Recurse over our children
	for (int i = 0; i < b->children.size(); i++) {
		recursiveFindPos(b->children[i]);
	}
	glPopMatrix();
}


//-------------------------------------------------------------
// [Assignment 2] :
// Should render each bone by drawing the axes, and the bone
// then translating to the end of the bone and drawing each
// of it's children. Remember to use the basis rotations
// before drawing the axes (and for Completion, rotation).
// The actual bone is NOT drawn with the basis rotation.
//
// Should not draw the root bone (because it has zero length)
// but should go on to draw it's children
//-------------------------------------------------------------
void Skeleton::renderBone(bone *b) {
	// Make one quadratic to use for all rendering
	GLUquadricObj *quadratic;
	quadratic = gluNewQuadric();

	// Start recursive process to render the skeleton
	renderBone(b, quadratic);

	// Clean up our quad
	gluDeleteQuadric(quadratic);

}

// Recursive method for drawing a skeleton.
// Called in Skeleton::renderBone(bone *b)
void Skeleton::renderBone(bone *b, GLUquadricObj *quad) {
	glPushMatrix();

	// Apply Basis Rotations (For Axes)
	glRotatef(b->basisRot.z, 0, 0, 1);
	glRotatef(b->basisRot.y, 0, 1, 0);
	glRotatef(b->basisRot.x, 1, 0, 0);

	// Sphere Joint
	if (b->selected)
		glColor3f(selectedJointColor.x / 255, selectedJointColor.y / 255, selectedJointColor.z / 255);
	else
		glColor3f(b->pickColor.x / 255, b->pickColor.y / 255, b->pickColor.z / 255);

	cgraSphere(R, 10, 10);

	// Red X Axis
	glPushMatrix();
		if (b->selected && b->selectedAxis == 0)
			glColor3f(selectedAxisX.x, selectedAxisX.y, selectedAxisX.z);
		else
			glColor3f(1, 0, 0);

		glRotatef(90, 0, 1, 0);
		gluCylinder(quad, cyBase, cyBase, cyLength, 5, 5);
		glTranslatef(0, 0, cyLength);
		gluCylinder(quad, coneBase, 0.0, coneHeight, 5, 5);
	glPopMatrix();

	// Green Y Axis
	glPushMatrix();
		if (b->selected && b->selectedAxis == 1)
			glColor3f(selectedAxisY.x, selectedAxisY.y, selectedAxisY.z);
		else
			glColor3f(0, 1, 0);

		glRotatef(-90, 1, 0, 0);
		gluCylinder(quad, cyBase, cyBase, cyLength, 5, 5);
		glTranslatef(0, 0, cyLength);
		gluCylinder(quad, coneBase, 0.0, coneHeight, 5, 5);
	glPopMatrix();

	// Blue Z Axis
	glPushMatrix();
		if (b->selected && b->selectedAxis == 2)
			glColor3f(selectedAxisZ.x, selectedAxisZ.y, selectedAxisZ.z);
		else
			glColor3f(0, 0, 1);

		gluCylinder(quad, cyBase, cyBase, cyLength, 5, 5);
		glTranslatef(0, 0, cyLength);
		gluCylinder(quad, coneBase, 0.0, coneHeight, 5, 5);
	glPopMatrix();

	// DO QUATERNION ROTATION HERE (BEFORE REVERTING BASIS ROTATION)

	if (editMode) { // The user is trying to edit the skeleton
		glRotatef(b->rotation.z, 0, 0, 1);
		glRotatef(b->rotation.y, 0, 1, 0);
		glRotatef(b->rotation.x, 1, 0, 0);
	}

	// Reverse Basis Rotations
	glRotatef(-b->basisRot.x, 1, 0, 0);
	glRotatef(-b->basisRot.y, 0, 1, 0);
	glRotatef(-b->basisRot.z, 0, 0, 1);

	// Rotate angle for our bone segment (from radians to degrees)
	float theta = acos(b->boneDir.z) * 180 / 3.14159;

	//float theta = acos(dot(b->boneDir, vec3(0, 0, 1)));
	//vec3 bone_rot_axis = cross(b->boneDir, vec3(0, 0, 1));


	// Draw Bone Segment
	glPushMatrix();
		if (b->selected)
			glColor3f(selectedBoneColor.x / 255, selectedBoneColor.y / 255, selectedBoneColor.z / 255);
		else
			glColor3f(0.8, 0.8, 0.8);

		glRotatef(theta, -(b->boneDir.y), b->boneDir.x, 0);

		gluCylinder(quad, baseRadius, topRadius, b->length, 10, 10);
	glPopMatrix();

	// Translate to the end of the bone we just drew
	glTranslatef(b->length * b->boneDir.x, b->length * b->boneDir.y, b->length*b->boneDir.z);

	// Recurse over our children
	for (int i = 0; i < b->children.size(); i++) {
		renderBone(b->children[i], quad);
	}
	glPopMatrix();
}

////Set positions of the joints for calculating weights on mesh vertex
//void Skeleton::calcJointPos(bone* b) {
//	for (int i = 0; i < m_bones.size(); i++) {
//		//if (i == 0 | m_bones[i].name=="lowerback") {
//		//	m_bones[i].jointPosition = vec3(0, 0, 0);
//		//}
//		//else {
//			bone* b = &m_bones[i - 1];
//			bone* current = &m_bones[i];
//			current->jointPosition = b->jointPosition + vec3(b->length * b->boneDir.x, b->length * b->boneDir.y, b->length*b->boneDir.z);
//		//}
//	}
//	bone* b = &m_bones[2];
//	cout << b->jointPosition << endl;
//}

// Helper method for retreiving and trimming the next line in a file.
// You should not need to modify this method.
namespace {
	string nextLineTrimmed(istream &file) {
		// Pull out line from file
		string line;
		getline(file, line);
		// Remove leading and trailing whitespace and comments
		size_t i = line.find_first_not_of(" \t\r\n");
		if (i != string::npos) {
			if (line[i] != '#') {
				return line.substr(i, line.find_last_not_of(" \t\r\n") - i + 1);
			}
		}
		return "";
	}
}


int Skeleton::findBone(string name) {
	for (size_t i = 0; i < m_bones.size(); i++)
		if (m_bones[i].name == name)
			return i;
	return -1;
}


void Skeleton::readASF(string filename) {

	ifstream file(filename);

	if (!file.is_open()) {
		cerr << "Failed to open file " <<  filename << endl;
		throw runtime_error("Error :: could not open file.");
	}

	cout << "Reading file: " << filename << endl;

	// good() means that failbit, badbit and eofbit are all not set
	while (file.good()) {

		// Pull out line from file
		string line = nextLineTrimmed(file);

		// Check if it is a comment or just empty
		if (line.empty() || line[0] == '#')
			continue;
		else if (line[0] == ':') {
			// Line starts with a ':' character so it must be a header
			readHeading(line, file);
		} else {
			// Would normally error here, but becuase we don't parse
			// every header entirely we will leave this blank.
		}
	}

	cout << "Completed reading skeleton file" << endl;
}


void Skeleton::readHeading(string headerline, ifstream &file) {

	string head;
	istringstream lineStream(headerline);
	lineStream >> head; // get the first token from the stream

	// remove the ':' from the header name
	if (head[0] == ':')
		head = head.substr(1);

	if (lineStream.fail() || head.empty()) {
		cerr << "Could not get heading name from\"" << headerline << "\", all is lost" << endl;
		throw runtime_error("Error :: could not parse .asf file.");
	}

	if (head == "version") {
		//version string - must be 1.10
		string version;
		lineStream >> version;
		if (lineStream.fail() || version != "1.10") {
			cerr << "Invalid version: \"" << version << "\" must be 1.10" << endl;
			throw runtime_error("Error :: invalid .asf version.");
		}
	}
	else if (head == "name") {
		// This allows the skeleton to be called something
		// other than the file name. We don't actually care
		// what the name is, so can ignore this whole section
	}
	else if (head == "documentation") {
		// Documentation section has no meaningful information
		// only of use if you want to copy the file. So we skip it
	}
	else if (head == "units") {
		// Has factors for the units to be able to model the
		// real person, these must be parsed correctly. Only
		// really need to check if deg or rad, but that is 
		// not needed for this assignment.

		// We are going to assume that the units:length feild
		// is 0.45, and that the angles are in degrees
	}
	else if (head == "root") {
		// Read in information about root. Let's just assume
		// it'll be at the origin for this assignment.
	}
	else if (head == "bonedata") {
		// Read in each bone until we get to the
		// end of the file or a new header
		string line = nextLineTrimmed(file);
		while (file.good() && !line.empty()) {
			if (line[0] == ':') {
				// finished our reading of bones
				// read next header and return
				return readHeading(line, file);
			}
			else if (line == "begin") {
				// Read the bone data
				readBone(file);
			}
			else {
				cerr << "Expected 'begin' in bone data, found \"" << line << "\"";
				throw runtime_error("Error :: could not parse .asf file.");
			}
			line = nextLineTrimmed(file);
		}
	}
	else if (head == "hierarchy") {
		// Description of how the bones fit together
		// Read in each line until we get to the
		// end of the file or a new header
		string line = nextLineTrimmed(file);
		while (file.good() && !line.empty()) {
			if (line[0] == ':') {
				// finished our reading of bones
				// read next header and return
				return readHeading(line, file);
			}
			else if (line == "begin") {
				// Read the entire hierarchy
				readHierarchy(file);
			}
			else {
				cerr << "Expected 'begin' in hierarchy, found \"" << line << "\"";
				throw runtime_error("Error :: could not parse .asf file.");
			}
			line = nextLineTrimmed(file);
		}
	}
	else {
		// Would normally error here, but becuase we don't parse
		// every header entirely we will leave this blank.
	}
}


void Skeleton::readBone(ifstream &file) {
	// Create the bone to add the data to
	bone b;

	// Initialise color picking variables for the bone
	b.pickColor -= vec3(0, 0, colorOffset);
	colorOffset++;

	string line = nextLineTrimmed(file);
	while (file.good()) {
		if (line == "end") {
			// End of the data for this bone
			// Push the bone into the vector
			m_bones.push_back(b);
			return;
		}
		else {
			
			string head;
			istringstream lineStream(line);
			lineStream >> head; // Get the first token

			if (head == "name") {
				// Name of the bone
				lineStream >> b.name;
			}
			else if (head == "direction") {
				// Direction of the bone
				lineStream >> b.boneDir.x >> b.boneDir.y >> b.boneDir.z;
				b.boneDir = normalize(b.boneDir); // Normalize here for consistency
			}
			else if (head == "length") {
				// Length of the bone
				float length;
				lineStream >> length;
				length *= (1.0/0.45);  // scale by 1/0.45 to get actual measurements
				length *= 0.0254;      // convert from inches to meters
				b.length = length;
			}
			else if (head == "dof") {
				// Degrees of Freedom of the joint (rotation)
				while (lineStream.good()) {
					string dofString;
					lineStream >> dofString;
					if (!dofString.empty()) {
						// Parse each dof string
						if      (dofString == "rx") b.freedom |= dof_rx;
						else if (dofString == "ry") b.freedom |= dof_ry;
						else if (dofString == "rz") b.freedom |= dof_rz;
						else throw runtime_error("Error :: could not parse .asf file.");
					}
				}
			}
			else if (head == "axis") {
				// Basis rotations 
				lineStream >> b.basisRot.x >> b.basisRot.y >> b.basisRot.z;
			}
			else if (head == "limits") {
				// Limits for each of the DOF
				// Assumes dof has been read first
				// You can optionally fill this method out
			}

			// Because we've tried to parse numerical values
			// check if we've failed at any point
			if (lineStream.fail()) {
				cerr << "Unable to parse \"" << line << "\"";
				throw runtime_error("Error :: could not parse .asf file.");
			}
		}

		// Get the next line
		line = nextLineTrimmed(file);
	}

	cerr << "Expected end in bonedata, found \"" << line << "\"";
	throw runtime_error("Error :: could not parse .asf file.");
}


void Skeleton::readHierarchy(ifstream &file) {
	string line = nextLineTrimmed(file);
	while (file.good()) {
		if (line == "end") {
			// End of hierarchy
			return;
		}
		else if (!line.empty()) {
			// Read the parent node
			string parentName;
			istringstream lineStream(line);
			lineStream >> parentName;

			// Find the parent bone and have a pointer to it
			int parentIndex = findBone(parentName);

			if (parentIndex < 0) {
				cerr << "Expected a valid parent bone name, found \"" << parentName << "\"" << endl;
				throw runtime_error("Error :: could not parse .asf file.");
			}

			//Read the connections
			string childName;
			lineStream >> childName;
			while (!lineStream.fail() && !childName.empty()) {

				int childIndex = findBone(childName);

				if (childIndex < 0) {
					cerr << "Expected a valid child bone name, found \"" << childName << "\"" << endl;
					throw runtime_error("Error :: could not parse .asf file.");
				}

				// Set a POINTER to the child to be recorded in the parents
				m_bones[parentIndex].children.push_back(&m_bones[childIndex]);
				
				// Get the next child
				lineStream >> childName;
			}
		}
		line = nextLineTrimmed(file);
	}
	cerr << "Expected end in bonedata, found \"" << line << "\"";
throw runtime_error("Error :: could not parse .asf file.");
}



//-------------------------------------------------------------
// [Assignment 2] :
// Complete the following method to load data from an *.amc file
//-------------------------------------------------------------
void Skeleton::readAMC(string filename) {
	// YOUR CODE GOES HERE
	// ...

}

/*
Given an RGB color, this method will find the joint with that color (color picking)
and select it for the user to rotate around.
*/
void Skeleton::selectJoint(int r, int g, int b) {
	cout << "Selecting joint with: " << r << ", " << g << ", " << b << endl;

	vec3 toFind = vec3(r, g, b);           // the color of the bone joint we are looking for
	int ghostIndex = selectedIndex;        // the previously selected joint, negative if no joint selected
	selectedIndex = -1;                    // if selectedIndex ix negative after the for loop, we never found a suitable joint

	// Loop over all the joints and check if it's joint color is what our mouse has picked
	for (int i = 0; i < m_bones.size(); i++) {
		if (m_bones[i].pickColor == toFind) {

			// Record the index of the bone, and set it's selected bool to true
			selectedIndex = i;
			m_bones[i].selected = true;
			cout << m_bones[i].name << endl;

			// If we had a previously selected bone, deselect it
			if (ghostIndex >= 0) {
				m_bones[ghostIndex].selected = false;
				m_bones[ghostIndex].selectedAxis = 0;
			}
			//createDisplayList();
			break;
		}
	}

	// If we never found a suitable joint, deselect everything
	if (selectedIndex < 0) {
		cout << "Didnt locate a joint" << endl;
		if (ghostIndex >= 0) {
			m_bones[ghostIndex].selected = false;
			m_bones[ghostIndex].selectedAxis = 0;
		}
	}
}

/*
Called when the user presses the x key, updates which axis of the current joint is selected
*/
void Skeleton::updateAxis() {
	if (selectedIndex < 0) {
		cout << "Haven't selected a joint!" << endl;
		return;
	}

	if (m_bones[selectedIndex].selectedAxis < 2)
		m_bones[selectedIndex].selectedAxis += 1;
	else
		m_bones[selectedIndex].selectedAxis = 0;

	cout << m_bones[selectedIndex].selectedAxis << endl;
	//createDisplayList();
}

//// Set the nearest bones and weights for every vetex in the model
//void Skeleton::getNearestBones(Model man) {
//	// For every triangle
//	for (int tri = 0; tri < man.body.m_triangles.size(); tri++) {
//		// For every vertex in the triangle
//		for (int v = 0; v < 3; v++) {
//			bone bones[2];
//			float nearest[2] = { 99999, 99999 };
//			// For every bone in man
//			for (bone b : this->m_bones) {
//				// Get the distance between current bone and current vertex
//				float distance = sqrt(pow((b.jointPosition.x - man.body.m_points[man.body.m_triangles[tri].v[v].p].x), 2) +
//					pow((b.jointPosition.y - man.body.m_points[man.body.m_triangles[tri].v[v].p].y), 2) +
//					pow((b.jointPosition.z - man.body.m_points[man.body.m_triangles[tri].v[v].p].z), 2));
//				// Set the current smallest distances if needed
//				if (distance < nearest[0]) {
//					nearest[0] = distance;
//					bones[0] = b;
//				}
//				else if (distance < nearest[1]) {
//					nearest[1] = distance;
//					bones[1] = b;
//				}
//			}
//			// Normalize and set as weights
//			float weight1 = nearest[0] / (nearest[0] + nearest[1]);
//			float weight2 = nearest[1] / (nearest[0] + nearest[1]);
//			man.body.m_triangles[tri].v[v].weights[0] = weight1;
//			man.body.m_triangles[tri].v[v].weights[1] = weight2;
//
//			// Set the bones for vertex
//			man.body.m_triangles[tri].v[v].closeJoints[0] = bones[0];
//			man.body.m_triangles[tri].v[v].closeJoints[1] = bones[1];
//			//cout << man.body.m_triangles[tri].v[v].closeJoints[0].name;
//			//cout << man.body.m_triangles[tri].v[v].weights[0];
//			//printing the bones/weights in closeJoints[]/weights[] tells me what they are, 
//			//but when I try to access them in another method they don't exist/are 0.
//		}
//	}
//}

/*
Rotates the selected axis of a joint given a delta in mouse position x and y
*/
void Skeleton::rotateAxis(double deltaX, double deltaY) {
	if (selectedIndex < 0) {
		cout << "No selected axis!" << endl;
		return;
	}

	// Moving the mouse in the y direction will not effect rotation
	// Only in the x direction

	double speedDampen = 5; // larger speed dampener means user has to move mouse further for rotation

	bone* b = &m_bones[selectedIndex];

	// current axis is x
	if (b->selectedAxis == 0) {
		b->rotation += vec3(deltaX / speedDampen, 0, 0);
	}
	// current axis is y
	else if (b->selectedAxis == 1) {
		b->rotation += vec3(0, deltaX / speedDampen, 0);
	}
	// current axis is z
	else if (b->selectedAxis == 2) {
		b->rotation += vec3(0, 0, deltaX / speedDampen);
	}
	//cout << "Should be rotating!" << endl; WE GET TO HERE
	//createDisplayList();
}

/*
Returns the vec3 rotation of the currently selected bone
*/
vec3 Skeleton::getSelectedBoneRot() {
	return m_bones[selectedIndex].rotation;
}

/*
Returns the currently selected bone's name
*/
string Skeleton::getSelectedBoneName() {
	return m_bones[selectedIndex].name;
}

/*
Writes a Skeleton pose to a.pose file, stored in work/res/poses directory
*/
void Skeleton::writePoseToFile(string filename) {
	// Append the path to the poses folder
	std::ostringstream oss;
	oss << "COMP308_FinalProject/res/" << filename << ".pose";
	ofstream outputFile(oss.str());
	outputFile << "begin" << endl;

	for (int i = 0; i < m_bones.size(); i++) {
		bone b = m_bones[i];
		outputFile << b.name << " " << b.rotation.x << " " << b.rotation.y << " " << b.rotation.z << endl;
	}

	outputFile << "end";
}

/*
Reads a .anim file and stores it in the posesforAnimation vector to use for animations
*/
void Skeleton::readAnimFile(string filename) {
	ifstream file(filename);

	if (!file.is_open()) {
		cerr << "Failed to open file " << filename << endl;
		throw runtime_error("Error :: could not open file.");
	}

	std::cout << "Reading file: " << filename << endl;

	// good() means that failbit, badbit and eofbit are all not set
	while (file.good()) {

		// Pull out line from file
		string line = nextLineTrimmed(file);

		if (line == "begin") {
			
		}
		else if (line == "end") {
			cout << "Finished reading " << posesforAnimation.size() << " poses" << endl;
			file.close();
			return;
		}
		else {
			readPoseFile(line);
		}
	}

	std::cout << "Finished reading file: " << filename << endl;
}

/*
Read a .pose file that is part of a .anim file
*/
void Skeleton::readPoseFile(string filename) {
	ifstream file(filename);

	if (!file.is_open()) {
		cerr << "Failed to open file " << filename << endl;
		throw runtime_error("Error :: could not open file.");
	}

	std::cout << "Reading file: " << filename << std::endl;

	Pose p = Pose();
	vector<bone> bones;

	// good() means that failbit, badbit and eofbit are all not set
	while (file.good()) {

		// Pull out line from file
		string line = nextLineTrimmed(file);

		if (line == "begin") {

		}
		else if (line == "end") {
			// Finalize our pose and add it to our poses list
			p.poseBones = bones;
			posesforAnimation.push_back(p);
			file.close();
			Pose startPose = p;
			for (int i = 0; i < startPose.poseBones.size(); i++) {
				bone* poseBone;
				bone* skeleBone;
				// Init our start rotations
				poseBone = &startPose.poseBones[i];
				skeleBone = &m_bones[findBone(poseBone->name)];
				skeleBone->startRotation = poseBone->startRotation;
			}
				return;
		}
		else {
			// Read one line of tokens into the appropriate variables
			string head, x, y, z;
			istringstream lineStream(line);
			lineStream >> head; // Get the first token
			lineStream >> x;
			lineStream >> y;
			lineStream >> z;

			// Make a new bone
			bone b = bone();
			b.name = head;
			b.startRotation = vec3(stof(x), stof(y), stof(z) );

			// Add the bone to our list
			bones.push_back(b);
		}
	}
}

/*
Changes every bone vec3 rotation to (0, 0, 0) - default priman.asf pose
*/
void Skeleton::resetToDefaultPosition() {
	for (int i = 0; i < m_bones.size(); i++) {
		bone* b = &m_bones[i];
		b->rotation = vec3(0, 0, 0);
	}
}

/*
Useful method for debugging data storage when reading from files.
*/
void Skeleton::debugRotations() {
	// Debugging print statements
	for (int i = 0; i < m_bones.size(); i++) {
		bone skeleBone = m_bones[i];

		if (usingCQuat) {
			cout << skeleBone.name << endl;
			cout << "Start is: " << skeleBone.startRotation << endl;
			cout << "Goal is: " << skeleBone.goalRotation << endl;
			cout << "Current Quaternion is: " << endl;
			cout << "--------------------------" << endl;
		}
		else {
			cout << skeleBone.name << endl;
			cout << "Start is: " << skeleBone.startRotation << endl;
			cout << "Goal is: " << skeleBone.goalRotation << endl;
			cout << "Current Quaternion is: " << endl;
			cout << "--------------------------" << endl;
		}
	}
}

///*
//Handles the startRotation, goalRotation and currentQuaternion data loading from our posesforAnimation vector
//when we reach the end of a key frame.
//*/
//void Skeleton::initAnimation(int startIndex) {
//	// Get the poses
//	// If we are rewinding, technically goalPose is our startingPose, and startPose is our goalPose.
//	Pose startPose, goalPose;
//	startPose = posesforAnimation[startIndex];
//	goalPose = posesforAnimation[startIndex + 1];
//
//	// Loop over the startpose and initialize our Skeleton bones startRotation and currentQuaternion or currentQuat
//	for (int i = 0; i < startPose.poseBones.size(); i++) {
//		bone* poseBone;
//		bone* skeleBone;
//
//		// Init our start rotations
//		poseBone = &startPose.poseBones[i];
//		skeleBone = &m_bones[findBone(poseBone->name)];
//		skeleBone->startRotation = poseBone->startRotation;
//
//		// init our goal rotations
//		poseBone = &goalPose.poseBones[i];
//		skeleBone = &m_bones[findBone(poseBone->name)];
//		skeleBone->goalRotation = poseBone->startRotation;
//
//		// init our current quaternion depending on if we are using CQuat.hpp or quat.hpp
//		if (usingCQuat) {
//			CQuat quaternion = CQuat();
//			if (!rewinding) {
//				quaternion.FromEuler(skeleBone->startRotation.x, skeleBone->startRotation.y, skeleBone->startRotation.z);
//				skeleBone->currentQuat = quaternion;
//			}
//			else {
//				quaternion.FromEuler(skeleBone->goalRotation.x, skeleBone->goalRotation.y, skeleBone->goalRotation.z);
//				skeleBone->currentQuat = quaternion;
//			}
//		}
//		else {
//			if (!rewinding) {
//				skeleBone->currentQuaternion = quat(skeleBone->startRotation.x, skeleBone->startRotation.y, skeleBone->startRotation.z);
//			}
//			else {
//				skeleBone->currentQuaternion = quat(skeleBone->goalRotation.x, skeleBone->goalRotation.y, skeleBone->goalRotation.z);
//			}
//		}
//	}
//
//	// At this point our skeleon's bones should have a start, goal and current orientations initialised.
//	// Now manage our animation variables
//	if (!rewinding) {
//		currentPoseIndex = startIndex;
//		currentT = 0.0f;
//	}
//	else {
//		currentPoseIndex = startIndex;
//		currentT = 1.0f;
//	}
//}

///*
//Does the interpolating for our animation, calls initAnimation with the appropriate parameters
//when we need to swap which keyframes we are interpolating between.
//Also updates our currentT variable appropriately.
//*/
//void Skeleton::manageAnimation() {
//	// Loop over the skeleton's bones and modify their currentQuaternion to be slerp(startQuaternion, goalQuaternion, currentT)
//	for (int i = 0; i < m_bones.size(); i++) {
//		bone* b = &m_bones[i];
//
//		if (usingCQuat) {
//			vec3 rot = b->startRotation;
//			vec3 rotG = b->goalRotation;
//			CQuat start = CQuat();
//			CQuat goal = CQuat();
//
//			start.FromEuler(rot.x, rot.y, rot.z);
//			goal.FromEuler(rotG.x, rotG.y, rotG.z);
//			CQuat newQuat = CQuat();
//			newQuat.Slerp(start, goal, currentT, true);
//
//			b->currentQuat = newQuat;
//		}
//		else {
//			vec3 rot = b->startRotation;
//			vec3 rotG = b->goalRotation;
//
//			quat start = quat(rot.x, rot.y, rot.z);
//			quat goal = quat(rotG.x, rotG.y, rotG.z);
//			quat newQuat = slerp(start, goal, currentT);
//			b->currentQuaternion = newQuat;
//		}
//	}
//
//	// Moved forward to the end of an animation
//	if (currentT > 1.0f && !rewinding) {
//		if (currentPoseIndex == posesforAnimation.size() - 2) {
//			currentPoseIndex = -1;
//			initAnimation(0);
//			cout << "Reset animation!" << endl;
//		}
//		else {
//			initAnimation(currentPoseIndex + 1);
//			cout << "Switched keyframes" << endl;
//		}
//	}
//	// Moved backwards to the start of an animation (reversed to a new keyframe pair)
//	else if (currentT < 0.0f && rewinding) {
//		if (currentPoseIndex == 0) {
//			currentPoseIndex = posesforAnimation.size() - 2;
//			initAnimation(posesforAnimation.size() - 2);
//			cout << "Reset animation!" << endl;
//		}
//		else {
//			initAnimation(currentPoseIndex - 1);
//			cout << "Switched keyframes" << endl;
//		}
//	}
//
//	// DO POSE VARIABLE MANAGEMENT
//	currentT += tSpeed;
//}

void Skeleton::prepare(Camera camera, int width, int height) {
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(-camera.position.x, -camera.position.y, -camera.position.z);
	glRotatef(camera.pitch, 1, 0, 0);
	glRotatef(camera.yaw, 0, 1, 0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(camera.g_fovy, float(width) / float(height), camera.g_znear, camera.g_zfar);
}

void Skeleton::createDisplayList() {
	// Create a new list
	cout << "Creating Poly Geometry" << endl;
	if (listID) {//if a list already exists, delete it
		glDeleteLists(listID, 1);
	}
	GLuint id = glGenLists(1);
	glNewList(id, GL_COMPILE);

	// Render entire Polygons, not just the 'wires'
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	this->renderSkeleton();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnd();

	glEndList();
	cout << "Finished creating Skeleton Geometry" << endl;

	// Record our display list ID
	listID = id;
}