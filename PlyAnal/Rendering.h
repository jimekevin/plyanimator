#pragma once

#ifndef _RENDERING_HEADER_
#define _RENDERING_HEADER_

#if KEVIN_MACOS
#pragma message("Included on Mac OS")
#endif

#define GL_GLEXT_PROTOTYPES

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <set>
#include <map>

//#define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <opencv2/core/core.hpp>

namespace rendering {

	struct MouseState {
		cv::Vec2f pos;

		bool leftPressed{ false };
		bool rightPressed{ false };
	};

	class ViewerInput {
	public:
		static ViewerInput& get();

		// Callback methods.
		static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

		void mouseButtonCallbackImpl(int button, int action);

		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

		void keyCallbackImpl(int key, int action);

		static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);

		void scrollCallbackImpl(double xOffset, double yOffset);

		static void cursorPosCallback(GLFWwindow* window, double x, double y);

		void cursorPosCallbackImpl(double x, double y);

		static void errorCallback(int error, const char* description);

		// Resets change events.
		void resetChangeEvents();

		// Event querries.
		bool isKeyDown(int key) const;

		bool isKeyUp(int key) const;

		bool isKeyPressed(int key) const;

		bool isMouseLeftDown() const;

		bool isMouseLeftUp() const;

		bool isMouseMoved() const;

		bool isScrollChanged() const;

		const MouseState& getMouse() const;

		const MouseState& getMousePrev() const;

		double getScrollChangeX() const;

		double getScrollChangeY() const;

	private:
		std::set<int> m_keysDown;
		std::set<int> m_keysUp;
		std::map<int, bool> m_keysPressed;

		MouseState m_mousePrev;
		MouseState m_mouse;

		double m_scrollChangeX{ 0.0 };
		double m_scrollChangeY{ 0.0 };

		bool m_bMouseMoved{ false };
		bool m_bMouseLeftUp{ false };
		bool m_bMouseLeftDown{ false };
		bool m_bMouseRightUp{ false };
		bool m_bMouseRightDown{ false };
		bool m_bScrollChange{ false };

		ViewerInput(void);

		ViewerInput(ViewerInput const&);
		void operator=(ViewerInput const&);
	};

	class Rendering {
	private:
		GLFWwindow* window = nullptr;
		//glfw_state* app_state;

		GLuint defaultProgram;
		unsigned int VBO, VAO, EBO;

		std::string readFile(const char* filePath);

	public:
		Rendering(int screenWidth, int screenHeight, std::string title/* glfw_state* app_sate*/);
		~Rendering();

		GLFWwindow*getWindow() const;

		GLuint loadShader(const char* vertex_path, const char* fragment_path);

		void printGLVersion();

		void loadPolygon(float* vertices, int verticesSize, unsigned int* indices, int indicesSize);

		void setInputCallback(std::function<void(GLFWwindow*)> callback);

		void clearScreen();

		void draw(GLuint program);
	};
};

#endif /* _RENDERING_HEADER_ */