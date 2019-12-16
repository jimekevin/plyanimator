#include <thread>
#include <chrono>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstring>
#include <iterator>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "tinyply.h"
using namespace tinyply;

#include "Rendering.h"
using namespace rendering;

inline std::vector<uint8_t> read_file_binary(const std::string& pathToFile)
{
    std::ifstream file(pathToFile, std::ios::binary);
    std::vector<uint8_t> fileBufferBytes;

    if (file.is_open())
    {
        file.seekg(0, std::ios::end);
        size_t sizeBytes = file.tellg();
        file.seekg(0, std::ios::beg);
        fileBufferBytes.resize(sizeBytes);
        if (file.read((char*)fileBufferBytes.data(), sizeBytes)) return fileBufferBytes;
    }
    else throw std::runtime_error("could not open binary ifstream to path " + pathToFile);
    return fileBufferBytes;
}

struct memory_buffer : public std::streambuf
{
    memory_buffer(char const* base, size_t size)
    {
        char* p(const_cast<char*>(base));
        this->setg(p, p, p + size);
    }
};

struct memory_stream : virtual memory_buffer, public std::istream
{
    memory_stream(char const* base, size_t size)
        : memory_buffer(base, size), std::istream(static_cast<std::streambuf*>(this)) {}
};

class manual_timer
{
    std::chrono::high_resolution_clock::time_point t0;
    double timestamp{ 0.f };
public:
    void start() { t0 = std::chrono::high_resolution_clock::now(); }
    void stop() { timestamp = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - t0).count() * 1000; }
    const double& get() { return timestamp; }
};

struct float2 { float x, y; };
struct float3 { float x, y, z; };
struct double3 { double x, y, z; };
struct uint3 { uint32_t x, y, z; };
struct uint4 { uint32_t x, y, z, w; };

struct geometry
{
    std::vector<float3> vertices;
    std::vector<float3> normals;
    std::vector<float2> texcoords;
    std::vector<uint3> triangles;
};

inline geometry make_cube_geometry()
{
    geometry cube;

    const struct CubeVertex { float3 position, normal; float2 texCoord; } verts[] = {
    { { -1, -1, -1 },{ -1, 0, 0 },{ 0, 0 } },{ { -1, -1, +1 },{ -1, 0, 0 },{ 1, 0 } },{ { -1, +1, +1 },{ -1, 0, 0 },{ 1, 1 } },{ { -1, +1, -1 },{ -1, 0, 0 },{ 0, 1 } },
    { { +1, -1, +1 },{ +1, 0, 0 },{ 0, 0 } },{ { +1, -1, -1 },{ +1, 0, 0 },{ 1, 0 } },{ { +1, +1, -1 },{ +1, 0, 0 },{ 1, 1 } },{ { +1, +1, +1 },{ +1, 0, 0 },{ 0, 1 } },
    { { -1, -1, -1 },{ 0, -1, 0 },{ 0, 0 } },{ { +1, -1, -1 },{ 0, -1, 0 },{ 1, 0 } },{ { +1, -1, +1 },{ 0, -1, 0 },{ 1, 1 } },{ { -1, -1, +1 },{ 0, -1, 0 },{ 0, 1 } },
    { { +1, +1, -1 },{ 0, +1, 0 },{ 0, 0 } },{ { -1, +1, -1 },{ 0, +1, 0 },{ 1, 0 } },{ { -1, +1, +1 },{ 0, +1, 0 },{ 1, 1 } },{ { +1, +1, +1 },{ 0, +1, 0 },{ 0, 1 } },
    { { -1, -1, -1 },{ 0, 0, -1 },{ 0, 0 } },{ { -1, +1, -1 },{ 0, 0, -1 },{ 1, 0 } },{ { +1, +1, -1 },{ 0, 0, -1 },{ 1, 1 } },{ { +1, -1, -1 },{ 0, 0, -1 },{ 0, 1 } },
    { { -1, +1, +1 },{ 0, 0, +1 },{ 0, 0 } },{ { -1, -1, +1 },{ 0, 0, +1 },{ 1, 0 } },{ { +1, -1, +1 },{ 0, 0, +1 },{ 1, 1 } },{ { +1, +1, +1 },{ 0, 0, +1 },{ 0, 1 } } };

    std::vector<uint4> quads = { { 0, 1, 2, 3 },{ 4, 5, 6, 7 },{ 8, 9, 10, 11 },{ 12, 13, 14, 15 },{ 16, 17, 18, 19 },{ 20, 21, 22, 23 } };

    for (auto& q : quads)
    {
        cube.triangles.push_back({ q.x,q.y,q.z });
        cube.triangles.push_back({ q.x,q.z,q.w });
    }

    for (int i = 0; i < 24; ++i)
    {
        cube.vertices.push_back(verts[i].position);
        cube.normals.push_back(verts[i].normal);
        cube.texcoords.push_back(verts[i].texCoord);
    }

    return cube;
}

