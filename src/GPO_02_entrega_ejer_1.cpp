/************************  GPO_02 ************************************
ATG, 2022
******************************************************************************/


#include <GpO.h>

// TAMA�O y TITULO INICIAL de la VENTANA
int ANCHO = 800, ALTO = 600;  // Tama�o inicial ventana
const char *prac = "OpenGL(GpO)";   // Nombre de la practica (aparecera en el titulo de la ventana).

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     CODIGO SHADERS 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GLSL(src) "#version 330 core\n" #src

const char *vertex_prog = GLSL(
                                  layout(location = 0) in vec3 pos;
                                  layout(location = 1) in vec3 color;
                                  out vec3 col;
                                  uniform mat4 MVP = mat4(1.0f);
                                  void main() {
                                      gl_Position = MVP * vec4(pos,
                                                               1); // Construyo coord homog�neas y aplico matriz transformacion M
                                      col = color;                     // Paso color a fragment shader
                                  }
                          );


const char *fragment_prog = GLSL(
                                    in vec3 col;
                                    void main() {
                                        gl_FragColor = vec4(col, 1);
                                    }
                            );


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////   RENDER CODE AND DATA
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

GLFWwindow *window;
GLuint prog;
objeto obj1, obj2;

objeto crear_cubo(void) {
    objeto obj;
    GLuint VAO;
    GLuint buffer, i_buffer;

    GLfloat vertex_data[] =
            {       // posicion vertice  // color vertice
                    -1.0f, -1.0f, -1.0f, 0.00f, 0.00f, 0.56f,
                    -1.0f, 1.0f, -1.0f, 0.00f, 0.06f, 1.00f,
                    1.0f, 1.0f, -1.0f, 0.00f, 0.56f, 1.00f,
                    1.0f, -1.0f, -1.0f, 0.06f, 1.00f, 0.94f,
                    -1.0f, -1.0f, 1.0f, 0.56f, 1.00f, 0.44f,
                    -1.0f, 1.0f, 1.0f, 1.00f, 0.94f, 0.00f,
                    1.0f, 1.0f, 1.0f, 1.00f, 0.44f, 0.00f,
                    1.0f, -1.0f, 1.0f, 0.94f, 0.00f, 0.00f
            };

    GLbyte indices[] =
            {
                    0, 3, 7,
                    0, 7, 4,
                    0, 4, 1,
                    4, 5, 1,
                    0, 1, 2,
                    0, 2, 3,
                    4, 7, 6,
                    4, 6, 5,
                    1, 5, 6,
                    1, 6, 2,
                    2, 6, 7,
                    3, 2, 7,};

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);

    // Especifico como encontrar 1er argumento (atributo 0) del vertex shader
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
    // Defino 2� argumento (atributo 1) del vertex shader
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) (3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Asignados atributos, podemos desconectar BUFFER

    glGenBuffers(1, &i_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, i_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);  // Mandamos buffer con �ndices.

    glBindVertexArray(0);  //Cerramos Vertex Array con todo listo para ser pintado

    obj.VAO = VAO;
    obj.Nv = 8;
    obj.Ni = 12 * 3;
    obj.tipo_indice = GL_UNSIGNED_BYTE;

    return obj;
}

void dibujar_indexado(objeto obj) {
    // Activamos VAO asociado a obj1 (cubo) y lo dibujamos con glDrawElements
    glBindVertexArray(obj.VAO);
    glDrawElements(GL_TRIANGLES, obj.Ni, obj.tipo_indice, (void *) 0);  // Dibujar (usando indices)
    glBindVertexArray(0);  //Desactivamos VAO activo.
}

// Preparaci�n de los datos de los objetos a dibujar, enviarlos a la GPU
// Compilaci�n programas a ejecutar en la tarjeta gr�fica:  vertex shader, fragment shaders
// Opciones generales de render de OpenGL
void init_scene() {
    obj1 = crear_cubo();  // Datos del objeto, mandar a GPU
    obj2 = crear_cubo();
    prog = Compile_Link_Shaders(vertex_prog, fragment_prog); // Mandar programas a GPU, compilar y crear programa en GPU
    glUseProgram(prog);    // Indicamos que programa vamos a usar
    glEnable(GL_CULL_FACE);

    glEnable(GL_DEPTH_TEST);  // Habilita el test de profundidad (Z-Buffer)
    glDepthFunc(GL_LESS);     // Un fragmento se dibuja si su Z es MENOR que el almacenado
}


