/************************  GPO_01 ************************************
ATG, 2019
******************************************************************************/

#include <GpO.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

#include <stb_image.h>

// Tamaño ventana y titulo
int ANCHO = 1600, ALTO = 1200;  
const char* prac = "Visualizador Interactivo con OpenGL (GpO)";   // Nombre de la practica (aparecera en el titulo de la ventana).


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     CODIGO SHADERS 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GLSL(src) "#version 330 core\n" #src

// Todas la lineas comentadas lo estan en caso de querer cargar texturas

const char* vertex_prog = GLSL(
    layout(location = 0) in vec3 pos;           // Coordenadas de posición
    layout(location = 1) in vec3 color;         // Color (no lo usamos en este caso)
    //layout(location = 2) in vec2 texCoord;      // Coordenadas de textura
    //out vec2 TexCoords;                         // Pasamos las coordenadas de textura al fragment shader
    uniform mat4 MVP = mat4(1.0f);              // Matriz MVP (Model-View-Projection)
    
    void main()
    {
        gl_Position = MVP * vec4(pos, 1.0);     // Aplicar la matriz de transformación
        //TexCoords = texCoord;                   // Pasar las coordenadas de textura
    }
);

    
const char* fragment_prog = GLSL(
    in vec3 fragPos;
    in vec3 normal;
    //in vec2 TexCoords;

    out vec4 FragColor;

    //uniform sampler2D texture_diffuse;

    void main()
    {
        //vec4 textureColor = texture(texture_diffuse, TexCoords);
        //FragColor = textureColor;
        //FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Rojo
        FragColor = vec4(0.5f, 0.5f, 0.5f, 1.0f); //Gris
    }
);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////   LOAD DATA
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


GLFWwindow* window;
GLuint prog;
objeto obj_trunk, obj_leafs;
//GLuint textura_tronco, textura_hojas;

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

void procesar_mallas(const aiScene* escena, float* vertices, float* colores, /*float* texCoords,*/ unsigned int* indices, int& Ni_total) {
    aiMesh* malla = escena->mMeshes[0];  // Accedemos directamente a la primera malla
    int Nv = malla->mNumVertices;
    int Nf = malla->mNumFaces;

    // Color difuso por defecto
    aiColor4D color = {0.5f, 0.5f, 0.5f, 1.0f}; // Gris
    aiGetMaterialColor(escena->mMaterials[malla->mMaterialIndex], AI_MATKEY_COLOR_DIFFUSE, &color);

    int v_offset = 0;
    int i_offset = 0;

    // Cargar vértices, colores y coordenadas de textura
    for (int i = 0; i < Nv; ++i) {
        aiVector3D pos = malla->mVertices[i];
        int idx = v_offset + i;
        vertices[idx * 3 + 0] = pos.x;
        vertices[idx * 3 + 1] = pos.y;
        vertices[idx * 3 + 2] = pos.z;

        colores[idx * 3 + 0] = color.r;
        colores[idx * 3 + 1] = color.g;
        colores[idx * 3 + 2] = color.b;

        /*
        if (malla->HasTextureCoords(0)) {
            aiVector3D tex = malla->mTextureCoords[0][i];
            tex.x = glm::clamp(tex.x, 0.0f, 1.0f);  
            tex.y = glm::clamp(tex.y, 0.0f, 1.0f);

            // Asignar las coordenadas corregidas
            texCoords[idx * 2 + 0] = tex.x;
            texCoords[idx * 2 + 1] = tex.y;
        

            // Verificar si las coordenadas de textura están dentro del rango [0.0, 1.0]
            if (tex.x < 0.0f || tex.x > 1.0f || tex.y < 0.0f || tex.y > 1.0f) {
                std::cerr << "Advertencia: Coordenadas de textura fuera de rango en el vértice " << i << ": (" 
                          << tex.x << ", " << tex.y << ")" << std::endl;
            }
        } else {
            
            texCoords[idx * 2 + 0] = 0.0f;
            texCoords[idx * 2 + 1] = 0.0f;

            std::cout << "Advertencia: No se encontraron coordenadas de textura para el vértice " << i << std::endl;

            
        }
        */
    }

    // Cargar caras
    for (int i = 0; i < Nf; ++i) {
        aiFace face = malla->mFaces[i];
        if (face.mNumIndices == 3) {
            unsigned int idx0 = face.mIndices[0] + v_offset;
            unsigned int idx1 = face.mIndices[1] + v_offset;
            unsigned int idx2 = face.mIndices[2] + v_offset;

            // Verificar si los índices están dentro del rango
            if (idx0 >= Nv || idx1 >= Nv || idx2 >= Nv) {
                std::cerr << "Advertencia: Índice fuera de rango en la cara " << i << ": (" 
                          << idx0 << ", " << idx1 << ", " << idx2 << ")" << std::endl;
            }

            indices[i_offset * 3 + 0] = idx0;
            indices[i_offset * 3 + 1] = idx1;
            indices[i_offset * 3 + 2] = idx2;
            ++i_offset;
        } else {
            std::cerr << "Advertencia: Cara no triangular en la cara " << i << std::endl;
        }
    }

    Ni_total = i_offset * 3;  // Número total de índices
}


