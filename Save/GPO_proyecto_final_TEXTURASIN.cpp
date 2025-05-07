/************************  GPO_01 ************************************
ATG, 2019
******************************************************************************/

#include <GpO.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

// Tamaño ventana y titulo
int ANCHO = 1600, ALTO = 1200;  
const char* prac = "Visualizador Interactivo con OpenGL (GpO)";   // Nombre de la practica (aparecera en el titulo de la ventana).


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     CODIGO SHADERS 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GLSL(src) "#version 330 core\n" #src

const char* vertex_prog = GLSL(
layout(location = 0) in vec3 pos; 
layout(location = 1) in vec3 color;
out vec3 col;
uniform mat4 MVP=mat4(1.0f);
void main()
 {
  gl_Position = MVP*vec4(pos,1); // Construyo coord homog�neas y aplico matriz transformacion M
  col = color;                             // Paso color a fragment shader
 }
);

const char* fragment_prog = GLSL(
in vec3 col;
out vec3 outputColor;
void main() 
 {
	outputColor = col;
 }
);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////   RENDER CODE AND DATA
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

GLFWwindow* window;
GLuint prog;
objeto obj_trunk, obj_leafs;

objeto cargar_objeto(const char* ruta) {
    Assimp::Importer importador;
    const aiScene* escena = importador.ReadFile(ruta,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals);

    if (!escena || escena->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !escena->mRootNode) {
        printf("ERROR::ASSIMP:: %s\n", importador.GetErrorString());
        exit(-1);
    }

    // Contar vértices y caras totales
    int Nv_total = 0;
    int Nf_total = 0;
    for (unsigned int m = 0; m < escena->mNumMeshes; ++m) {
        Nv_total += escena->mMeshes[m]->mNumVertices;
        Nf_total += escena->mMeshes[m]->mNumFaces;
    }

    float* vertices = (float*)malloc(sizeof(float) * 3 * Nv_total);
    float* colores  = (float*)malloc(sizeof(float) * 3 * Nv_total);
    unsigned int* indices = (unsigned int*)malloc(sizeof(unsigned int) * 3 * Nf_total);

    int v_offset = 0;
    int i_offset = 0;

    for (unsigned int m = 0; m < escena->mNumMeshes; ++m) {
        aiMesh* malla = escena->mMeshes[m];
        int Nv = malla->mNumVertices;
        int Nf = malla->mNumFaces;

        // Obtener color del material (o gris por defecto)
        aiColor4D color;
        aiColor4D color_difuso = {0.5f, 0.5f, 0.5f, 1.0f};
        if (aiGetMaterialColor(escena->mMaterials[malla->mMaterialIndex], AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS)
            color_difuso = color;

        // Cargar vértices y color
        for (int i = 0; i < Nv; ++i) {
            aiVector3D pos = malla->mVertices[i];
            int idx = v_offset + i;
            vertices[idx * 3 + 0] = pos.x;
            vertices[idx * 3 + 1] = pos.y;
            vertices[idx * 3 + 2] = pos.z;

            colores[idx * 3 + 0] = color_difuso.r;
            colores[idx * 3 + 1] = color_difuso.g;
            colores[idx * 3 + 2] = color_difuso.b;
        }

        // Cargar caras con offset
        for (int i = 0; i < Nf; ++i) {
            aiFace face = malla->mFaces[i];
            if (face.mNumIndices == 3) {
                indices[i_offset * 3 + 0] = face.mIndices[0] + v_offset;
                indices[i_offset * 3 + 1] = face.mIndices[1] + v_offset;
                indices[i_offset * 3 + 2] = face.mIndices[2] + v_offset;
                ++i_offset;
            }
        }

        v_offset += Nv;
    }

    // Crear buffers OpenGL
    GLuint VAO, VBO_pos, VBO_col, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO_pos);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_pos);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * Nv_total, vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &VBO_col);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_col);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * Nv_total, colores, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 3 * i_offset, indices, GL_STATIC_DRAW);

    glBindVertexArray(0); // Desvincular

    free(vertices);
    free(colores);
    free(indices);

    objeto obj;
    obj.VAO = VAO;
    obj.Ni = 3 * i_offset; // número real de índices
    return obj;
}

// Preparaci�n de los datos de los objetos a dibujar, envialarlos a la GPU
// Compilaci�n programas a ejecutar en la tarjeta gr�fica:  vertex shader, fragment shaders
// Opciones generales de render de OpenGL

void init_scene()
{
	int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height); 
    
	obj_trunk =  cargar_objeto("./data/Trunk_v2.obj");  // Preparar datos de objeto, mandar a GPU
    obj_leafs = cargar_objeto("./data/Leafs.obj");

	
	prog = Compile_Link_Shaders(vertex_prog, fragment_prog); 

	glUseProgram(prog);    // Indicamos que programa vamos a usar 
    glEnable(GL_CULL_FACE);
    //glCullFace(GL_FRONT);
    //glCullFace(GL_BACK);
    //glDisable(GL_CULL_FACE);  
    
}

float rot_arbol = 0.0f; 
vec3 desplazamiento(-7.0f, -3.5f, 0.0f);  // Desplazamiento en X, Y, Z

vec3 pos_obs=vec3(10.0f,0.0f,0.0f); //###vec3 pos_obs=vec3(1.5f,0.0f,0.0f); 
vec3 target = vec3(0.0f,0.0f,0.0f);
vec3 up = vec3(0,0,1);