vec3 pos_obs = vec3(0.0f, 6.0f, 4.0f);
vec3 target = vec3(0.0f, 0.0f, 0.0f);
vec3 up = vec3(0, 0, 1);

float zfar = 25.0f;
float znear = 1.0f;

mat4 Proy, View, M;

//bool invertirObjs = true;
bool mostrarPrimeroObj1 = true;
// Actualizar escena: cambiar posici�n objetos, nuevos objetros, posici�n c�mara, luces, etc.
void render_scene() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Especifica color para el fondo (RGB+alfa)
    //glClear(GL_COLOR_BUFFER_BIT);          // Aplica color asignado borrando el buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    float t = (float) glfwGetTime();  // Contador de tiempo en segundos

    ///////// C�digo para actualizar escena  /////////
    Proy = perspective(40.0f, 4.0f / 3.0f, znear, zfar);  //40� Y-FOV,  4:3 ,  ZNEAR, ZFAR
    View = lookAt(pos_obs, target, up);  // Pos camara, Lookat, head up

    mat4 T, R, S;

    M = rotate(1.0f*t, vec3(0.0f, 0.0f, 1.0f));

    transfer_mat4("MVP", Proy * View * M);
    dibujar_indexado(obj1);

    ////////////////////////////////////////////////////////
    //Segundo cubo
    float angle = t * 50.0f; // 50 grados por segundo
    float radius = 2.5f;

    //Escalado
    S = scale(mat4(1.0f), vec3(0.6f, 0.6f, 0.6f));

    //Rotacion
    R = rotate(radians(angle), vec3(1.0f, 0.0f, 0.0f));

    //TRalacion
    T = translate(mat4(1.0f), vec3(radius * cos(t), radius * sin(t), 0.0f));

    M = T * R * S;
    transfer_mat4("MVP", Proy * View * M);
    dibujar_indexado(obj2);


}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PROGRAMA PRINCIPAL

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    init_GLFW();            // Inicializa lib GLFW
    window = Init_Window(prac);  // Crea ventana usando GLFW, asociada a un contexto OpenGL
    load_Opengl();         // Carga funciones de OpenGL, comprueba versi�n.
    init_scene();          // Prepara escena

    while (!glfwWindowShouldClose(window)) {
        render_scene();
        glfwSwapBuffers(window);
        glfwPollEvents();
        show_info();
    }

    glfwTerminate();

    exit(EXIT_SUCCESS);
}

/////////////////////  FUNCION PARA MOSTRAR INFO EN TITULO DE VENTANA  //////////////
void show_info() {
    static int fps = 0;
    static double last_tt = 0;
    double elapsed, tt;
    char nombre_ventana[128];   // buffer para modificar titulo de la ventana

    fps++;
    tt = glfwGetTime();  // Contador de tiempo en segundos

    elapsed = (tt - last_tt);
    if (elapsed >= 0.5)  // Refrescar cada 0.5 segundo
    {
        sprintf_s(nombre_ventana, 128, "%s: %4.0f FPS @ %d x %d", prac, fps / elapsed, ANCHO, ALTO);
        glfwSetWindowTitle(window, nombre_ventana);
        last_tt = tt;
        fps = 0;
    }

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////  ASIGNACON FUNCIONES CALLBACK
///////////////////////////////////////////////////////////////////////////////////////////////////////////



// Callback de cambio tama�o
void ResizeCallback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    ALTO = height;
    ANCHO = width;
}


static void KeyCallback(GLFWwindow *window, int key, int code, int action, int mode) {
    //fprintf(stdout, "Key %d Code %d Act %d Mode %d\n", key, code, action, mode);
    switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
    }
}


void asigna_funciones_callback(GLFWwindow *window) {
    glfwSetWindowSizeCallback(window, ResizeCallback);
    glfwSetKeyCallback(window, KeyCallback);
}










