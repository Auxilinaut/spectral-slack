/**
* Credit of original goes to Stamate Cosmin
*
* Description: World generator.
* The algorithm used in generation is the diamond square fractal generator:
* http://www.gameprogrammer.com/fractal.html
* http://www.bluh.org/code-the-diamond-square-algorithm/
*/

#pragma once

#include "world.h"

// Initialize the random number seed.
static std::random_device rd;
static std::mt19937 rgn(rd());

// Vertex initialization.
WorldVertex::WorldVertex() {
    this->position = glm::vec3(0, 0, 0);
    this->normal = glm::vec3(0, 0, 0);
}
WorldVertex::WorldVertex(glm::vec3 position) {
    this->position = position;
    this->normal = glm::vec3(0, 0, 0);
}
WorldVertex::WorldVertex(glm::vec3 position, glm::vec3 normal) {
    this->position = position;
    this->normal = normal;
}

// Instantiates the world, generates the terrains and binds all the buffers.
World::World(glm::vec3 position, float radius, unsigned int mode) {
    // Cache various values.
    this->radius = radius * WORLD_RADIUS_MULTIPLY;
    this->length = this->radius * 2;
    this->position = position;
    this->setMode(mode);
    int i;

    // Initialize the mode blocks.
    for (i = 0; i < WORLD_MODE_COUNT; i++)
        this->blocks[i] = NULL;

    // Generate every mode's block.
    this->generateBase(WORLD_MODE_BASE, WORLD_SQUARE_COUNT);
    this->generateTerrain(WORLD_MODE_FRACTAL, WORLD_SQUARE_COUNT);
    //this->tessellateTerrain(WORLD_MODE_FRACTAL, WORLD_MODE_FRACTAL_TESSELLATED_1X);
    //this->tessellateTerrain(WORLD_MODE_FRACTAL_TESSELLATED_1X,WORLD_MODE_FRACTAL_TESSELLATED_2X);
    //this->tessellateTerrain(WORLD_MODE_FRACTAL_TESSELLATED_2X,WORLD_MODE_FRACTAL_TESSELLATED_4X);

    // Bind buffers and prepare for rendering.
    this->bufferData();
}

// Destructor.
World::~World() {
    for (int i = 0; i < WORLD_MODE_COUNT; i++) {
        glDeleteBuffers(1, &(this->blocks[i]->vao));
        glDeleteBuffers(1, &(this->blocks[i]->vbo));
        glDeleteBuffers(1, &(this->blocks[i]->ibo));
    }
}

// Set the current rendered mode.
void World::setMode(unsigned int mode) { this->mode = mode; }