objeto crear_buffers_opengl(float* vertices, float* colores, /*float* texCoords,*/ unsigned int* indices, int Nv_total, int Ni_total) {
    GLuint VAO, VBO_pos, VBO_col, /*VBO_tex,*/ EBO;
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
    /*
    glGenBuffers(1, &VBO_tex);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_tex);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * Nv_total, texCoords, GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    */
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

// --- Función principal ---
objeto cargar_objeto(const char* ruta) {
    Assimp::Importer importador;
    const aiScene* escena = cargar_escena(ruta, importador);

    int Nv_total, Nf_total;
    contar_vertices_caras(escena->mMeshes[0], Nv_total, Nf_total);

    float* vertices = (float*)malloc(sizeof(float) * 3 * Nv_total);
    float* colores = (float*)malloc(sizeof(float) * 3 * Nv_total);
    //float* texCoords = (float*)malloc(sizeof(float) * 2 * Nv_total);
    unsigned int* indices = (unsigned int*)malloc(sizeof(unsigned int) * 3 * Nf_total);

    int Ni_total;
    procesar_mallas(escena, vertices, colores, /*texCoords,*/ indices, Ni_total);

    objeto obj = crear_buffers_opengl(vertices, colores, /*texCoords,*/ indices, Nv_total, Ni_total);

    free(vertices);
    free(colores);
    //free(texCoords);
    free(indices);

    return obj;
}

/*

GLuint cargar_textura(const char* ruta) {
    int width, height, nrChannels;
    unsigned char *data = stbi_load(ruta, &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << ruta << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Configuración de parámetros de la textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Cargar la imagen en OpenGL
    GLenum format = (nrChannels == 1) ? GL_RED : (nrChannels == 3) ? GL_RGB : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    return textureID;
}
*/
// Preparaci�n de los datos de los objetos a dibujar, envialarlos a la GPU
// Compilaci�n programas a ejecutar en la tarjeta gr�fica:  vertex shader, fragment shaders
// Opciones generales de render de OpenGL

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////   RENDER CODE 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


void init_scene()
{
	int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height); 
    
	obj_trunk =  cargar_objeto("./data/Trunk_v3.obj");  // Preparar datos de objeto, mandar a GPU
    obj_leafs = cargar_objeto("./data/Leafs_v2.obj");

	//textura_tronco = cargar_textura("./data/Tronco/Tronco.png");
    //textura_hojas = cargar_textura("./data/Hoja/Hoja.png");

	prog = Compile_Link_Shaders(vertex_prog, fragment_prog); 

    //glUniform1i(glGetUniformLocation(prog, "texture_diffuse"), 0);

	glUseProgram(prog);    // Indicamos que programa vamos a usar 
    glEnable(GL_CULL_FACE);
    //glCullFace(GL_FRONT);
    glCullFace(GL_BACK);
    //glDisable(GL_CULL_FACE);  
    //glEnable(GL_DEPTH_TEST);
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

    // Aplica la rotación antes de la traslación
    //M = R * T;
	// Model matrix: primero rotación vertical (R_y), luego giro de base (R), luego traslación
	M = R * T * R_y;
	

    transfer_mat4("MVP", P * V * M);  // Calcula MVP (Model-View-Projection)


    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Activa el modo Wireframe

    // Activamos el programa de shaders
    glUseProgram(prog);

    // ORDEN de dibujar
    // Dibujar el tronco
    
    //glActiveTexture(GL_TEXTURE0);  // Activamos la unidad de textura 0
    //glBindTexture(GL_TEXTURE_2D, textura_tronco);  // Asignamos la textura del tronco a la unidad 0
    //glUniform1i(glGetUniformLocation(prog, "texture_diffuse"), 0);  // Enviamos la unidad de textura (0) al shader
    glBindVertexArray(obj_trunk.VAO);
    glDrawElements(GL_TRIANGLES, obj_trunk.Ni, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Dibujar las hojas
    
    //glActiveTexture(GL_TEXTURE0);  // Activamos la unidad de textura 0
    //glBindTexture(GL_TEXTURE_2D, textura_hojas);  // Asignamos la textura de las hojas a la unidad 0
    //glUniform1i(glGetUniformLocation(prog, "texture_diffuse"), 0);  // Enviamos la unidad de textura (0) al shader
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
            }
            desplazamiento.x += 0.1f;  // Mover hacia abajo en el eje Y
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



