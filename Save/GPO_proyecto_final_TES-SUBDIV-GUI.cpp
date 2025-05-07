/************************  GPO_01 ************************************
ATG, 2019
******************************************************************************/

#include <GpO.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

#include <stb_image.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// Tamaño ventana y titulo
int ANCHO = 1600, ALTO = 1200;  
const char* prac = "Visualizador Interactivo con OpenGL (GpO)";   // Nombre de la practica (aparecera en el titulo de la ventana).


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     CODIGO SHADERS 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GLSL(src) "#version 450 core\n" #src


const char* vertex_shader = GLSL(
    layout(location = 0) in vec3 pos;
    layout(location = 1) in vec2 uv;

    uniform mat4 MVP; 

    out vec2 vUV;

    void main() {
        gl_Position = MVP * vec4(pos, 1.0);
        vUV = uv;
    }
);

const char* tcs_shader = GLSL(
    layout(vertices = 3) out;

    uniform float tessellationFactor;

    void main() {

        gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;


        if (gl_InvocationID == 0) {
            gl_TessLevelOuter[0] = tessellationFactor;
            gl_TessLevelOuter[1] = tessellationFactor;
            gl_TessLevelOuter[2] = tessellationFactor;
            gl_TessLevelInner[0] = tessellationFactor;
        }
    }
);

const char* tes_shader = GLSL(
    layout(triangles, equal_spacing, cw) in;

    void main() {
        // gl_TessCoord = (u, v, w) baricéntricas dentro del triángulo
        vec3 bary = gl_TessCoord;
    
        // Interpolación lineal de posición en base a coordenadas baricéntricas
        vec4 p0 = gl_in[0].gl_Position;
        vec4 p1 = gl_in[1].gl_Position;
        vec4 p2 = gl_in[2].gl_Position;
    
        gl_Position = bary.x * p0 + bary.y * p1 + bary.z * p2;
    }
);

const char* fragment_shader = GLSL(
    in vec2 uv;  // Coordenadas UV (de ser necesario)
    out vec4 FragColor;  // El color del fragmento

    void main() {
        // Simplemente asignamos un color fijo para el wireframe (bordes)
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);  // Rojo para las aristas
    }
);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////   GUI
//////////////////////////////////////////////////////////////////////////////////////////////////////////////



void init_gui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void render_gui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Tessellation Control");
    ImGui::SliderFloat("SubDivision Factor", &tessellationFactor, 1.0f, 128.0f);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void shutdown_gui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////   LOAD DATA
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


GLFWwindow* window;
GLuint prog;
objeto obj_trunk, obj_leafs;
float tessellationFactor = 1.0f;  // valor inicial

const aiScene* cargar_escena(const char* ruta, Assimp::Importer& importador) {
    const aiScene* escena = importador.ReadFile(ruta,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals);

    if (!escena || escena->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !escena->mRootNode) {
        printf("ERROR::ASSIMP:: %s\n", importador.GetErrorString());
        exit(-1);
    }
    return escena;
}

void contar_vertices_caras(aiMesh* malla, int& Nv_total, int& Nf_total) {
    Nv_total = malla->mNumVertices;
    Nf_total = malla->mNumFaces;
}

void procesar_mallas(const aiScene* escena, float* vertices, float* colores, unsigned int* indices, int& Ni_total) {
    aiMesh* malla = escena->mMeshes[0];
    int Nv = malla->mNumVertices;
    int Nf = malla->mNumFaces;

    aiColor4D color = {0.5f, 0.5f, 0.5f, 1.0f};
    aiGetMaterialColor(escena->mMaterials[malla->mMaterialIndex], AI_MATKEY_COLOR_DIFFUSE, &color);

    int v_offset = 0;
    int i_offset = 0;

    for (int i = 0; i < Nv; ++i) {
        aiVector3D pos = malla->mVertices[i];
        int idx = v_offset + i;
        vertices[idx * 3 + 0] = pos.x;
        vertices[idx * 3 + 1] = pos.y;
        vertices[idx * 3 + 2] = pos.z;

        colores[idx * 3 + 0] = color.r;
        colores[idx * 3 + 1] = color.g;
        colores[idx * 3 + 2] = color.b;
    }

    for (int i = 0; i < Nf; ++i) {
        aiFace face = malla->mFaces[i];
        if (face.mNumIndices == 3) {
            unsigned int idx0 = face.mIndices[0] + v_offset;
            unsigned int idx1 = face.mIndices[1] + v_offset;
            unsigned int idx2 = face.mIndices[2] + v_offset;

            indices[i_offset * 3 + 0] = idx0;
            indices[i_offset * 3 + 1] = idx1;
            indices[i_offset * 3 + 2] = idx2;
            ++i_offset;
        }
    }

    Ni_total = i_offset * 3;
}


