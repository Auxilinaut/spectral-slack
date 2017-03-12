
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

#include "mesh_loader.h"

namespace mesh{
	// Is a vertex format?
    VertexFormat::VertexFormat(){
        position_x = position_y = position_z = 0;
        normal_x = normal_y = normal_z = 0;
        texcoord_x = texcoord_y = 0;
    }
    VertexFormat::VertexFormat(float px, float py, float pz){
        position_x = px;		position_y = py;		position_z = pz;
        normal_x = normal_y = normal_z = 0;
        texcoord_x = texcoord_y = 0;
    }
    VertexFormat::VertexFormat(float px, float py, float pz, float nx, float ny, float nz){
        position_x = px;		position_y = py;		position_z = pz;
        normal_x = nx;		normal_y = ny;		normal_z = nz;
        texcoord_x = texcoord_y = 0;
    }
    VertexFormat::VertexFormat(float px, float py, float pz, float tx, float ty){
        position_x = px;		position_y = py;		position_z = pz;
        texcoord_x = tx;		texcoord_y = ty;
        normal_x = normal_y = normal_z = 0;
    }
    VertexFormat::VertexFormat(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty){
        position_x = px;		position_y = py;		position_z = pz;
        normal_x = nx;		normal_y = ny;		normal_z = nz;
        texcoord_x = tx;		texcoord_y = ty;
    }
    VertexFormat VertexFormat::operator=(const VertexFormat &rhs){
        position_x = rhs.position_x;	position_y = rhs.position_y;	position_z = rhs.position_z;
        normal_x = rhs.normal_x;		normal_y = rhs.normal_y;		normal_z = rhs.normal_z;
        texcoord_x = rhs.texcoord_x;	texcoord_y = rhs.texcoord_y;
        return (*this);
    }