// Render the block on the correct position around the camera.
// We always render 4 blocks, that cover the fog radius completely.
void World::render(unsigned int shader, glm::mat4 model_matrix,
    glm::vec3 position, glm::mat4* objectToWorldMatrix, glm::mat4* projectionMatrix, glm::mat4* cameraToWorldMatrix, glm::mat4* modelViewProjectionMatrix, glm::mat3* objectToWorldNormalMatrix, GLuint uniformBindingPoint, GLuint uniformBlock, GLint uniformOffset[]) {
    WorldBlock* block = this->blocks[this->mode];
    glm::vec3 direction = glm::vec3((*cameraToWorldMatrix)[3]);

    // The start point can be found by dividing the current distance from the
    // origin point by the length of the block, rounding it to the closest
    // integer, then multiplying by the length. This gives the starting point
    // for the bottom-right block.
    glm::vec3 start = glm::vec3(
        round(direction.x / this->length), this->position.y,
        round(direction.z / this->length)) *
        glm::vec3(this->length, 0, this->length);

    // We only render if we actually have a block to render.
    if (block) {
        // If we're drawing the fractal mountains, send color variables to the
        // shader.
        if (this->mode != WORLD_MODE_BASE) {
            glUniform2f(glGetUniformLocation(shader, "boundary_top"),
                this->boundary_top.s, this->boundary_top.t);
            glUniform2f(glGetUniformLocation(shader, "boundary_bottom"),
                this->boundary_bottom.s, this->boundary_bottom.t);
            glUniform1i(glGetUniformLocation(shader, "draw_mountain"), true);
        }

        // Render all the blocks, starting from the previously
        RawModelFactory::render(block->vao, block->total_index_count,
            (RawModelMaterial*)materials[this->mode],
            start, glm::vec3(1, 1, 1),
            model_matrix, glm::mat4(), shader, objectToWorldMatrix, projectionMatrix, cameraToWorldMatrix, modelViewProjectionMatrix, objectToWorldNormalMatrix, uniformBindingPoint, uniformBlock, uniformOffset);

        RawModelFactory::render(block->vao, block->total_index_count,
            (RawModelMaterial*)materials[this->mode],
            start + glm::vec3(-this->length, 0, 0),
            glm::vec3(1, 1, 1),
            model_matrix, glm::mat4(), shader, objectToWorldMatrix, projectionMatrix, cameraToWorldMatrix, modelViewProjectionMatrix, objectToWorldNormalMatrix, uniformBindingPoint, uniformBlock, uniformOffset);

        RawModelFactory::render(block->vao, block->total_index_count,
            (RawModelMaterial*)materials[this->mode],
            start + glm::vec3(0, 0, -this->length),
            glm::vec3(1, 1, 1),
            model_matrix, glm::mat4(), shader, objectToWorldMatrix, projectionMatrix, cameraToWorldMatrix, modelViewProjectionMatrix, objectToWorldNormalMatrix, uniformBindingPoint, uniformBlock, uniformOffset);

        RawModelFactory::render(block->vao, block->total_index_count,
            (RawModelMaterial*)materials[this->mode],
            start + glm::vec3(-this->length, 0, -this->length),
            glm::vec3(1, 1, 1),
            model_matrix, glm::mat4(), shader, objectToWorldMatrix, projectionMatrix, cameraToWorldMatrix, modelViewProjectionMatrix, objectToWorldNormalMatrix, uniformBindingPoint, uniformBlock, uniformOffset);

        // Inform the shader we're no longer drawing a mountain.
        glUniform1i(glGetUniformLocation(shader, "draw_mountain"), false);
    }
}

// Generate the base, flat terrain.
void World::generateBase(unsigned int mode, unsigned int square_count) {
    WorldBlock* block = this->initializeBlock(mode, square_count);
    unsigned int i;

    // All the vertices are initialized with Infinity height, fix that.
    for (i = 0; i < block->total_vertex_count; i++) {
        block->vertices[i].position.y = this->position.y;
    }

    // Compute the vertex normals.
    this->computeNormals(mode);
}

// Generate simple, non-tessellated fractal terrain.
void World::generateTerrain(unsigned int mode, unsigned int square_count) {
    // Calculate the number of iterations. The total square count is
    // 4 ^ iterations, so starting from the desired number of quads we can
    // determine how many iterations should we execute.
    int iterations = (int)floor(log2(square_count * square_count) / 2.0f);
    int actual_square_count = (int)pow(2, iterations);
    float min = 0, max = 0, range;

    WorldBlock* block = this->initializeBlock(mode, actual_square_count);

    // "Fractalize" the terrain.
    this->generateFractal(mode, iterations);

    // Go through the vertices and offset them, to correct the height.
    // We also find the minimun and maximum height generated.
    for (unsigned int i = 0; i < block->total_vertex_count; i++) {
        block->vertices[i].position.y += WORLD_FRACTAL_Y_OFFSET;

        if (block->vertices[i].position.y > max)
            max = block->vertices[i].position.y;
        if (block->vertices[i].position.y < min)
            min = block->vertices[i].position.y;
    }
    range = max - min;

    // Compute the color boundaries for the mountains.
    this->boundary_top = glm::vec2(min + range * WORLD_BOUNDARY_TOP,
        max * WORLD_BOUNDARY_TOP_HIGH);
    this->boundary_bottom = glm::vec2(min * (WORLD_BOUNDARY_BOTTOM_LOW + 1),
        min + range * WORLD_BOUNDARY_BOTTOM);

    // Compute normals.
    this->computeNormals(mode);
}

