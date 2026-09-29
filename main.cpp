#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<stb/stb_image.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#include"Texture.h"
#include"shaderClass.h"
#include"VAO.h"
#include"VBO.h"
#include"EBO.h"

const unsigned int width = 800;
const unsigned int height = 800;

// Coordenadas de los vértices de la pirámide
GLfloat vertices[] =
{ //   COORDENADAS (x, y, z)   /   COLORES (r, g, b)   /   TexCoord (u, v)  //
	-0.5f, 0.0f,  0.5f,     0.0f, 0.2f, 0.8f,	0.0f, 0.0f,
	-0.5f, 0.0f, -0.5f,     0.0f, 0.2f, 0.8f,	5.0f, 0.0f,
	 0.5f, 0.0f, -0.5f,     0.0f, 0.2f, 0.8f,	0.0f, 0.0f,
	 0.5f, 0.0f,  0.5f,     0.0f, 0.2f, 0.8f,	5.0f, 0.0f,
	 0.0f, 0.8f,  0.0f,     0.4f, 0.7f, 1.0f,	2.5f, 5.0f
};

// Orden en que se conectan los vértices para formar los triángulos (base y 4 caras)
GLuint indices[] =
{
	0, 1, 2,
	0, 2, 3,
	0, 1, 4,
	1, 2, 4,
	2, 3, 4,
	3, 0, 4
};

int main()
{
	// Inicia la librería GLFW
	glfwInit();

	// Indica que usamos OpenGL 3.3 con el perfil CORE (solo funciones modernas)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Crea la ventana de 800 x 800 píxeles
	GLFWwindow* window = glfwCreateWindow(width, height, "YoutubeOpenGL", NULL, NULL);
	// Si la ventana no se pudo crear, muestra un error y cierra el programa
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	// Hace que la ventana sea el contexto de OpenGL actual
	glfwMakeContextCurrent(window);

	// Carga GLAD para poder usar las funciones de OpenGL
	gladLoadGL();

	// Define el área de dibujo: de x = 0, y = 0 hasta x = 800, y = 800
	glViewport(0, 0, width, height);

	// Crea el programa de shaders a partir de default.vert y default.frag
	Shader shaderProgram("default.vert", "default.frag");

	// Crea y activa el VAO, y crea el VBO (vértices) y el EBO (índices)
	VAO VAO1;
	VAO1.Bind();
	VBO VBO1(vertices, sizeof(vertices));
	EBO EBO1(indices, sizeof(indices));

	// Enlaza los atributos al VAO: coordenadas, colores y coordenadas de textura
	VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 8 * sizeof(float), (void*)0);
	VAO1.LinkAttrib(VBO1, 1, 3, GL_FLOAT, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	VAO1.LinkAttrib(VBO1, 2, 2, GL_FLOAT, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	// Desactiva todo para no modificarlos por accidente
	VAO1.Unbind();
	VBO1.Unbind();
	EBO1.Unbind();

	// Obtiene el ID del uniform "scale" del shader
	GLuint uniID = glGetUniformLocation(shaderProgram.ID, "scale");

	// Carga la textura (chai.jpeg debe estar en la carpeta del proyecto)
	Texture brickTex("chai.jpeg", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
	brickTex.texUnit(shaderProgram, "tex0", 0);

	// Variables para la rotación de la pirámide
	float rotation = 0.0f;
	double prevTime = glfwGetTime();

	// Activa el buffer de profundidad para que las caras se dibujen en el orden correcto
	glEnable(GL_DEPTH_TEST);

	// Ciclo principal: se repite hasta cerrar la ventana
	while (!glfwWindowShouldClose(window))
	{
		// Color de fondo (rojo oscuro)
		glClearColor(0.35f, 0.0f, 0.02f, 1.0f);
		// Limpia el buffer de color y el de profundidad
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Indica qué programa de shaders se va a usar
		shaderProgram.Activate();

		// Temporizador simple: aumenta la rotación unas 60 veces por segundo
		double crntTime = glfwGetTime();
		if (crntTime - prevTime >= 1.0 / 60)
		{
			rotation += 0.5f;
			prevTime = crntTime;
		}

		// Crea las matrices como matriz identidad (para que no sean nulas)
		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 proj = glm::mat4(1.0f);

		// Asigna una transformación a cada matriz: giro, posición de la cámara y perspectiva
		model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
		view = glm::translate(view, glm::vec3(0.0f, -0.5f, -2.0f));
		proj = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);

		// Envía las matrices al Vertex Shader
		int modelLoc = glGetUniformLocation(shaderProgram.ID, "model");
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		int viewLoc = glGetUniformLocation(shaderProgram.ID, "view");
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		int projLoc = glGetUniformLocation(shaderProgram.ID, "proj");
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

		// Asigna el valor del uniform (siempre después de activar el shader)
		glUniform1f(uniID, 0.5f);
		// Activa la textura para que se use al dibujar
		brickTex.Bind();
		// Activa el VAO para que OpenGL sepa qué datos dibujar
		VAO1.Bind();
		// Dibuja los triángulos usando los índices
		glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(int), GL_UNSIGNED_INT, 0);
		// Intercambia el buffer de atrás con el de adelante (muestra el cuadro dibujado)
		glfwSwapBuffers(window);
		// Procesa los eventos de la ventana (teclado, mouse, cerrar)
		glfwPollEvents();
	}

	// Elimina todos los objetos que creamos
	VAO1.Delete();
	VBO1.Delete();
	EBO1.Delete();
	brickTex.Delete();
	shaderProgram.Delete();

	// Destruye la ventana antes de terminar el programa
	glfwDestroyWindow(window);
	// Cierra GLFW antes de terminar
	glfwTerminate();
	return 0;
};