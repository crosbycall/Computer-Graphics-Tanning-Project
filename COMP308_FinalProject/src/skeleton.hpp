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

#pragma once

#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "cgra_math.hpp"
#include "opengl.hpp"
#include "Camera.hpp"
//#include "Model.hpp"
// #include "quat.hpp"
// #include "CQuat.hpp"



// Needed for Completion/Challenge
// We use bitmasking to work out the Degrees of Freedom
// To work out if a bone b has a y-axis dof, simply:
//     if (b.freedom & dof_ry) {...}
//
// To add and subtract degrees of freedom, respectively:
//     b.freedom |= dof_rx
//     b.freedom ^= dof_rx
using dof_set = unsigned int;

enum dof {
	dof_none = 0,
	dof_rx = 1,
	dof_ry = 2,
	dof_rz = 4,
	dof_root = 8 // Root has 6, 3 translation and 3 rotation
};


// Type to represent a bone
struct bone {
	cgra::vec3 jointPosition;	  // Position of the joint
	cgra::vec3 propTransform;	  // Propagated transform of the joint
	std::string name;             // Name of the bone
	float length = 0;             // Length of the bone
	cgra::vec3 boneDir;           // Direction of the bone
	cgra::vec3 basisRot;          // Euler angle rotations for the bone basis
	dof_set freedom = dof_none;   // Degrees of freedom for the joint rotation
	std::vector<bone *> children; // Pointers to bone children	



	cgra::vec3 pickColor = 
		cgra::vec3(0, 255, 255);  // Color of the joint for mouse picking using color picking (gets modified, this is the base)

	bool selected = false;        // if this is the selected bone
	int selectedAxis = 0;         // 0 for x axis, 1 for y axis, 2 for z axis

	// Completion and Challenge
	cgra::vec3 rotation;          // Rotation of joint in the basis (degrees)

	// cgra::quat currentQuaternion; // the current rotation in between start / goal
	// qtr::CQuat currentQuat;

	cgra::vec3 startRotation;
	cgra::vec3 goalRotation;

	// Challenge
	cgra::vec3 translation;       // Translation (Only for the Root)
	cgra::vec3 rotation_max;      // Maximum value for rotation for this joint (degrees)
	cgra::vec3 rotation_min;      // Minimum value for rotation for this joint (degrees)
};

struct Pose {
	std::vector<bone> poseBones;
};

struct Joint{
	vec3 worldPos;
	string name;
	mat4 rotationM4;
	std::vector<Joint> chlidJoints;
};


class Skeleton {

private:
	

	// R value
	float R = 0.015;

	// Axis Vars
	float cyBase = 0.3f * R; float cyLength = 4 * R;
	float coneBase = 0.8 * R; float coneHeight = 1.4 * R;

	// Bone vars
	float baseRadius = R / 1.5; float topRadius = R / 4;      // radii values for the top and bottom of the bone
	int colorOffset = 0;                                      // used during bone creation to generate similar colors for color picking

	// Selected Colors
	cgra::vec3 selectedJointColor = cgra::vec3(255, 0, 255);  // the color of a selected joint
	cgra::vec3 selectedBoneColor = cgra::vec3(255, 255, 0);   // the color of a selected bone
	cgra::vec3 selectedAxisX = cgra::vec3(1, 0.5, 0.5);       // the color of a selected x axis
	cgra::vec3 selectedAxisY = cgra::vec3(0.5, 1, 0.5);       // the color of a selected y axis
	cgra::vec3 selectedAxisZ = cgra::vec3(0.5, 0.5, 1);       // the color of a selected z axis

	// Helper method
	int findBone(std::string);
	
	// Reading code
	void readASF(std::string);
	void readHeading(std::string, std::ifstream&);
	void readBone(std::ifstream&);
	void readHierarchy(std::ifstream&);

	// Rendering
	void renderBone(bone *);
	void renderBone(bone *, GLUquadricObj *);


public:
	//void getNearestBones(Model man);
	std::vector<bone> m_bones;                                // the list of bones that make up the skeleton
	Skeleton(std::string, vec3 origin, vec3 rotation, vec3 scale);
	void renderSkeleton();
	void recursePropTransform(bone * b, vec3 rotation);
	void calcPropTransforms();
	void prepRecursive();
	void recursiveFindPos(bone * b);
	void readAMC(std::string);

	//void calcJointPos();
	int selectedIndex = -1;                                   // the index of the currently selected bone
	int currentPoseIndex = -1;                                // the index of the current pose in posesForAnimation
	float currentT = 0.0f;                                    // the current t value for the slerp function
	float tSpeed = 0.05f;                                     // speed of the t variable for slerp
	float pausedTSpeed = 0.0f;                                // a holder for the previous speed when we pause the animation
	bool rewinding = false;                                   // are we rewinding?
	bool editMode = true;                                     // are we editing priman.asf or animating priman.asf?

	vec3 origin;
	vec3 rotation;
	vec3 scale;
	GLuint listID;

	std::vector<Pose> posesforAnimation;                      // a list of poses that this skeleton should interpolate between, in order, to form an animation

	// User interaction with joints
	void selectJoint(int r, int g, int b);
	void rotateAxis(double deltaX, double deltaY);
	void updateAxis();

	// Other stuff
	cgra::vec3 getSelectedBoneRot();
	std::string getSelectedBoneName();
	void writePoseToFile(std::string filename);
	// cgra::quat getQuatForBone(int indexOfBone);

	// Custom file parsers
	void readAnimFile(std::string path);
	void readPoseFile(std::string line);

	// Animation methods
	// void initAnimation(int startIndex);
	// void manageAnimation();

	void prepare(Camera camera, int width, int height);
	void createDisplayList();

	// Debugger methods
	void debugRotations();
	void resetToDefaultPosition();
};