// To tessellate a generated block, apply the fractal algorithm once more, to multiply the number of quads by 4.
void World::tessellateTerrain(unsigned int source_mode, unsigned int mode) {
    WorldBlock* source_block = this->blocks[source_mode];
    int square_count = source_block->square_count * 2;
    WorldBlock* block = this->initializeBlock(mode, square_count);
    unsigned int i, j;

    // Copy the height of already generated vertices in the source to
    // the new block. As you can see, we jump by 2 both in columns and
    // rows, to achieve that.
    for (i = 0, j = 0; i < source_block->total_vertex_count; i++) {
        block->vertices[j].position.y = source_block->vertices[i].position.y;

        if ((i + 1) % source_block->vertex_count == 0)
            j += 1 + block->vertex_count;
        else j += 2;
    }

    // "Fractalize" the terrain for one more iteration.
    this->generateFractal(mode, 1);

    // Compute normals.
    this->computeNormals(mode);
}

// Generates a fractal.
void World::generateFractal(unsigned int mode, unsigned int iterations) {
    float sum;
    WorldBlock* block = this->blocks[mode];
    unsigned int vertex_count = block->vertex_count;
    unsigned int vertex_limit = block->square_count;
    unsigned int i, j, k, halfstep, count, index;
    unsigned int step = (int)pow(2, iterations);

    // The total number of iterations, according to the vertex count.
    int total_iterations = (int)log2(block->vertex_count);
    int iteration_diff = total_iterations - iterations;
    float displacement_range = WORLD_FRACTAL_DISPLACEMENT_RANGE /
        (float)pow(2, iteration_diff);

    // Initialize the random floating point numbers generator.
    std::uniform_real_distribution<> fractal_seed(
        WORLD_FRACTAL_DISPLACEMENT_RANGE / 4.0f, WORLD_FRACTAL_DISPLACEMENT_RANGE);

    // Initialize the world corners, if they are not already.
    if (block->vertices[0].position.y == WORLD_INFINITY) {
        block->vertices[0].position.y =
            block->vertices[vertex_limit].position.y =
            block->vertices[vertex_limit * vertex_count].position.y =
            block->vertices[vertex_count * vertex_count - 1].position.y =
            (float)fractal_seed(rgn);
    }

    // Iterate the number of times requested.
    // With each iteration, the step halves, as well as the displacement range.
    // Check out the links at the top for explanations.
    for (k = 0; k < iterations;
        k++, step = (int)(step / 2), displacement_range /= 2.0f) {
        halfstep = step / 2;

        // Initialize the current random number generator.
        std::uniform_real_distribution<> displacement(-displacement_range,
            displacement_range);

        // The diamond step.
        for (i = halfstep; i < vertex_count - halfstep; i += step) {
            for (j = halfstep; j < vertex_count - halfstep; j += step) {
                index = i * vertex_count + j;

                // Only compute this value if the vertex is not initialized.
                if (block->vertices[index].position.y == WORLD_INFINITY) {
                    sum = 0;

                    // Sum the corner values and average.
                    sum += block->vertices[(i - halfstep) * vertex_count +
                        (j - halfstep)].position.y;
                    sum += block->vertices[(i + halfstep) * vertex_count +
                        (j - halfstep)].position.y;
                    sum += block->vertices[(i - halfstep) * vertex_count +
                        (j + halfstep)].position.y;
                    sum += block->vertices[(i + halfstep) * vertex_count +
                        (j + halfstep)].position.y;

                    block->vertices[index].position.y =
                        (sum / 4.0f) + (float)displacement(rgn);
                }
            }
        }

        // Square step.
        for (i = 0; i < vertex_count; i += halfstep) {
            for (j = 0; j < vertex_count; j += halfstep) {
                index = i * vertex_count + j;

                // Initialize the vertex only if it is required.
                if (block->vertices[index].position.y == WORLD_INFINITY &&
                    (i + j) % step != 0) {
                    sum = 0;
                    count = 0;

                    // To ensure our world is wrapping, for vertices on the
                    // block margin we also consider the vertex on the other
                    // side, in computations. Again, more information in the
                    // link provided.
                    if (i > 0)
                        sum += block->vertices[(i - halfstep) * vertex_count +
                        j].position.y;
                    else
                        sum += block->vertices[(vertex_limit - halfstep) *
                        vertex_count + j].position.y;

                    if (i < vertex_limit)
                        sum += block->vertices[(i + halfstep) * vertex_count +
                        j].position.y;
                    else
                        sum += block->vertices[halfstep * vertex_count + j]
                        .position.y;

                    if (j < vertex_limit)
                        sum += block->vertices[i * vertex_count +
                        (j + halfstep)].position.y;
                    else
                        sum += block->vertices[i * vertex_count +
                        halfstep].position.y;

                    if (j > 0)
                        sum += block->vertices[i * vertex_count +
                        (j - halfstep)].position.y;
                    else
                        sum += block->vertices[i * vertex_count +
                        (vertex_limit - halfstep)].position.y;

                    block->vertices[index].position.y =
                        (sum / 4.0f) + (float)displacement(rgn);

                    // If we're on the margin, also duplicate this
                    // values on the other side of the block, to ensure
                    // the block wraps.
                    if (i == 0)
                        block->vertices[vertex_limit * vertex_count + j]
                        .position.y = block->vertices[index].position.y;
                    if (i == vertex_limit)
                        block->vertices[j].position.y =
                        block->vertices[index].position.y;
                    if (j == 0)
                        block->vertices[i * vertex_count + vertex_limit]
                        .position.y = block->vertices[index].position.y;
                    if (j == vertex_limit)
                        block->vertices[i * vertex_count].position.y =
                        block->vertices[index].position.y;
                }
            }
        }
    }

    // Compute normals.
    this->computeNormals(mode);
}