void write_ply_example(const std::string& filename)
{
    geometry cube = make_cube_geometry();

    std::filebuf fb_binary;
    fb_binary.open(filename + "-binary.ply", std::ios::out | std::ios::binary);
    std::ostream outstream_binary(&fb_binary);
    if (outstream_binary.fail()) throw std::runtime_error("failed to open " + filename);

    std::filebuf fb_ascii;
    fb_ascii.open(filename + "-ascii.ply", std::ios::out);
    std::ostream outstream_ascii(&fb_ascii);
    if (outstream_ascii.fail()) throw std::runtime_error("failed to open " + filename);

    PlyFile cube_file;

    cube_file.add_properties_to_element("vertex", { "x", "y", "z" },
        Type::FLOAT32, cube.vertices.size(), reinterpret_cast<uint8_t*>(cube.vertices.data()), Type::INVALID, 0);

    cube_file.add_properties_to_element("vertex", { "nx", "ny", "nz" },
        Type::FLOAT32, cube.normals.size(), reinterpret_cast<uint8_t*>(cube.normals.data()), Type::INVALID, 0);

    cube_file.add_properties_to_element("vertex", { "u", "v" },
        Type::FLOAT32, cube.texcoords.size(), reinterpret_cast<uint8_t*>(cube.texcoords.data()), Type::INVALID, 0);

    cube_file.add_properties_to_element("face", { "vertex_index" },
        Type::UINT32, cube.triangles.size(), reinterpret_cast<uint8_t*>(cube.triangles.data()), Type::UINT8, 3);

    cube_file.get_comments().push_back("generated by tinyply 2.2");

    // Write an ASCII file
    cube_file.write(outstream_ascii, false);

    // Write a binary file
    cube_file.write(outstream_binary, true);
}
// Tinyply treats parsed data as untyped byte buffers. See below for examples.
std::shared_ptr<PlyData> vertices, normals, faces; // , texcoords;

