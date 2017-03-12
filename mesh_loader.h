
// ------------------------------------------------ -------------------------------------------------
// Description: Mesh Loader, translated by Google
//
//
// Note:
// You are encouraged to write your own parser for other formats. Suggested formats: ply, off
//
// Author: Lucian Petrescu
// Date: September 28, 2013
// ------------------------------------------------ -------------------------------------------------

#pragma once

#include "GL\glew.h"
#include "glm\glm.hpp"
#include "glm\gtc\type_ptr.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

namespace mesh{
	// Is a vertex format?
    struct VertexFormat{
        float position_x, position_y, position_z;
        float normal_x, normal_y, normal_z;						
        float texcoord_x, texcoord_y;							
        VertexFormat();
        VertexFormat(float px, float py, float pz);
        VertexFormat(float px, float py, float pz, float nx, float ny, float nz);
        VertexFormat(float px, float py, float pz, float tx, float ty);
        VertexFormat(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty);
        VertexFormat operator=(const VertexFormat &rhs);
    };

	// Load a file type Obj (without NURBS without materials)
	// Returns the arguments submitted by reference id vao OpenGL (Vertex Array Object) for vbo (Vertex Buffer Object) and Ibo (Index Buffer Object)
	void loadObj(const std::string &filename, unsigned int &vao, unsigned int& vbo, unsigned int &ibo, unsigned int &num_indices);

	//-------------------------------------------------------------------------------------------------
	//follows parsing code ...

    //helper funcs
    float _stringToFloat(const std::string &source);
    //transforms a string to an int
    unsigned int _stringToUint(const std::string &source);
    //transforms a string to an int
    int _stringToInt(const std::string &source);
    //writes the tokens of the source string into tokens
    void _stringTokenize(const std::string &source, std::vector<std::string> &tokens);
    //variant for faces
    void _faceTokenize(const std::string &source, std::vector<std::string> &tokens);

	// Load only geometry from a file obj (not loaded high order surfaces, materials, coordinated extra lines)
	// Format: http://paulbourke.net/dataformats/obj/
	// Calculate not normal or texture coordinates or tangent, but readable non-optimal performance (relative to other parsers ..)
	// Consider geometry as a single object, so do not take into account or smoothing groups
	void _loadObjFile(const std::string &filename, std::vector<VertexFormat> &vertices, std::vector<unsigned int> &indices);
}