// Initializing a block also computes all necessarry values and creates
// the indexes list.
WorldBlock* World::initializeBlock(unsigned int mode,
    unsigned int square_count) {
    WorldBlock* block = new WorldBlock();
    unsigned int i, j, k, l, m, n;

    this->blocks[mode] = block;

    // Compute various values used in further calculations.
    block->square_count = square_count;
    block->square_size = this->length / block->square_count;
    block->vertex_count = block->square_count + 1;
    block->total_square_count = block->square_count * block->square_count;
    block->total_vertex_count = block->vertex_count * block->vertex_count;
    block->total_triangle_count = block->total_square_count * 2;
    block->total_index_count = block->total_triangle_count * 3;

    // Initialize lists for vertices and indexes.
    block->vertices = (WorldVertex*)malloc(sizeof(WorldVertex)*
        block->total_vertex_count);
    block->indexes = (unsigned int*)malloc(sizeof(unsigned int)*
        block->total_index_count);

    // Iterate through the vertex matrix and compute indexes.
    for (i = 0, k = 0, n = 0; i < block->vertex_count; i++) {
        for (j = 0; j < block->vertex_count; j++, k++) {
            // Insert the indexes in the list for each triangle in each
            // quad.
            if (i < block->square_count) {
                if (j > 0) {
                    l = k + block->vertex_count;
                    m = l - 1;

                    block->indexes[n] = k; n++;
                    block->indexes[n] = l; n++;
                    block->indexes[n] = m; n++;
                }

                if (j < block->square_count) {
                    l = k + 1;
                    m = k + block->vertex_count;

                    block->indexes[n] = k; n++;
                    block->indexes[n] = l; n++;
                    block->indexes[n] = m; n++;
                }
            }

            // Instantiate the current vertex.
            block->vertices[k] = WorldVertex(glm::vec3(i * block->square_size,
                WORLD_INFINITY, j * block->square_size));
        }
    }

    return block;
}

