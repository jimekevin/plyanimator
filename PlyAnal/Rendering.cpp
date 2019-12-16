#pragma once

#include "Rendering.h"


rendering::ViewerInput& rendering::ViewerInput::get() // Singleton is accessed via get()
{
	static ViewerInput instance; // lazy singleton, instantiated on first use
	return instance;
}

// Callback methods.
void rendering::ViewerInput::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	get().mouseButtonCallbackImpl(button, action);
}

void rendering::ViewerInput::mouseButtonCallbackImpl(int button, int action) {
	if (button == GLFW_MOUSE_BUTTON_1) {
		if (action == GLFW_PRESS) {
			m_bMouseLeftDown = true;
			m_mouse.leftPressed = true;
		}
		else if (action == GLFW_RELEASE) {
			m_bMouseLeftUp = true;
			m_mouse.leftPressed = false;
		}
	}
	if (button == GLFW_MOUSE_BUTTON_1) {
		if (action == GLFW_PRESS) {
			m_bMouseLeftDown = true;
			m_mouse.rightPressed = true;
		}
		else if (action == GLFW_RELEASE) {
			m_bMouseLeftUp = true;
			m_mouse.rightPressed = false;
		}
	}
}

void rendering::ViewerInput::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, 1);

	//here we access the instance via the singleton pattern and forward the callback to the instance method
	get().keyCallbackImpl(key, action);
}

void rendering::ViewerInput::keyCallbackImpl(int key, int action) {
	if (action == GLFW_PRESS) {
		m_keysPressed[key] = true;
		m_keysDown.insert(key);
	}
	else if (action == GLFW_RELEASE) {
		m_keysPressed[key] = false;
		m_keysUp.insert(key);
	}
}

void rendering::ViewerInput::scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
	get().scrollCallbackImpl(xOffset, yOffset);
}

void rendering::ViewerInput::scrollCallbackImpl(double xOffset, double yOffset) {
	m_scrollChangeX = xOffset;
	m_scrollChangeY = yOffset;
	m_bScrollChange = true;
}

void rendering::ViewerInput::cursorPosCallback(GLFWwindow* window, double x, double y) {
	get().cursorPosCallbackImpl(x, y);
}

void rendering::ViewerInput::cursorPosCallbackImpl(double x, double y) {
	m_bMouseMoved = true;
	m_mouse.pos = cv::Vec2f(x, y);
}

void rendering::ViewerInput::errorCallback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}

// Resets change events.
void rendering::ViewerInput::resetChangeEvents() {
	m_keysUp.clear();
	m_keysDown.clear();

	m_bMouseMoved = false;
	m_bMouseLeftUp = false;
	m_bMouseLeftDown = false;
	m_bMouseRightUp = false;
	m_bMouseRightDown = false;
	m_bScrollChange = false;

	m_mousePrev = m_mouse;
	m_scrollChangeX = 0.0;
	m_scrollChangeY = 0.0;
}

// Event querries.
bool rendering::ViewerInput::isKeyDown(int key) const {
	return m_keysDown.find(key) != m_keysDown.end();
}

bool rendering::ViewerInput::isKeyUp(int key) const {
	return m_keysUp.find(key) != m_keysUp.end();
}

bool rendering::ViewerInput::isKeyPressed(int key) const {
	return m_keysPressed.find(key) != m_keysPressed.end() && m_keysPressed.find(key)->second;
}

bool rendering::ViewerInput::isMouseLeftDown() const {
	return m_bMouseLeftDown;
}

bool rendering::ViewerInput::isMouseLeftUp() const {
	return m_bMouseLeftUp;
}

bool rendering::ViewerInput::isMouseMoved() const {
	return m_bMouseMoved;
}

bool rendering::ViewerInput::isScrollChanged() const {
	return m_bScrollChange;
}

const rendering::MouseState& rendering::ViewerInput::getMouse() const {
	return m_mouse;
}

const rendering::MouseState& rendering::ViewerInput::getMousePrev() const {
	return m_mousePrev;
}

double rendering::ViewerInput::getScrollChangeX() const {
	return m_scrollChangeX;
}

double rendering::ViewerInput::getScrollChangeY() const {
	return m_scrollChangeY;
}

rendering::ViewerInput::ViewerInput(void) // private constructor necessary to allow only 1 instance
{ }

std::string rendering::Rendering::readFile(const char* filePath) {
	std::string content;
	std::ifstream fileStream(filePath, std::ios::in);

	if (!fileStream.is_open()) {
		std::cerr << "Could not read file " << filePath << ". File does not exist." << std::endl;
		return "";
	}

	std::string line = "";
	while (!fileStream.eof()) {
		std::getline(fileStream, line);
		content.append(line + "\n");
	}

	fileStream.close();
	return content;
}