	// Load a file type Obj (without NURBS without materials)
	// Returns the arguments submitted by reference id vao OpenGL (Vertex Array Object) for vbo (Vertex Buffer Object) and Ibo (Index Buffer Object)
	void loadObj(const std::string &filename, unsigned int &vao, unsigned int& vbo, unsigned int &ibo, unsigned int &num_indices){
		// Load and indexes the file vertecsii
        std::vector<VertexFormat> vertices;
        std::vector<unsigned int> indices;
        _loadObjFile(filename, vertices, indices);

        std::cout << "Mesh Loader : loaded file " << filename << std::endl;

		// Create the necessary OpenGL drawing objects
        unsigned int gl_vertex_array_object, gl_vertex_buffer_object, gl_index_buffer_object;

		// Vertex array object -> object that represents a container for drawing state
        glGenVertexArrays(1, &gl_vertex_array_object);
        glBindVertexArray(gl_vertex_array_object);

		// Vertex buffer object -> object to hold our vertecsii
        glGenBuffers(1, &gl_vertex_buffer_object);
        glBindBuffer(GL_ARRAY_BUFFER, gl_vertex_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(VertexFormat), &vertices[0], GL_STATIC_DRAW);

		// Index buffer object -> object to hold our indexes
        glGenBuffers(1, &gl_index_buffer_object);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_index_buffer_object);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		// Link between attributes vertecsilor and pipeline, our data are interleaved.
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)0);						//pos pipe 0
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)(sizeof(float)* 3));		//normal pipe 1
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)(2 * sizeof(float)* 3));	//texcoords pipe 2

        vao = gl_vertex_array_object;
        vbo = gl_vertex_buffer_object;
        ibo = gl_index_buffer_object;
        num_indices = indices.size();
    }

    //-------------------------------------------------------------------------------------------------
    //follows parsing code ...

    //helper funcs
    float _stringToFloat(const std::string &source){
        std::stringstream ss(source.c_str());
        float result;
        ss >> result;
        return result;
    }
    //transforms a string to an int
    unsigned int _stringToUint(const std::string &source){
        std::stringstream ss(source.c_str());
        unsigned int result;
        ss >> result;
        return result;
    }
    //transforms a string to an int
    int _stringToInt(const std::string &source){
        std::stringstream ss(source.c_str());
        int result;
        ss >> result;
        return result;
    }
    //writes the tokens of the source string into tokens
    void _stringTokenize(const std::string &source, std::vector<std::string> &tokens){
        tokens.clear();
        std::string aux = source;
        for (unsigned int i = 0; i<aux.size(); i++) if (aux[i] == '\t' || aux[i] == '\n') aux[i] = ' ';
        std::stringstream ss(aux, std::ios::in);
        while (ss.good()){
            std::string s;
            ss >> s;
            if (s.size()>0) tokens.push_back(s);
        }
    }
    //variant for faces
    void _faceTokenize(const std::string &source, std::vector<std::string> &tokens){
        std::string aux = source;
        for (unsigned int i = 0; i < aux.size(); i++) if (aux[i] == '\\' || aux[i] == '/') aux[i] = ' ';
        _stringTokenize(aux, tokens);
    }

	// Load only geometry from a file obj (not loaded high order surfaces, materials, coordinated extra lines)
	// Format: http://paulbourke.net/dataformats/obj/
	// Calculate not normal or texture coordinates or tangent, but readable non-optimal performance (relative to other parsers ..)
	// Consider geometry as a single object, so do not take into account or smoothing groups
	void _loadObjFile(const std::string &filename, std::vector<VertexFormat> &vertices, std::vector<unsigned int> &indices){
        //read the file
        std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
        if (!file.good()){
            std::cout << "Mesh Loader: Obj file not found " << filename << " or no rights to open!" << std::endl;
            std::terminate();
        }

        std::string line;
        std::vector<std::string> tokens, facetokens;
        std::vector<glm::vec3> positions;		positions.reserve(1000);
        std::vector<glm::vec3> normals;		normals.reserve(1000);
        std::vector<glm::vec2> texcoords;		texcoords.reserve(1000);
        while (std::getline(file, line)){
            //tokens line read
            _stringTokenize(line, tokens);

            //if I have nothing going on
            if (tokens.size() == 0) continue;

            //If I go further continue
            if (tokens.size() > 0 && tokens[0].at(0) == '#') continue;

            //if I have a vertex
            if (tokens.size() > 3 && tokens[0] == "v") positions.push_back(glm::vec3(_stringToFloat(tokens[1]), _stringToFloat(tokens[2]), _stringToFloat(tokens[3])));

            //if I have a normal
            if (tokens.size() > 3 && tokens[0] == "vn") normals.push_back(glm::vec3(_stringToFloat(tokens[1]), _stringToFloat(tokens[2]), _stringToFloat(tokens[3])));

            //if I have a texcoord
            if (tokens.size() > 2 && tokens[0] == "vt") texcoords.push_back(glm::vec2(_stringToFloat(tokens[1]), _stringToFloat(tokens[2])));

            //if I have a face (f + minimum three indexes)
            if (tokens.size() >= 4 && tokens[0] == "f"){
				// Use the first vertex of the face determines the format girl (v v/t v//n v/t/n) = (1 2 3 4)
                unsigned int face_format = 0;
                if (tokens[1].find("//") != std::string::npos) face_format = 3;
                _faceTokenize(tokens[1], facetokens);
                if (facetokens.size() == 3) face_format = 4; // vertecsi/texcoords/normal
                else{
                    if (facetokens.size() == 2){
                        if (face_format != 3) face_format = 2;
                    }
                    else{
                        face_format = 1; //only vertecsi
                    }
                }

                //the first index of this polygon
                unsigned int index_of_first_vertex_of_face = -1;

                for (unsigned int num_token = 1; num_token<tokens.size(); num_token++){
                    if (tokens[num_token].at(0) == '#') break;					
                    _faceTokenize(tokens[num_token], facetokens);
                    if (face_format == 1){
                        //only position
                        int p_index = _stringToInt(facetokens[0]);
                        if (p_index>0) p_index -= 1;								//obj has 1...n indices
                        else p_index = positions.size() + p_index;				//index negativ

                        vertices.push_back(VertexFormat(positions[p_index].x, positions[p_index].y, positions[p_index].z));
                    }
                    else if (face_format == 2){
                        // position and texcoord
                        int p_index = _stringToInt(facetokens[0]);
                        if (p_index > 0) p_index -= 1;								//obj has 1...n indices
                        else p_index = positions.size() + p_index;				//index negativ

                        int t_index = _stringToInt(facetokens[1]);
                        if (t_index > 0) t_index -= 1;								//obj has 1...n indices
                        else t_index = texcoords.size() + t_index;				//index negativ

                        vertices.push_back(VertexFormat(positions[p_index].x, positions[p_index].y, positions[p_index].z, texcoords[t_index].x, texcoords[t_index].y));
                    }
                    else if (face_format == 3){
                        //position and normal
                        int p_index = _stringToInt(facetokens[0]);
                        if (p_index > 0) p_index -= 1;								//obj has 1...n indices
                        else p_index = positions.size() + p_index;				//index negativ

                        int n_index = _stringToInt(facetokens[1]);
                        if (n_index > 0) n_index -= 1;								//obj has 1...n indices
                        else n_index = normals.size() + n_index;					//index negativ

                        vertices.push_back(VertexFormat(positions[p_index].x, positions[p_index].y, positions[p_index].z, normals[n_index].x, normals[n_index].y, normals[n_index].z));
                    }
                    else{
                        //normal position & texcoord
                        int p_index = _stringToInt(facetokens[0]);
                        if (p_index > 0) p_index -= 1;								//obj has 1...n indices
                        else p_index = positions.size() + p_index;				//index negativ

                        int t_index = _stringToInt(facetokens[1]);
                        if (t_index > 0) t_index -= 1;								//obj has 1...n indices
                        else t_index = normals.size() + t_index;					//index negativ

                        int n_index = _stringToInt(facetokens[2]);
                        if (n_index > 0) n_index -= 1;								//obj has 1...n indices
                        else n_index = normals.size() + n_index;					//index negativ

                        vertices.push_back(VertexFormat(positions[p_index].x, positions[p_index].y, positions[p_index].z, normals[n_index].x, normals[n_index].y, normals[n_index].z, texcoords[t_index].x, texcoords[t_index].y));
                    }

                    //add indexes
                    if (num_token < 4){
                        if (num_token == 1) index_of_first_vertex_of_face = vertices.size() - 1;
                        //doar triunghiuri f 0 1 2 3 (4 indecsi, primul e ocupat de f)
                        indices.push_back(vertices.size() - 1);
                    }
                    else{
						// Polygon => triangle predecessor last vertex and 0 relatively new addition to vertecsi polygon (independent clockwise)
                        indices.push_back(index_of_first_vertex_of_face);
                        indices.push_back(vertices.size() - 2);
                        indices.push_back(vertices.size() - 1);
                    }
                }//end for
            }//end face
        }//end while
    }
}