// To compute vertex normals, we first compute triangle normals and then
// average those for each vertex.
void World::computeNormals(unsigned int mode) {
    unsigned int i, j, k, l, m, n;
    WorldBlock* block = this->blocks[mode];
    WorldVertex *p1, *p2, *p3;
    glm::vec3 v1, v2;
    glm::vec3 normal;

    // First calculate the normal for each triangle defined by 3 consecutive
    // indexes in the index array.
    for (i = 0, j = 0; i < block->total_index_count; i += 3, j++) {
        p1 = &(block->vertices[block->indexes[i]]);
        p2 = &(block->vertices[block->indexes[i + 1]]);
        p3 = &(block->vertices[block->indexes[i + 2]]);

        // Compute 2 vector sides of the triangle.
        v1 = p2->position - p1->position;
        v2 = p3->position - p1->position;

        // Cross product of 2 vectors gives us the orthogonal vector = normal.
        normal = glm::cross(v1, v2);

        // For each vertex of this triangle, add the triangle normal to their
        // normal. Don't normalize yet, as this is only a partial result.
        for (k = 0; k < 3; k++) {
            block->vertices[block->indexes[i + k]].normal += normal;
        }
    }

    // Iterate through each vertex in the block and normalize its normal,
    // to average the normals of all the triangles it is a part of.
    // For vertices on the margin, we also take into account the normal
    // of the opposite vertex, so we can get rid of any dividing lines
    // visible when rendering wrapped blocks.
    for (i = 0; i < block->square_count; i++) {
        for (j = 0; j < block->square_count; j++) {
            k = i * block->vertex_count + j;

            if (i == 0 && j == 0) {
                l = block->square_count;
                m = block->square_count * block->vertex_count;
                n = block->vertex_count * block->vertex_count - 1;

                block->vertices[k].normal = block->vertices[l].normal =
                    block->vertices[m].normal = block->vertices[n].normal =
                    glm::normalize(block->vertices[k].normal +
                    block->vertices[l].normal + block->vertices[m].normal +
                    block->vertices[n].normal);
            }
            else if (i == 0) {
                l = block->square_count * block->vertex_count + j;
                block->vertices[k].normal = block->vertices[l].normal =
                    glm::normalize(block->vertices[k].normal +
                    block->vertices[l].normal);
            }
            else if (j == 0) {
                l = i * block->vertex_count + block->square_count;
                block->vertices[k].normal = block->vertices[l].normal =
                    glm::normalize(block->vertices[k].normal +
                    block->vertices[l].normal);
            }
            else {
                block->vertices[k].normal = glm::normalize(block->vertices[k].normal);
            }
        }
    }
}

glm::vec3 World::getBlockPos(glm::vec3 pos)
{
	return glm::vec3();
}

// Binds the buffers for later rendering.
void World::bufferData() {
    WorldBlock* block;
    unsigned int i;

    // Bind buffers for each world mode.
    for (i = 0; i < WORLD_MODE_COUNT; i++) {
        block = this->blocks[i];

        if (block) {
            glGenVertexArrays(1, &(block->vao));
            glBindVertexArray(block->vao);

            glGenBuffers(1, &(block->vbo));
            glBindBuffer(GL_ARRAY_BUFFER, block->vbo);
            glBufferData(GL_ARRAY_BUFFER, block->total_vertex_count *
                sizeof(WorldVertex), block->vertices, GL_STATIC_DRAW);

            glGenBuffers(1, &(block->ibo));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, block->ibo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, block->total_index_count *
                sizeof(unsigned int), block->indexes, GL_STATIC_DRAW);

            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(WorldVertex),
                (void*)0);
            glEnableVertexAttribArray(7);
			glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE , sizeof(WorldVertex),
                (void*)(3 * sizeof(float)));

            free(block->vertices);
            free(block->indexes);
        }
    }
}