objeto crear_buffers_opengl(float* vertices, float* colores, unsigned int* indices, int Nv_total, int Ni_total) {
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * Ni_total, indices, GL_STATIC_DRAW);

    glBindVertexArray(0);

    objeto obj;
    obj.VAO = VAO;
    obj.Ni = Ni_total;
    obj.Nv = Nv_total;
    obj.tipo_indice = GL_UNSIGNED_INT;

    return obj;
}

objeto cargar_objeto(const char* ruta) {
    Assimp::Importer importador;
    const aiScene* escena = cargar_escena(ruta, importador);

    int Nv_total, Nf_total;
    contar_vertices_caras(escena->mMeshes[0], Nv_total, Nf_total);

    float* vertices = (float*)malloc(sizeof(float) * 3 * Nv_total);
    float* colores = (float*)malloc(sizeof(float) * 3 * Nv_total);
    unsigned int* indices = (unsigned int*)malloc(sizeof(unsigned int) * 3 * Nf_total);

    int Ni_total;
    procesar_mallas(escena, vertices, colores, indices, Ni_total);

    objeto obj = crear_buffers_opengl(vertices, colores, indices, Nv_total, Ni_total);

    free(vertices);
    free(colores);
    free(indices);

    return obj;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////   RENDER CODE 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


void init_scene()
{
	int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height); 
    
	obj_trunk =  cargar_objeto("./data/Trunk_v3.obj");  
    obj_leafs = cargar_objeto("./data/Leafs_v2.obj");

    
	prog = Compile_Link_Shaders(vertex_shader, fragment_shader, tcs_shader, tes_shader);
    //prog = Compile_Link_Shaders(vertex_shader, fragment_shader); 

    glPatchParameteri(GL_PATCH_VERTICES, 3);

    // Bind y uso de shaders
    glUseProgram(prog);

    // Uniforms
    //glUniform1f(glGetUniformLocation(prog, "tessellationFactor"), 5.0f);
    glUniform1f(glGetUniformLocation(prog, "tessellationFactor"), tessellationFactor);
    

	//glUseProgram(prog);   
    glEnable(GL_CULL_FACE);
    //glCullFace(GL_FRONT);
    glCullFace(GL_BACK);
    //glDisable(GL_CULL_FACE);  
    //glEnable(GL_DEPTH_TEST);

    //GUI
    init_gui(window);
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);           // Aplica color asignado borrando el buffer

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

	// Model matrix: primero rotación vertical (R_y), luego giro de base (R), luego traslación
	M = R * T * R_y;
	

    transfer_mat4("MVP", P * V * M);  // Calcula MVP (Model-View-Projection)


    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Activa el modo Wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Activamos el programa de shaders
    glUseProgram(prog);

    // ORDEN de dibujar el tronco
    glBindVertexArray(obj_trunk.VAO);
    glDrawElements(GL_PATCHES, obj_trunk.Ni, obj_trunk.tipo_indice, 0);
    //glDrawElements(GL_TRIANGLES, obj_trunk.Ni, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Dibujar las hojas
    glBindVertexArray(obj_leafs.VAO);
    glDrawElements(GL_PATCHES, obj_leafs.Ni, obj_trunk.tipo_indice, 0);
    //glDrawElements(GL_TRIANGLES, obj_leafs.Ni, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // En el bucle principal:
    render_gui();

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
    // Al cerrar la app:
    shutdown_gui();

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
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            if (key == GLFW_KEY_UP) {
                tessellationFactor += 1.0f;
            } else if (key == GLFW_KEY_DOWN) {
                tessellationFactor = std::max(1.0f, tessellationFactor - 1.0f);  // mínimo 1
            }
    
            // Actualiza el uniform en tiempo real
            glUseProgram(prog);
            glUniform1f(glGetUniformLocation(prog, "tessellationFactor"), tessellationFactor);
    
            std::cout << "Tessellation factor: " << tessellationFactor << std::endl;
        }
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