float fov = 35.0f, aspect = 4.0f / 3.0f; //###float fov = 40.0f, aspect = 4.0f / 3.0f;



void render_scene()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Especifica color para el fondo (RGB+alfa)
    glClear(GL_COLOR_BUFFER_BIT);           // Aplica color asignado borrando el buffer

    float t = (float)glfwGetTime();  // Contador de tiempo en segundos 

    ///////// Actualizacion matrices M, V, P /////////    
    mat4 P, V, M, T, R, R_y, S;

    P = perspective(glm::radians(fov), aspect, 0.5f, 20.0f);  // 40° FOV, 4:3, Znear=0.5, Zfar=20
    V = lookAt(pos_obs, target, up);  // Pos camara, Lookat, head up

    // Aplica rotación de 90 grados en sentido contrario a las agujas de reloj (π/2 radianes) alrededor del eje Y
    R = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	// Rotación del árbol sobre su eje vertical (Z en este sistema)
	R_y = glm::rotate(mat4(1.0f), glm::radians(rot_arbol), vec3(0.0f, 1.0f, 0.0f));

    // Aplica desplazamiento en los tres ejes
	T = glm::translate(glm::vec3(desplazamiento.x, desplazamiento.y, desplazamiento.z)); 

    // Aplica la rotación antes de la traslación
    //M = R * T;
	// Model matrix: primero rotación vertical (R_y), luego giro de base (R), luego traslación
	M = R * T * R_y;
	

    transfer_mat4("MVP", P * V * M);  // Calcula MVP (Model-View-Projection)


    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Activa el modo Wireframe

    // ORDEN de dibujar
    glBindVertexArray(obj_trunk.VAO);
    glDrawElements(GL_TRIANGLES, obj_trunk.Ni, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glBindVertexArray(obj_leafs.VAO);
    glDrawElements(GL_TRIANGLES, obj_leafs.Ni, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    ////////////////////////////////////////////////////////
}




//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PROGRAMA PRINCIPAL
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	init_GLFW();            // Inicializa lib GLFW
	window = Init_Window(prac);  // Crea ventana usando GLFW, asociada a un contexto OpenGL	X.Y
	load_Opengl();         // Carga funciones de OpenGL, comprueba versi�n.
	init_scene();          // Prepara escena
	
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(window))
	{
		render_scene();
		glfwSwapBuffers(window);
		glfwPollEvents();
		show_info();
	}

	glfwTerminate();
	exit(EXIT_SUCCESS);
}


//////////  FUNCION PARA MOSTRAR INFO OPCIONAL EN EL TITULO DE VENTANA  //////////
void show_info()
{
	static int fps = 0;
	static double last_tt = 0;
	double elapsed, tt;
	char nombre_ventana[128];   // buffer para modificar titulo de la ventana

	fps++; tt = glfwGetTime();  // Contador de tiempo en segundos 

	elapsed = (tt - last_tt);
	if (elapsed >= 0.5)  // Refrescar cada 0.5 segundo
	{
		sprintf_s(nombre_ventana, 128, "%s: %4.0f FPS @ %d x %d", prac, fps / elapsed, ANCHO, ALTO);
		glfwSetWindowTitle(window, nombre_ventana);
		last_tt = tt; fps = 0;
	}

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////  ASIGNACON FUNCIONES CALLBACK
///////////////////////////////////////////////////////////////////////////////////////////////////////////


// Callback de cambio tama�o de ventana
void ResizeCallback(GLFWwindow* window, int width, int height)
{
	glfwGetFramebufferSize(window, &width, &height); 
	glViewport(0, 0, width, height);
	ALTO = height;	ANCHO = width;
	
}

// Callback de pulsacion de tecla
static void KeyCallback(GLFWwindow* window, int key, int code, int action, int mode)
{
	fprintf(stdout, "Key %d Code %d Act %d Mode %d\n", key, code, action, mode);
	if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {  // Solo hacer algo si la tecla es presionada
		/*
        // Movimiento con las flechas de dirección
        if (key == GLFW_KEY_UP) {
            desplazamiento.x -= 0.1f;  // Mover hacia arriba en el eje Y
        }
        if (key == GLFW_KEY_DOWN) {
            desplazamiento.x += 0.1f;  // Mover hacia abajo en el eje Y
        }
		*/
		
        if (key == GLFW_KEY_LEFT) {
            rot_arbol += 5.0f;
        }
        if (key == GLFW_KEY_RIGHT) {
            rot_arbol -= 5.0f;  // Gira 5 grados a la derecha
        }
		

        // Imprimir el nuevo desplazamiento para depuración
        printf("Desplazamiento: x = %f, y = %f, z = %f\n", desplazamiento.x, desplazamiento.y, desplazamiento.z);
    }
}


void scroll(GLFWwindow* window, double dx, double dy){
    // Ajusta desplazamiento en el eje Z (profundidad)
    desplazamiento.x += (dy > 0) ? 0.1f : (dy < 0) ? -0.1f : 0.0f;

    // Limitar desplazamiento.z entre un rango específico si es necesario
    desplazamiento.x = glm::clamp(desplazamiento.x, -10.0f, 10.0f);  // Ajusta el rango según lo necesites

}



void asigna_funciones_callback(GLFWwindow* window)
{
	glfwSetWindowSizeCallback(window, ResizeCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window,scroll);
}



