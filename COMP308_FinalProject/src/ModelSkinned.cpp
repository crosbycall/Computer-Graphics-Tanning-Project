#pragma once

#include "ModelSkinned.hpp"
#include "skeleton.hpp"

ModelSkinned::ModelSkinned() {

}

ModelSkinned::ModelSkinned(string filename, vec3 origin, vec3 rotation, vec3 scale, int texRepeat) {
	readOBJ(filename);
	createDisplayList(origin, rotation, scale, texRepeat);
	textureID = -1;

	// Record orientation
	this->origin = origin;
	this->rotation = rotation;
	this->scale = scale;
	this->texRepeat = texRepeat;
}

void ModelSkinned::loadShaderComponents(GLuint g_shader) {
	// Load material for the model
	glUniform3f(glGetUniformLocation(g_shader, "material.ambient"), material.ambient.x, material.ambient.y, material.ambient.z);
	glUniform3f(glGetUniformLocation(g_shader, "material.diffuse"), material.diffuse.x, material.diffuse.y, material.diffuse.z);
	glUniform3f(glGetUniformLocation(g_shader, "material.specular"), material.specular.x, material.specular.y, material.specular.z);
	glUniform1f(glGetUniformLocation(g_shader, "material.shininess"), material.shininess);
}

void ModelSkinned::prepare(GLuint g_shader) {
	// Normal matrix
	vec3 columnOne = vec3(modelMatrix[0][0], modelMatrix[0][1], modelMatrix[0][2]);
	vec3 columnTwo = vec3(modelMatrix[1][0], modelMatrix[1][1], modelMatrix[1][2]);
	vec3 columnThree = vec3(modelMatrix[2][0], modelMatrix[2][1], modelMatrix[2][2]);
	mat3 matrix = mat3(columnOne, columnTwo, columnThree);
	mat3 normalMatrixFinal = matrix;

	// uniforms for shader
	glUniform1i(glGetUniformLocation(g_shader, "is2DTexture"), is2DTexture);
	glUniform1i(glGetUniformLocation(g_shader, "isBumpMapTexture"), isBumpMapTexture);
	glUniformMatrix3fv(glGetUniformLocation(g_shader, "normalMatrix"), 1, false, normalMatrixFinal.dataPointer());
}

void ModelSkinned::setMaterial(vec3 amb, vec3 diff, vec3 spec, float shine) {
	Material material = Material();
	material.ambient = amb;
	material.diffuse = diff;
	material.specular = spec;
	material.shininess = shine;
	this->material = material;
}

void ModelSkinned::readOBJ(std::string filename) {
	RigidBody rigidbody = RigidBody();
	rigidbody.filepath = filename;

	// Make sure our geometry information is cleared
	rigidbody.m_points.clear();
	rigidbody.m_uvs.clear();
	rigidbody.m_normals.clear();
	rigidbody.m_triangles.clear();

	// Load dummy points because OBJ indexing starts at 1 not 0
	rigidbody.m_points.push_back(vec3(0, 0, 0));
	rigidbody.m_uvs.push_back(vec2(0, 0));
	rigidbody.m_normals.push_back(vec3(0, 0, 1));

	ifstream objFile(filename);

	if (!objFile.is_open()) {
		std::cerr << "Error reading " << filename << std::endl;
		throw std::runtime_error("Error :: could not open file.");
	}

	std::cout << "Reading file " << filename << std::endl;

	// good() means that failbit, badbit and eofbit are all not set
	while (objFile.good()) {

		// Pull out line from file
		string line;
		getline(objFile, line);
		istringstream objLine(line);

		// Pull out mode from line
		std::string mode;
		objLine >> mode;

		// Reading like this means whitespace at the start of the line is fine
		// attempting to read from an empty string/line will set the failbit
		if (!objLine.fail()) {

			if (mode == "v") {
				vec3 v;
				objLine >> v.x >> v.y >> v.z;
				rigidbody.m_points.push_back(v);

			}
			else if (mode == "vn") {
				vec3 vn;
				objLine >> vn.x >> vn.y >> vn.z;
				rigidbody.m_normals.push_back(vn);

			}
			else if (mode == "vt") {
				vec2 vt;
				objLine >> vt.x >> vt.y;
				rigidbody.m_uvs.push_back(vt);

			}
			else if (mode == "f") {

				std::vector<vertex> verts;
				while (objLine.good()) {
					vertex v;

					//-------------------------------------------------------------
					// [Assignment 1] :
					// Modify the following to parse the bunny.obj. It has no uv
					// coordinates so each vertex for each face is in the format
					// v//vn instead of the usual v/vt/vn.
					//
					// Modify the following to parse the dragon.obj. It has no
					// normals or uv coordinates so the format for each vertex is
					// v instead of v/vt/vn or v//vn.
					//
					// Hint : Check if there is more than one uv or normal in
					// the uv or normal vector and then parse appropriately.
					//-------------------------------------------------------------

					// Assignment code (assumes you have all of v/vt/vn for each vertex)
					objLine >> v.p;		// Scan in position index

					if (objLine.peek() == '/')
						objLine.ignore(1);	// Ignore the '/' character

					if (rigidbody.m_uvs.size() > 1) // If we have read any TextureCoords
						objLine >> v.t;		// Scan in uv (texture coord) index

					if (objLine.peek() == '/')
						objLine.ignore(1);	// Ignore the '/' character

					if (rigidbody.m_normals.size() > 1) // If we have read any Normals
						objLine >> v.n;		// Scan in normal index

					verts.push_back(v);
				}

				// IFF we have 3 verticies, construct a triangle
				if (verts.size() == 3) {
					triangle tri;
					tri.v[0] = verts[0];
					tri.v[1] = verts[1];
					tri.v[2] = verts[2];
					rigidbody.m_triangles.push_back(tri);
				}
			}
		}
	}

	//std::cout << "Reading OBJ file is DONE." << std::endl;
	std::cout << "Finished reading from " << filename << endl;
	std::cout << rigidbody.m_points.size() - 1 << " points" << std::endl;
	std::cout << rigidbody.m_uvs.size() - 1 << " uv coords" << std::endl;
	std::cout << rigidbody.m_normals.size() - 1 << " normals" << std::endl;
	std::cout << rigidbody.m_triangles.size() << " faces" << std::endl;

	rigidbody.filepath == filename;
	body = rigidbody;
}

