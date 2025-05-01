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
            { // posicion vertice        // color vertice
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
                    2, 7, 3,};

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
    prog = Compile_Link_Shaders(vertex_prog, fragment_prog); // Mandar programas a GPU, compilar y crear programa en GPU
    glUseProgram(prog);    // Indicamos que programa vamos a usar

    glEnable(GL_CULL_FACE);

    glEnable(GL_DEPTH_TEST);  // Habilita el test de profundidad (Z-Buffer)
    glDepthFunc(GL_LESS);     // Un fragmento se dibuja si su Z es MENOR que el almacenado

}

float d = 8.0f, az = 0.0f, el = 0.6f;

vec3 pos_obs = d*vec3(sin(az)*cos(el),  cos(az)*cos(el),  sin(el) );
void update_camera_position() {
    pos_obs = d * vec3(sin(az) * cos(el),  // X = d·sin(az)·cos(el)
                       cos(az) * cos(el),  // Y = d·cos(az)·cos(el)
                       sin(el));           // Z = d·sin(el)
}

//vec3 pos_obs = vec3(0.0f, 6.0f, 4.0f);
vec3 target = vec3(0.0f, 0.0f, 0.0f);
vec3 up = vec3(0, 0, 1);

float zfar = 25.0f;
float znormear = 1.0f;

mat4 Proy, View, M;


// Actualizar escena: cambiar posici�n objetos, nuevos objetros, posici�n c�mara, luces, etc.
void render_scene() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Especifica color para el fondo (RGB+alfa)
    //glClear(GL_COLOR_BUFFER_BIT);          // Aplica color asignado borrando el buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    float t = (float) glfwGetTime();  // Contador de tiempo en segundos

    update_camera_position();  

    ///////// C�digo para actualizar escena  /////////
    Proy = perspective(40.0f, 4.0f / 3.0f, znormear, zfar);  //40� Y-FOV,  4:3 ,  znormEAR, ZFAR
    View = lookAt(pos_obs, target, up);  // Pos camara, Lookat, head up

    mat4 T, R, S;

    M = rotate(30.0f, vec3(0.0f, 0.0f, 1.0f));

    transfer_mat4("MVP", Proy * View * M);
    dibujar_indexado(obj1);

    ////////////////////////////////////////////////////////

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

bool mouse_pressed = false;  // Para arrastrar la cámara
bool mouse_right_pressed = false;
double xp = 0, yp = 0;  // Última posición del ratón
GLfloat zp = 0;

float xnorm, ynorm, znorm;


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

//Callback Scroll
void scroll(GLFWwindow* window, double dx, double dy){

    d += (dy > 0) ? 0.25f : (dy < 0) ? -0.25f : 0.0f;

    // Limitar d entre 4 y 22
    d = glm::clamp(d, 4.0f, 22.0f); 

    fprintf(stdout, "Scroll: dy=%.1f, d=%.1f\n", dy, d);

}

//Callback mover
void mover(GLFWwindow* window, double x, double y){
    if (mouse_pressed) {
        // Calcular diferencia respecto a la última posición
        double dx = x - xp;
        double dy = y - yp;

        fprintf(stdout, "Desplazamiento: (%.1f, %.1f)\n", dx, dy);

        // Sensibilidad (ajustable)
        float F = 0.005f * d;

        // Actualizar ángulos
        az -= dx * -F;  // Rotación horizontal
        el -= dy * -F;  // Rotación vertical

        fprintf(stdout, "Angulos: az=%.2f rad, el=%.2f rad\n", az, el);

        // Limitar elevación para evitar volteretas
        el = glm::clamp(el, -1.5f, 1.5f); // +- 85º

        // Guardar posición actual para el próximo frame
        xp = x;
        yp = y;
    }
}

//Callback pulsar
void pulsar(GLFWwindow* window, int Button, int Action, int Mode) {
    fprintf(stdout, "Botón: %d | Acción: %d | Mod: %d\n", Button, Action, Mode);
    if (Button == GLFW_MOUSE_BUTTON_LEFT) {
        mouse_pressed = (Action == GLFW_PRESS);  // True al pulsar, False al soltar


        fprintf(stdout, "Botón izquierdo: %s\n", mouse_pressed ? "PULSADO" : "LIBERADO");

        // Guardar posición inicial al pulsar
        if (mouse_pressed) {
            glfwGetCursorPos(window, &xp, &yp);

            fprintf(stdout, "xp= %.0f yp= %.0f\n", xp, yp);
        }
    }
    if (Button == GLFW_MOUSE_BUTTON_RIGHT) {
        mouse_right_pressed = (Action == GLFW_PRESS);  // True al pulsar, False al soltar


        fprintf(stdout, "Botón derecho: %s\n", mouse_right_pressed ? "PULSADO" : "LIBERADO");

        // Guardar posición inicial al pulsar
        if (mouse_right_pressed) {
            glfwGetCursorPos(window, &xp, &yp);
            yp = (ALTO-yp);

            glReadPixels((GLint)xp,(GLint)yp,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&zp); 
            fprintf(stdout, "xp= %.0f yp= %.0f zp= %.3f\n", xp, yp, zp);

            xnorm = (2*xp/ANCHO)-1;
            ynorm = (2*yp/ALTO)-1;
            znorm = (2*zp) -1;
            fprintf(stdout, "xnorm= %.3f ynorm= %.3f znorm= %.3f\n", xnorm, ynorm, znorm);

            float zcam = - ((-2*zfar*znormear)/((zfar+znormear)+(znorm*(-zfar+znormear))));

            float xcam = -(xnorm*zcam/Proy[0][0]);
            float ycam = -(ynorm*zcam/Proy[1][1]);

            fprintf(stdout, "xcam= %.3f ycam= %.3f zcam= %.3f\n", xcam, ycam, zcam);
        }
    }
} 

void asigna_funciones_callback(GLFWwindow *window) {
    glfwSetWindowSizeCallback(window, ResizeCallback);
    glfwSetKeyCallback(window, KeyCallback);

    glfwSetScrollCallback(window,scroll);
    glfwSetCursorPosCallback(window,mover);
    glfwSetMouseButtonCallback(window,pulsar) ;

}