GLuint rendering::Rendering::loadShader(const char* vertex_path, const char* fragment_path) {
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Read shaders

	std::string vertShaderStr = readFile(vertex_path);
	std::string fragShaderStr = readFile(fragment_path);
	if (vertShaderStr.length() == 0 || fragShaderStr.length() == 0) {
		return 0;
	}
	const char* vertShaderSrc = vertShaderStr.c_str();
	const char* fragShaderSrc = fragShaderStr.c_str();

	GLint result = GL_FALSE;
	int logLength;

	// Compile vertex shader

	std::cout << "Compiling vertex shader" << std::endl;
	glShaderSource(vertShader, 1, &vertShaderSrc, NULL);
	glCompileShader(vertShader);

	// Check vertex shader

	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &result);
	if (!result) {
		glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &logLength);
		std::vector<char> vertShaderError((logLength > 1) ? logLength : 1);
		glGetShaderInfoLog(vertShader, logLength, NULL, &vertShaderError[0]);
		std::cerr << &vertShaderError[0] << std::endl;
		return 0;
	}

	// Compile fragment shader

	std::cout << "Compiling fragment shader" << std::endl;
	glShaderSource(fragShader, 1, &fragShaderSrc, NULL);
	glCompileShader(fragShader);

	// Check fragment shader

	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &result);
	if (!result) {
		glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &logLength);
		std::vector<char> fragShaderError((logLength > 1) ? logLength : 1);
		glGetShaderInfoLog(fragShader, logLength, NULL, &fragShaderError[0]);
		std::cerr << &fragShaderError[0] << std::endl;
		return 0;
	}

	std::cout << "Linking program" << std::endl;
	GLuint program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
	std::vector<char> programError((logLength > 1) ? logLength : 1);
	glGetProgramInfoLog(program, logLength, NULL, &programError[0]);
	std::cout << &programError[0] << std::endl;

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	return program;
}

void rendering::Rendering::printGLVersion() {
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* vendor = glGetString(GL_VENDOR);
	const GLubyte* version = glGetString(GL_VERSION);
	const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	printf("GL Vendor : %s\n", vendor);
	printf("GL Renderer : %s\n", renderer);
	printf("GL Version (string) : %s\n", version);
	printf("GL Version (integer) : %d.%d\n", major, minor);
	printf("GLSL Version : %s\n", glslVersion);
}

rendering::Rendering::Rendering(int screenWidth, int screenHeight, std::string title/* glfw_state* app_sate*/)
//: app_state(app_state)
{
	/*GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "Error initializing GLEW: %s\n", glewGetErrorString(err));
	}*/

	if (!glfwInit()) {
		throw std::exception("Loading glfw failed!");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	window = glfwCreateWindow(screenWidth, screenHeight, title.c_str(), NULL, NULL);
	if (!window) {
		glfwTerminate();
		throw std::exception("Could not create glfw window!");
	}

	// Set event callbacks.
	glfwSetKeyCallback(window, ViewerInput::keyCallback);
	glfwSetMouseButtonCallback(window, ViewerInput::mouseButtonCallback);
	glfwSetScrollCallback(window, ViewerInput::scrollCallback);
	glfwSetCursorPosCallback(window, ViewerInput::cursorPosCallback);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (!gladLoadGL()) {;
		throw std::exception("Loading glad failed!");
	}

	glfwSetErrorCallback([](int error, const char* description) {
		fprintf(stderr, "glfwError: %s\n", description);
	});
}

GLFWwindow *rendering::Rendering::getWindow() const {
	return window;
}

void rendering::Rendering::loadPolygon(float *vertices, int verticesSize, unsigned int *indices, int indicesSize) {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, verticesSize, vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize, indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void rendering::Rendering::setInputCallback(std::function<void (GLFWwindow *)> callback) {
	auto key = glfwGetKey(window, GLFW_KEY_ESCAPE);
	if (key == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	callback(window);
}

void rendering::Rendering::clearScreen() {
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void rendering::Rendering::draw(GLuint program) {
	// draw our first triangle
	glUseProgram(program);
	glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
	//glDrawArrays(GL_TRIANGLES, 0, 6);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	// glBindVertexArray(0); // no need to unbind it every time 
}

rendering::Rendering::~Rendering() {
	// optional: de-allocate all resources once they've outlived their purpose:
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	glfwTerminate();
}