void read_ply_file(const std::string& filepath, const bool preload_into_memory = false)
{
    std::unique_ptr<std::istream> file_stream;
    std::vector<uint8_t> byte_buffer;

    try
    {
        // For most files < 1gb, pre-loading the entire file upfront and wrapping it into a 
        // stream is a net win for parsing speed, about 40% faster. 
        if (preload_into_memory)
        {
            byte_buffer = read_file_binary(filepath);
            file_stream.reset(new memory_stream((char*)byte_buffer.data(), byte_buffer.size()));
        }
        else
        {
            file_stream.reset(new std::ifstream(filepath, std::ios::binary));
        }

        if (!file_stream || file_stream->fail()) throw std::runtime_error("failed to open " + filepath);

        PlyFile file;
        file.parse_header(*file_stream);

        std::cout << "........................................................................\n";
        for (const auto& c : file.get_comments())
        {
            std::cout << "Comment: " << c << std::endl;
        }
        for (const auto& e : file.get_elements())
        {
            std::cout << "element - " << e.name << " (" << e.size << ")" << std::endl;
            for (const auto& p : e.properties)
            {
                std::cout << "\tproperty - " << p.name << " (" << tinyply::PropertyTable[p.propertyType].str << ")" << std::endl;
            }
        }
        std::cout << "........................................................................\n";

        // The header information can be used to programmatically extract properties on elements
        // known to exist in the header prior to reading the data. For brevity of this sample, properties 
        // like vertex position are hard-coded: 
        try { vertices = file.request_properties_from_element("vertex", { "x", "y", "z" }); }
        catch (const std::exception & e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }

        try { normals = file.request_properties_from_element("vertex", { "nx", "ny", "nz" }); }
        catch (const std::exception & e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }

        //try { texcoords = file.request_properties_from_element("vertex", { "u", "v" }); }
        //catch (const std::exception & e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }

        // Providing a list size hint (the last argument) is a 2x performance improvement. If you have 
        // arbitrary ply files, it is best to leave this 0. 
        try { faces = file.request_properties_from_element("face", { "vertex_index" }, 3); }
        catch (const std::exception & e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }

        manual_timer read_timer;

        read_timer.start();
        file.read(*file_stream);
        read_timer.stop();

        //std::cout << "Reading took " << read_timer.get() / 1000.f << " seconds." << std::endl;
        if (vertices) std::cout << "\tRead " << vertices->count << " total vertices " << std::endl;
        if (normals) std::cout << "\tRead " << normals->count << " total vertex normals " << std::endl;
        //if (texcoords) std::cout << "\tRead " << texcoords->count << " total vertex texcoords " << std::endl;
        if (faces) std::cout << "\tRead " << faces->count << " total faces (triangles) " << std::endl;

        // type casting to your own native types - Option A
        {
            const size_t numVerticesBytes = vertices->buffer.size_bytes();
            std::vector<float3> verts(vertices->count);
            std::memcpy(verts.data(), vertices->buffer.get(), numVerticesBytes);
        }

        // type casting to your own native types - Option B
        {
            std::vector<float3> verts_floats;
            std::vector<double3> verts_doubles;
            if (vertices->t == tinyply::Type::FLOAT32) { /* as floats ... */ }
            if (vertices->t == tinyply::Type::FLOAT64) { /* as doubles ... */ }
        }
    }
    catch (const std::exception & e)
    {
        std::cerr << "Caught tinyply exception: " << e.what() << std::endl;
    }
}

#define SCREEN_WIDTH  1024
#define SCREEN_HEIGHT 760