void ModelSkinned::createDisplayList(vec3 origin, vec3 rotation, vec3 scale, int texRepeat) {
	if (listID) {
		glDeleteLists(listID, 1);
	}

	float matrix2[16];
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(origin.x, origin.y, origin.z);
	glRotatef(rotation.x, 1, 0, 0);
	glRotatef(rotation.y, 0, 1, 0);
	glRotatef(rotation.z, 0, 0, 1);
	glScalef(scale.x, scale.y, scale.z);
	glGetFloatv(GL_MODELVIEW_MATRIX, matrix2);
	vec4 one = vec4(matrix2[0], matrix2[1], matrix2[2], matrix2[3]);
	vec4 two = vec4(matrix2[4], matrix2[5], matrix2[6], matrix2[7]);
	vec4 three = vec4(matrix2[8], matrix2[9], matrix2[10], matrix2[11]);
	vec4 four = vec4(matrix2[12], matrix2[13], matrix2[14], matrix2[15]);
	mat4 _m = mat4(one, two, three, four);
	modelMatrix = _m;
	//cout << modelMatrix << endl;

	// Create a new list
	//cout << "Creating Poly Geometry" << endl;
	GLuint id = glGenLists(1);
	glNewList(id, GL_COMPILE);

	// Render entire Polygons, not just the 'wires'
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Extract some information to make code below shoter
	vector<cgra::vec3> m_points = body.m_points;
	vector<cgra::vec3> m_normals = body.m_normals;
	vector<cgra::vec2> m_uvs = body.m_uvs;
	vector<triangle> m_triangles = body.m_triangles;

	// Setup our transformation matrix
	mat4 tMatrix = tMatrix.translate(origin.x, origin.y, origin.z);
	mat4 rMatrix = rMatrix.rotateX(rotation.x);
	rMatrix *= rMatrix.rotateY(rotation.y);
	rMatrix *= rMatrix.rotateZ(rotation.z);
	mat4 sMatrix = sMatrix.scale(scale.x, scale.y, scale.z);

	// Find the total transformation matrix
	mat4 matrix = tMatrix * rMatrix * sMatrix;

	//modelMatrix = matrix;
	// Render 

	glBegin(GL_TRIANGLES);
	for (int i = 0; i < m_triangles.size(); i++) {
		for (int c = 0; c < 3; c++) {
			// Retrieve normals
			float nx = m_normals[m_triangles[i].v[c].n].x;
			float ny = m_normals[m_triangles[i].v[c].n].y;
			float nz = m_normals[m_triangles[i].v[c].n].z;

			// Retrieve texture coords
			float u = m_uvs[m_triangles[i].v[c].t].x * texRepeat;
			float v = m_uvs[m_triangles[i].v[c].t].y * texRepeat;

			// Retrieve vertices
			float vx = m_points[m_triangles[i].v[c].p].x;
			float vy = m_points[m_triangles[i].v[c].p].y;
			float vz = m_points[m_triangles[i].v[c].p].z;
			vec4 vertex(vx, vy, vz, 1.0f);
			
			//cout << modelMatrix << endl;

			//vec4 diffd = vec4(m_triangles[i].v[c].diff + vec3(vx, vy, vz),1);
			
			vertex = m_triangles[i].v[c].trans * modelMatrix * vertex ;
			//vertex = vertex * modelMatrix;
			//vertex =  vertex * modelMatrix ;
			//vertex = matrix * vertex;

			// Make the OpenGL calls with the Matrix applied to the vertices, make sure to do Vertex last!
			if (bonesCalcd) {
				string bname = m_triangles[i].v[c].closeJoints[0]->name;
				if (bname == "topback") {
					glColor3f(1, 0, 0);
				}
				else if (bname == "upperback") {
					glColor3f(0, 1, 0);
				}
				else if (bname == "lowerback") {
					glColor3f(0, 0, 1);
				}
				else {
					glColor3f(0, 0, 0);
				}
			}
			glNormal3f(nx, ny, nz);
			glTexCoord2f(u, v);
			glVertex3f(vertex.x, vertex.y, vertex.z);
		}
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnd();

	glEndList();
	//cout << "Finished creating Poly Geometry" << endl;

	// Record our display list ID
	listID = id;
}

void ModelSkinned::setTextureID(GLuint texID, bool is2D, bool isBumpMap) {
	textureID = texID;
	is2DTexture = is2D;
	isBumpMapTexture = isBumpMap;
}