int main(int argc, char* argv[]) try {
    //const std::string filepath = "C:/Users/Kevin Bein/Desktop/Hologram.ply";
    //const std::string filepath = "C:/Users/Kevin Bein/Desktop/Bone_ascii.ply";
    //const std::string filepath = "C:/Users/Kevin Bein/Desktop/Bone.ply";
    const std::string filepath = "C:/Users/Kevin Bein/Desktop/Bone_binary_normals.ply";
    //const std::string filepath = "C:/Users/Kevin Bein/Desktop/simple.ply";
    
    read_ply_file(filepath);

    std::unique_ptr<std::istream> file_stream;
    file_stream.reset(new std::ifstream(filepath, std::ios::binary));
    if (!file_stream || file_stream->fail()) {
        throw std::runtime_error("failed to open " + filepath);
    }

    PlyFile plyf;
    plyf.parse_header(*file_stream);
    auto info = plyf.get_info();

    auto vertices_ply = plyf.request_properties_from_element("vertex", { "x", "y", "z" });
    auto normals_ply = plyf.request_properties_from_element("vertex", { "nx", "ny", "nz" });
    auto faces_ply = plyf.request_properties_from_element("face", { "vertex_indices" }, 3);

    plyf.read(*file_stream);

    const size_t numVerticesBytes = vertices_ply->buffer.size_bytes();
    std::vector<float> vertices(vertices_ply->count * 3);
    std::memcpy(vertices.data(), vertices_ply->buffer.get(), numVerticesBytes);

    const size_t numNormalsBytes = normals_ply->buffer.size_bytes();
    std::vector<float> normals(normals_ply->count * 3);
    std::memcpy(normals.data(), normals_ply->buffer.get(), numNormalsBytes);

    const size_t numFacesBytes = faces_ply->buffer.size_bytes();
    std::vector<unsigned int> faces(faces_ply->count * 3);
    std::memcpy(faces.data(), faces_ply->buffer.get(), numFacesBytes);

    std::cout << "vertices size: " << vertices_ply->buffer.size_bytes() << std::endl;
    std::cout << "normals size: " << normals_ply->buffer.size_bytes() << std::endl;
    std::cout << "faces size: " << faces_ply->buffer.size_bytes() << std::endl;

    std::vector<float> faces_triangles(faces_ply->buffer.size_bytes());
    int triangle_index = 0;
    int normal_index = 0;
    int totalConnectedTriangles = 0;
    glm::vec3 min_vertex = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    glm::vec3 max_vertex = { std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min() };
    for (int i = 0; i < vertices_ply->count; ++i) {
        min_vertex[0] = MIN(vertices[i * 3 + 0], min_vertex[0]);
        min_vertex[1] = MIN(vertices[i * 3 + 1], min_vertex[1]);
        min_vertex[2] = MIN(vertices[i * 3 + 2], min_vertex[2]);
        max_vertex[0] = MAX(vertices[i * 3 + 0], max_vertex[0]);
        max_vertex[1] = MAX(vertices[i * 3 + 1], max_vertex[1]);
        max_vertex[2] = MAX(vertices[i * 3 + 2], max_vertex[2]);
    }
    std::cout << "Min: (" << min_vertex[0] << "," << min_vertex[1] << "," << min_vertex[2] << ")" << std::endl;
    std::cout << "Max: (" << max_vertex[0] << "," << max_vertex[1] << "," << max_vertex[2] << ")" << std::endl;
    for (int i = 0; i < faces_ply->count; ++i) {
        faces_triangles.push_back(vertices[3 * faces[i * 3 + 0] + 0]);
        faces_triangles.push_back(vertices[3 * faces[i * 3 + 0] + 1]);
        faces_triangles.push_back(vertices[3 * faces[i * 3 + 0] + 2]);

        faces_triangles.push_back(vertices[3 * faces[i * 3 + 1] + 0]);
        faces_triangles.push_back(vertices[3 * faces[i * 3 + 1] + 1]);
        faces_triangles.push_back(vertices[3 * faces[i * 3 + 1] + 2]);

        faces_triangles.push_back(vertices[3 * faces[i * 3 + 2] + 0]);
        faces_triangles.push_back(vertices[3 * faces[i * 3 + 2] + 1]);
        faces_triangles.push_back(vertices[3 * faces[i * 3 + 2] + 2]);

        normal_index += 9;
        triangle_index += 9;
        totalConnectedTriangles += 3;
    }
    glm::vec3 verticesCenterTransformation = max_vertex - min_vertex / -2.0f;
    std::cout << "Center transformation: (" << verticesCenterTransformation.x << "," << verticesCenterTransformation.y << "," << verticesCenterTransformation.z << ")" << std::endl;
    glm::vec3 verticesScaling = 1.0f / (max_vertex - min_vertex);
    std::cout << "Scaling to (-1.0, 1.0): (" << verticesScaling[0] << "," << verticesScaling[1] << "," << verticesScaling[2] << ")" << std::endl;

    auto rend = new Rendering(SCREEN_WIDTH, SCREEN_HEIGHT, "Ply Animator");
    rend->printGLVersion();

    auto defaultProgram = rend->loadShader("shaders/vertices.vert", "shaders/vertices.frag");

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


    unsigned int VBO, VAO, EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(float), faces.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);



    /*float vertices2[] = {
     0.5f,  0.5f, 0.0f,  // top right
     0.5f, -0.5f, 0.0f,  // bottom right
    -0.5f, -0.5f, 0.0f,  // bottom left
    -0.5f,  0.5f, 0.0f   // top left 
    };
    unsigned int indices2[] = {  // note that we start from 0!
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };*/

    /*float vertices2[] = {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f
    }; 
    unsigned int indices2[] = {
        0, 1, 2,
        0, 2, 3,
        7, 6, 5,
        7, 5, 4,
        0, 4, 5,
        0, 5, 1,
        1, 5, 6,
        1, 6, 2,
        2, 6, 7,
        2, 7, 3,
        3, 7, 4,
        3, 4, 0,
    };*/

    /*unsigned int VBO2, VAO2, EBO2;

    std::cout << "sizeof(vertices2): " << sizeof(vertices2) << ", sizeof(indices2): " << sizeof(indices2) << std::endl;
    glGenVertexArrays(1, &VAO2);
    glGenBuffers(1, &VBO2);
    glGenBuffers(1, &EBO2);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO2);

    glBindBuffer(GL_ARRAY_BUFFER, VBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO2);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2), indices2, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);
    */



    auto window = rend->getWindow();
    while (!glfwWindowShouldClose(window))
    {
        // Process input
        rend->setInputCallback([](GLFWwindow* window) {
            if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }
        });

        rend->clearScreen();

        glUseProgram(defaultProgram);

        float timeValue = glfwGetTime() * 2.0;
        float greenValue = (sin(timeValue) / 2.0) + 0.5;
        int vertexColorLocation = glGetUniformLocation(defaultProgram, "vertexColor");
        glUniform4f(vertexColorLocation, 0.0, greenValue, 0.0, 1.0);

        glm::mat4 trans = glm::mat4(1.0);
        unsigned int transformLocation = glGetUniformLocation(defaultProgram, "transform");
        
        trans = glm::mat4(1.0f);
        trans = glm::rotate(trans, glm::radians(90.0f), glm::vec3(1.0, 1.0, 1.0));
        //trans = glm::scale(trans, glm::vec3(0.5, 0.5, 0.5));
        trans = glm::scale(trans, verticesScaling);
        //trans = glm::translate(trans, verticesCenterTransformation);
        glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(trans));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, totalConnectedTriangles * 3, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        if (rendering::ViewerInput::get().isMouseLeftDown()) {
            std::cout << "Mouse down" << std::endl;
        }
        else if (rendering::ViewerInput::get().isMouseLeftUp()) {
            std::cout << "Mouse up" << std::endl;
        }

        /*trans = glm::mat4(1.0f);
        trans = glm::rotate(trans, glm::radians(90.0f), glm::vec3(0.0, 1.0, 1.0));
        trans = glm::translate(trans, glm::vec3(-0.5, -0.5, -0.5));
        trans = glm::scale(trans, glm::vec3(0.5, 0.5, 0.5));
        glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(trans));

        glBindVertexArray(VAO2);
        glDrawElements(GL_TRIANGLES, 6 * 3, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);*/

        //glDrawElements(GL_TRIANGLES, totalConnectedTriangles, GL_UNSIGNED_INT, 0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    /* working triangle
    float vertices2[] = {
     0.5f,  0.5f, 0.0f,  // top right
     0.5f, -0.5f, 0.0f,  // bottom right
    -0.5f, -0.5f, 0.0f,  // bottom left
    -0.5f,  0.5f, 0.0f   // top left 
    };
    unsigned int indices2[] = {  // note that we start from 0!
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };
    //rend->loadPolygon(vertices2, sizeof(vertices2), indices2, sizeof(indices2));


    unsigned int VBO, VAO, EBO;
    auto verticesSize = sizeof(vertices2);
    auto indicesSize = sizeof(indices2);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verticesSize, vertices2, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize, indices2, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);



    auto window = rend->getWindow();
    while (!glfwWindowShouldClose(window))
    {
        // Process input
        rend->setInputCallback([](GLFWwindow* window) {
            if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }
        });

        rend->clearScreen();

        glUseProgram(defaultProgram);
        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        //glDrawArrays(GL_TRIANGLES, 0, 6);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    */

    delete rend;
    
    return EXIT_SUCCESS;
}
catch (const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}