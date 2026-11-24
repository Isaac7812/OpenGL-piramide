# **Guía de estudio para sustentar tu proyecto de OpenGL**

La idea es que no tengas que memorizar el código completo, sino que entiendas **qué hace cada función, por qué existe, qué información recibe y qué resultado produce**.

Voy a organizar la explicación desde el funcionamiento general del programa hasta las preguntas técnicas que podría hacerte el profesor, incluyendo respuestas que puedes utilizar durante la sustentación.

# **1. Funcionamiento general del proyecto**

Tu proyecto integra cinco sistemas principales:

**main()**

Coordina todo el programa

**Importación**

Automóvil y escenario glTF

**Texturizado**

Carga de imágenes

**Iluminación**

Modelo Phong

**Movimiento**

Bézier compuesta

**Ciclo de renderizado**

Actualiza la cámara, la luz y el automóvil

**Resultado en pantalla**

Escenario tridimensional interactivo

**Pregunta del profesor: ¿Cómo funciona su proyecto?**

*Respuesta:* El programa importa dos modelos tridimensionales en formato glTF, almacena su geometría y texturas en la tarjeta gráfica y los renderiza mediante OpenGL. El automóvil se mueve utilizando una curva de Bézier compuesta, mientras que la iluminación se calcula dinámicamente mediante el modelo Phong. También implementé una cámara libre y una fuente luminosa que puede desplazarse mediante el teclado.

# **2. La función&#x20;**`main()`

La función `main()` es el punto de entrada del programa. Es la encargada de inicializar, coordinar y finalizar todos los procesos.

Puedes dividirla mentalmente en cuatro etapas:

| Etapa          | Función                                       |
| -------------- | --------------------------------------------- |
| Inicialización | Preparar OpenGL y crear la ventana.           |
| Carga          | Importar modelos, texturas y shaders.         |
| Ejecución      | Actualizar y dibujar continuamente la escena. |
| Finalización   | Liberar la memoria y cerrar el programa.      |

## **2.1. Inicialización de OpenGL**

```
if(!glfwInit()){
    std::cerr<<"No se pudo iniciar GLFW\n";
    return 1;
}
```

Esta instrucción inicializa GLFW. Si falla, el programa termina devolviendo un código de error.

```
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
```

Estas instrucciones solicitan la versión 3.3 de OpenGL.

```
GLFWwindow* window=glfwCreateWindow(
    WIDTH,HEIGHT,
    "Aston Martin - Phong y Bezier",
    nullptr,nullptr
);
```

Aquí se crea la ventana donde se visualizará el proyecto, con unas dimensiones iniciales de 1000 × 700 píxeles.

```
glfwMakeContextCurrent(window);
glfwSwapInterval(1);
```

La primera instrucción establece el contexto gráfico activo. La segunda solicita sincronizar la presentación de fotogramas con la frecuencia de actualización del monitor.

```
gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)
```

GLAD carga las funciones de OpenGL disponibles en el contexto gráfico.

**Pregunta: ¿Cuál es la diferencia entre GLAD y GLFW?**

GLFW permite crear ventanas, gestionar el contexto gráfico y recibir entradas del usuario. GLAD permite acceder a las funciones de OpenGL.

## **2.2. Prueba de profundidad**

```
glEnable(GL_DEPTH_TEST);
glDisable(GL_CULL_FACE);
```

`GL_DEPTH_TEST` activa la prueba de profundidad para determinar qué superficies están delante de otras.

`GL_CULL_FACE` controla el descarte de caras según su orientación. Inicialmente lo desactivo para evitar descartar superficies que podrían necesitar dibujarse por ambos lados.

**Sin prueba de profundidad**

Un objeto lejano dibujado después puede ocultar al cercano.

**Con prueba de profundidad**

El objeto cercano oculta correctamente al lejano.

**Pregunta: ¿Qué pasaría si desactivara GL_DEPTH_TEST?**

Los objetos podrían superponerse incorrectamente, dependiendo del orden en que se dibujen y no de su profundidad real.

# **3. ¿Cómo se importan los modelos tridimensionales?**

## **3.1. Formato glTF**

El automóvil y el escenario están compuestos por tres tipos de recursos.

| Recurso      | Contenido                                    |
| ------------ | -------------------------------------------- |
| `scene.gltf` | Estructura, materiales, nodos y referencias. |
| `scene.bin`  | Datos binarios de la geometría.              |
| `textures/`  | Imágenes de las texturas.                    |

En tu proyecto utilizas:

```
tg3_model scene = {}, nieve = {};
```

`scene` almacena el modelo del automóvil y `nieve` almacena el escenario, cuando ambos están en su estado habitual.

La función:

```
tg3_parse_file(...)
```

interpreta el archivo glTF y permite acceder a su información desde C++.

**Pregunta: ¿Usted programó la geometría del automóvil y del escenario?**

No. Ambos modelos fueron descargados de Sketchfab. Programé su importación, procesamiento, texturizado, iluminación y renderizado. También programé la trayectoria del automóvil y la esfera que representa la fuente luminosa.

## **3.2. Funciones para leer la geometría**

| Función            | ¿Qué hace?                                               |
| ------------------ | -------------------------------------------------------- |
| `attributeIndex()` | Busca un atributo por su nombre.                         |
| `accessorData()`   | Localiza los datos dentro del buffer binario.            |
| `readFloats()`     | Lee posiciones, normales y coordenadas UV de tipo float. |
| `readIndex()`      | Lee los índices de la geometría.                         |
| `uploadScene()`    | Prepara y envía los modelos a la GPU.                    |

Debes entender especialmente la diferencia entre buffer, buffer view y accessor.

**Buffer**

Contiene todos los datos binarios.

**Buffer view**

Selecciona una región de esos datos.

**Accessor**

X, Y, Z

X, Y, Z

X, Y, Z

Describe cómo interpretar los datos, por ejemplo, como vectores de tres componentes.

**Pregunta: ¿Qué hace&#x20;**`accessorData()`**?**

Calcula la ubicación exacta de un elemento dentro de los datos binarios, considerando los desplazamientos y la separación entre elementos. También comprueba que no se acceda fuera de los límites del buffer.

# **4. Vértices, normales y coordenadas UV**

En el código tienes:

```
struct Vertex {
    glm::vec3 pos,normal;
    glm::vec2 uv;
};
```

Cada vértice almacena ocho valores numéricos: tres de posición, tres de normal y dos de textura.

**Posición (X, Y, Z)**

Determina dónde se encuentra el vértice en el espacio tridimensional.

**Normal (NX, NY, NZ)**

Representa la orientación de la superficie y permite calcular cómo recibe la luz.

**Coordenadas UV (U, V)**

Indican qué parte de la imagen de textura corresponde a cada vértice.

**Pregunta: ¿Para qué sirven las normales?**

Para determinar la orientación de una superficie respecto a la luz y calcular las componentes difusa y especular de Phong.

**Pregunta: ¿Qué pasaría si no tuviera coordenadas UV?**

No podría asignar correctamente la imagen de textura a la geometría utilizando el método actual.

# **5. VAO, VBO y EBO**

Estos tres conceptos son muy importantes en OpenGL.

```
glGenVertexArrays(1,&g.vao);
glGenBuffers(1,&g.vbo);
glGenBuffers(1,&g.ebo);
```

### **Ejemplo: construir un cuadrado con dos triángulos**

VBO: almacena los cuatro vértices.

V0

V1

V2

V3

EBO: almacena los seis índices de los dos triángulos.

0

1

2

0

2

3

**VAO:** guarda la configuración necesaria para interpretar los atributos y utilizar los buffers.

| Elemento | Significado           | Función                             |
| -------- | --------------------- | ----------------------------------- |
| VAO      | Vertex Array Object   | Configuración de los atributos.     |
| VBO      | Vertex Buffer Object  | Almacena los datos de los vértices. |
| EBO      | Element Buffer Object | Almacena los índices.               |

La función `glBufferData()` transfiere los datos a los buffers de OpenGL.

La función `glVertexAttribPointer()` indica cómo están organizados los atributos de cada vértice.

Finalmente:

```
glDrawElements(g.mode,g.count,GL_UNSIGNED_INT,0);
```

ordena dibujar la geometría utilizando los índices almacenados.

**Pregunta: ¿Por qué utilizar un EBO?**

Porque permite reutilizar vértices que pertenecen a varios triángulos, evitando almacenar repetidamente la misma información.

**Pregunta: ¿El VAO almacena los vértices?**

No. Los vértices se almacenan en el VBO. El VAO almacena la configuración que permite acceder correctamente a sus atributos y al buffer de índices.

# **6. ¿Cómo funcionan las texturas?**

Una textura es una imagen que se aplica sobre una superficie tridimensional para darle color y detalles visuales sin necesidad de aumentar la cantidad de geometría.

En tu proyecto, las texturas se cargan dentro de `uploadScene()`.

```
unsigned char* datos=stbi_load(
    ruta.c_str(),&ancho,&alto,&canales,4
);
```

Esta función lee una imagen y obtiene sus píxeles, dimensiones y cantidad de canales originales.

El número 4 indica que quieres obtener los datos en formato RGBA.

| Canal | Significado   |
| ----- | ------------- |
| R     | Rojo          |
| G     | Verde         |
| B     | Azul          |
| A     | Transparencia |

Después utilizas:

```
glGenTextures(1,&tex);
glBindTexture(GL_TEXTURE_2D,tex);
```

Estas instrucciones crean una textura de OpenGL y la seleccionan para trabajar con ella.

```
glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,
    ancho,alto,0,GL_RGBA,GL_UNSIGNED_BYTE,datos);
```

Esta instrucción envía los píxeles de la imagen a la tarjeta gráfica.

```
glGenerateMipmap(GL_TEXTURE_2D);
```

Genera versiones de menor resolución de la textura para mejorar su visualización a diferentes distancias.

Los mipmaps contienen versiones progresivamente más pequeñas de una textura.

Finalmente:

```
stbi_image_free(datos);
```

Libera la memoria de la imagen cargada en la CPU, porque los datos necesarios ya fueron enviados a OpenGL.

**Pregunta: ¿Cuál es la diferencia entre una textura y un material?**

Una textura es una imagen. Un material contiene propiedades que determinan la apariencia de una superficie, como su color, transparencia, rugosidad y las texturas que utiliza.

**Pregunta: ¿Cuántas texturas utiliza el proyecto?**

El código contempla la carga de 21 imágenes del automóvil y 29 del escenario, aunque el shader utiliza principalmente las texturas de color base.

**Pregunta: ¿Utiliza mapas de normales?**

El programa carga esos archivos, pero no aplica normal mapping. Para calcular Phong utiliza las normales geométricas de los vértices.

# **7. Shaders**

Los shaders son programas que se ejecutan en la GPU. Están escritos en GLSL, que es el lenguaje de sombreado de OpenGL.

En tu proyecto tienes cuatro shaders agrupados en dos programas.

| Shader    | Función                                 |
| --------- | --------------------------------------- |
| `sceneVS` | Transforma los vértices de los modelos. |
| `sceneFS` | Calcula texturas e iluminación Phong.   |
| `lightVS` | Transforma los vértices de la esfera.   |
| `lightFS` | Asigna el color de la esfera luminosa.  |

## **7.1. Vertex shader**

```
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
```

Recibe los atributos de los vértices almacenados en el VBO.

Los números 0, 1 y 2 corresponden a las ubicaciones configuradas mediante `glVertexAttribPointer()`.

Después realiza:

```
vec4 world = model * vec4(aPos,1.0);
```

Transforma la posición local del vértice a una posición dentro del mundo tridimensional.

```
gl_Position = proj * view * world;
```

Calcula la posición del vértice en el espacio de recorte, que OpenGL utilizará para proyectarlo sobre la pantalla.

## **7.2. Fragment shader**

Recibe las posiciones, normales y coordenadas UV interpoladas durante la rasterización.

Su función principal es determinar el color de cada fragmento.

```
FragColor = vec4(clamp(color,0.0,1.0),base.a);
```

Esta instrucción establece el color final, incluyendo su transparencia.

**Vértices**

Datos almacenados en el VBO

**Vertex Shader**

Transformaciones tridimensionales

**Rasterización**

Generación de fragmentos e interpolación

**Fragment Shader**

Texturas e iluminación Phong

**Framebuffer**

Resultado gráfico

**Pregunta: ¿Cuál es la diferencia entre el vertex shader y el fragment shader?**

El vertex shader trabaja con los vértices y sus transformaciones. El fragment shader trabaja con los fragmentos generados durante la rasterización y calcula su color.

**Pregunta: ¿Por qué se utilizan shaders?**

Porque permiten programar el comportamiento gráfico directamente en la GPU, aprovechando su capacidad de procesamiento paralelo.

## **7.3. Compilación de los shaders**

Las funciones `compileShader()` y `makeProgram()` se encargan de preparar los shaders.

| Instrucción         | Función                         |
| ------------------- | ------------------------------- |
| `glCreateShader()`  | Crea un shader.                 |
| `glShaderSource()`  | Asigna el código GLSL.          |
| `glCompileShader()` | Compila el código.              |
| `glAttachShader()`  | Asocia un shader a un programa. |
| `glLinkProgram()`   | Enlaza los shaders.             |
| `glUseProgram()`    | Activa el programa gráfico.     |

**Pregunta: ¿Qué es una variable uniform?**

Es una variable que se establece desde el programa C++ y cuyo valor permanece uniforme para las ejecuciones del shader durante una llamada de dibujo.

Por ejemplo:

```
uniform vec3 lightPos;
```

Desde C++ puedes modificarla mediante:

```
glUniform3fv(
    glGetUniformLocation(sceneProgram,"lightPos"),
    1,glm::value_ptr(lightPos)
);
```

Así se comunica la posición de la luz desde el programa principal al fragment shader.

# **8. Matrices de transformación**

Este tema es importante porque está relacionado con prácticamente todo el proyecto.

Utilizas tres matrices principales:

**Matriz model**

Determina la posición, rotación y escala del objeto dentro del mundo.

**Matriz view**

Transforma el mundo respecto a la posición y orientación de la cámara.

**Matriz proj**

Define cómo se proyecta el espacio tridimensional, incluyendo el efecto de perspectiva.

El orden de multiplicación es:

\\[ P\_{clip}=Projection \times View \times Model \times P \\]

**Pregunta: ¿Qué diferencia existe entre traslación, rotación y escala?**

- Traslación: cambia la posición.
- Rotación: cambia la orientación.
- Escala: cambia las dimensiones.

## **8.1. Ajuste del tamaño del automóvil**

```
glm::vec3 dimensiones=maximo-minimo;

float largo=std::max(
    dimensiones.x,dimensiones.z
);

float escala=4.5f/largo;
```

La función `limitesNodo()` calcula los límites espaciales del automóvil considerando su jerarquía de nodos.

Después calculas sus dimensiones y obtienes un factor de escala para que su dimensión horizontal mayor sea de 4.5 unidades.

Por ejemplo, si el automóvil original midiera 9 unidades:

\\[ Escala=\frac{4.5}{9}=0.5 \\]

El modelo quedaría reducido a la mitad de su tamaño.

**Pregunta: ¿Por qué no dejó el tamaño original del automóvil?**

Porque los modelos descargados pueden utilizar diferentes escalas. Necesitaba ajustar las dimensiones del automóvil para que fueran proporcionales a la carretera.

## **8.2. Jerarquía de nodos**

La función `nodeTransform()` obtiene la transformación original de cada nodo.

La función `drawNode()` recorre recursivamente la jerarquía.

```
glm::mat4 world=parent*nodeTransform(n);
```

**Nodo principal**

Transformación del automóvil

**Carrocería**

Transformación local

**Ruedas**

Transformación local

**Interior**

Transformación local

Ejemplo conceptual de una jerarquía de nodos.

Si se mueve el nodo padre, sus hijos heredan esa transformación.

**Pregunta: ¿Qué es una función recursiva?**

Es una función que se llama a sí misma. En este proyecto la utilizo para recorrer los nodos hijos de cada modelo, independientemente de cuántos niveles tenga su jerarquía.

# **9. Iluminación Phong — Tema fundamental**

El modelo Phong es una técnica que aproxima el comportamiento de la iluminación sobre las superficies.

Se compone de tres elementos:

**Ambiental**

Iluminación mínima

**Difusa**

Orientación respecto a la luz

**Especular**

Reflejo brillante

**Phong**

Combinación de las tres

## **9.1. Los cuatro vectores de Phong**

En tu fragment shader tienes:

```
vec3 n = normalize(Normal);
vec3 l = normalize(lightPos-FragPos);
vec3 v = normalize(viewPos-FragPos);
vec3 r = reflect(-l,n);
```

| Vector | Significado                |
| ------ | -------------------------- |
| N      | Normal de la superficie.   |
| L      | Dirección hacia la luz.    |
| V      | Dirección hacia la cámara. |
| R      | Dirección de reflexión.    |

**Pregunta: ¿Por qué normaliza los vectores?**

Porque quiero que tengan una longitud igual a uno, de manera que el producto punto represente la relación angular entre sus direcciones.

## **9.2. Componente ambiental**

```
float ambient = 0.06;
```

```
vec3 luzAmbiente = ambient * base.rgb;
```

Proporciona una iluminación mínima a todas las superficies.

El valor 0.06 representa un factor ambiental del 6 % aplicado al color base en espacio lineal.

**Factor ambiental**

## **0.06**

Restablecer valor del proyectoSimulación ilustrativa del efecto de la iluminación ambiental.

**Pregunta: ¿Por qué utilizó 0.06?**

Porque quería un ambiente nocturno que permitiera distinguir claramente los cambios producidos por la fuente luminosa.

## **9.3. Componente difusa**

```
float diffuse = max(dot(n,l),0.0);
```

La iluminación difusa depende del ángulo entre la normal y la dirección hacia la luz.

\\[ I_d=\max(N\cdot L,0) \\]

Como ambos vectores están normalizados, su producto punto equivale al coseno del ángulo entre ellos.

Ángulo entre N y L

# **45°**

Componente difusa

# **0.707**

0°

45°

90°

| Ángulo       | Iluminación difusa |
| ------------ | ------------------ |
| 0°           | 1.0                |
| 45°          | 0.707              |
| 60°          | 0.5                |
| 90°          | 0.0                |
| Mayor de 90° | 0.0                |

**Pregunta: ¿Para qué sirve&#x20;**`max()`**?**

Para evitar valores negativos de iluminación cuando la superficie está orientada en sentido contrario a la luz.

## **9.4. Componente especular**

```
float specular =
    pow(max(dot(v,r),0.0),32.0)
    * brillo
    * step(0.0,diffuse-0.0001);
```

Representa los reflejos brillantes sobre una superficie.

Compara la dirección de reflexión de la luz con la dirección hacia la cámara.

La potencia 32 determina qué tan concentrado es el reflejo.

**Exponente especular**

## **32**

Amplio

Concentrado

Valor del proyecto: 32Representación ilustrativa de cómo cambia la concentración del reflejo.

**Pregunta: ¿Qué pasa si aumenta el exponente de 32 a 128?**

El reflejo se vuelve más concentrado y estrecho. No significa necesariamente que su intensidad máxima aumente.

**Pregunta: ¿Cuál es la diferencia entre la componente difusa y especular?**

La difusa depende de la orientación de la superficie respecto a la luz. La especular también depende de la posición de la cámara.

## **9.5. Atenuación por distancia**

```
float distancia = length(lightPos - FragPos);

float atenuacion = 1.0 /
    (1.0 + 0.035 * distancia
         + 0.008 * distancia * distancia);
```

La atenuación hace que la iluminación disminuya conforme aumenta la distancia entre la superficie y la fuente luminosa.

\\[ A(d)=\frac{1}{1+0.035d+0.008d^2} \\]

Distancia

# **10 unidades**

Factor de atenuación

# **46.5%**

0%30%60%90%120%05101520253035404550

**Pregunta: ¿Qué pasaría si elimina la atenuación?**

La intensidad difusa y especular no disminuiría directamente con la distancia, aunque seguiría dependiendo de los ángulos de iluminación.

## **9.6. Fórmula final de iluminación**

```
vec3 color = luzAmbiente +
    (luzDifusa + luzEspecular) * atenuacion;
```

Esta es la combinación final de las tres componentes.

\\[ Color=Ambiental+(Difusa+Especular)A(d) \\]

Recuerda estos valores de tu proyecto:

| Propiedad             | Valor             |
| --------------------- | ----------------- |
| Factor ambiental      | 0.06              |
| Exponente especular   | 32                |
| Intensidad de la luz  | 3.5               |
| Atenuación constante  | 1.0               |
| Atenuación lineal     | 0.035             |
| Atenuación cuadrática | 0.008             |
| Color de la luz       | (1.0, 0.94, 0.84) |

**Pregunta: ¿Phong y PBR son lo mismo?**

No. Phong es un modelo de iluminación empírico basado en componentes ambiental, difusa y especular. PBR busca representar de forma más físicamente fundamentada el comportamiento de los materiales y la luz.

Mi proyecto utiliza Phong y aprovecha algunas propiedades de los materiales glTF para ajustar el brillo.

# **10. Curvas de Bézier — Tema fundamental**

Una curva de Bézier es una curva paramétrica definida mediante puntos de control.

En tu proyecto utilizas curvas cúbicas, es decir, de grado tres, con cuatro puntos de control por segmento.

La fórmula es:

\\[ B(t)=(1-t)^3P_0+3(1-t)^2tP_1 \\]

\\[ +3(1-t)t^2P_2+t^3P_3 \\]

| Punto | Función                        |
| ----- | ------------------------------ |
| P₀    | Inicio de la curva.            |
| P₁    | Controla la dirección inicial. |
| P₂    | Controla la dirección final.   |
| P₃    | Final de la curva.             |

## **10.1. Funcionamiento interactivo**

Puedes modificar los puntos de control y el parámetro t para entender cómo funciona la fórmula.

Progreso t

## **0.50**

Altura del control P₁

145

Altura del control P₂

145

Restablecer curva

**Pregunta: ¿Qué significa el parámetro t?**

Representa el progreso dentro de la curva. Cuando vale cero, se obtiene el punto inicial. Cuando vale uno, se obtiene el punto final.

**Pregunta: ¿La curva pasa por los cuatro puntos de control?**

No necesariamente. Pasa por los puntos inicial y final. Los dos puntos internos determinan su forma.

## **10.2. ¿Por qué Bézier compuesta?**

Tu carretera tiene curvas y desniveles, por lo que utilizas doce puntos tridimensionales.

```
const glm::vec3 ruta[] = {
    {289,-8.9f,-284.5f},
    {300,-10.2f,-286.5f},
    {316,-12.5f,-290},
    {328,-14.5f,-293.7f},
    {336,-15.6f,-292.8f},
    {340.3f,-16,-288.7f},
    {342.4f,-16.1f,-283.4f},
    {340,-16,-277.2f},
    {332,-15.9f,-271.9f},
    {320,-16.3f,-269},
    {305,-17.2f,-265},
    {294,-18,-261.9f}
};
```

Estos doce puntos forman once segmentos.

Vista superior calculada a partir de las coordenadas X y Z de tu código.

**Pregunta: ¿Por qué utiliza coordenadas X, Y y Z?**

Porque el recorrido es tridimensional. X y Z determinan el desplazamiento horizontal, mientras que Y permite seguir las pendientes de la carretera.

## **10.3. Selección del segmento**

```
float x = glm::clamp(t, 0.f, 1.f) * (N - 1);

int i = std::min(int(x), N - 2);
```

La primera instrucción convierte el progreso global en una posición dentro de los once segmentos.

La segunda determina el índice del segmento actual.

```
float v = x - i;
float u = 1.f - v;
```

Estas instrucciones calculan el progreso local dentro del segmento.

**Progreso global**

## **0.50**

x = t × 11

## **5.50**

Índice i

## **5**

Progreso local v

## **0.50**

Por ejemplo, cuando t = 0.5:

\\[ x=0.5\times11=5.5 \\]

\\[ i=5 \\]

\\[ v=5.5-5=0.5 \\]

El automóvil está a la mitad del sexto segmento.

**Pregunta: ¿Para qué utiliza&#x20;**`clamp()`**&#x20;y&#x20;**`min()`**?**

Para mantener el parámetro dentro de los límites válidos y evitar acceder a posiciones inexistentes del arreglo, especialmente cuando t llega a uno.

## **10.4. Cálculo de los puntos de control**

```
glm::vec3 a = ruta[i], d = ruta[i+1];
```

Los puntos a y d son los extremos del segmento.

```
glm::vec3 b = a +
    (d - ruta[std::max(0,i-1)]) / 6.f;

glm::vec3 c = d -
    (ruta[std::min(N-1,i+2)] - a) / 6.f;
```

Los puntos b y c se calculan utilizando los puntos vecinos.

Esto permite conectar los segmentos con tangentes compatibles, evitando cambios bruscos de dirección.

Los controles vecinos permiten conservar una dirección suave en la unión.

**Pregunta: ¿Por qué divide entre seis?**

Porque utiliza una conversión de tangentes de tipo Catmull-Rom a puntos de control de Bézier cúbica.

Si el profesor te pide profundizar, la tangente interna se estima mediante la diferencia entre los puntos vecinos dividida entre dos. Como la derivada de una Bézier cúbica en su extremo contiene un factor de tres, se obtiene el factor total de un sexto.

## **10.5. Aplicación de la fórmula**

```
return u*u*u*a
    + 3*u*u*v*b
    + 3*u*v*v*c
    + v*v*v*d
    - origen;
```

Esta instrucción calcula la posición tridimensional del automóvil.

Es exactamente la fórmula de Bézier cúbica, donde:

- a equivale a P₀.
- b equivale a P₁.
- c equivale a P₂.
- d equivale a P₃.
- v equivale al parámetro local.
- u equivale a uno menos v.

El vector `origen` se resta para trasladar la trayectoria al mismo sistema de coordenadas que el escenario.

```
const glm::vec3 origen(320,-15,-280);
```

**Pregunta: ¿Qué pasaría si elimina&#x20;**`-origen`**?**

El automóvil seguiría la trayectoria en las coordenadas originales, pero el escenario estaría trasladado. Por lo tanto, ambos dejarían de coincidir espacialmente.

# **11. ¿Cómo gira el automóvil?**

Calcular la posición no es suficiente. También necesitas determinar hacia dónde debe mirar el automóvil.

Para ello utilizas:

```
glm::vec3 tangente(float t){
    return bezier(std::min(1.f,t+.001f))
         - bezier(std::max(0.f,t-.001f));
}
```

Esta función calcula dos posiciones cercanas de la curva y las resta.

El resultado aproxima la dirección tangente.

## **11.1. Rotación horizontal — Yaw**

```
float angulo =
    std::atan2(direccion.x,direccion.z)
    + glm::radians(CAR_YAW_OFFSET);
```

La función `atan2()` calcula un ángulo a partir de las componentes X y Z del vector dirección.

Después aplicas:

```
modelo=glm::rotate(
    modelo,angulo,glm::vec3(0,1,0)
);
```

Esto rota el automóvil alrededor del eje Y.

## **11.2. Inclinación vertical — Pitch**

```
float inclinacion=std::atan2(
    direccion.y,
    glm::length(
        glm::vec2(direccion.x,direccion.z)
    )
);
```

Aquí calculas el ángulo entre la dirección del automóvil y el plano horizontal.

Después aplicas la rotación alrededor de su eje X local.

**Yaw — Eje Y**

Girar a izquierda o derecha.

**Pitch — Eje X**

Subir o bajar pendientes.

**Pregunta: ¿Por qué utiliza&#x20;**`atan2()`**&#x20;y no&#x20;**`atan()`**?**

Porque atan2 considera los signos de ambas componentes y determina correctamente el cuadrante del ángulo, además de manejar casos en los que una componente es cero.

**Pregunta: ¿Está simulando la dirección del volante o las físicas del automóvil?**

No. Estoy calculando matemáticamente la orientación del modelo completo a partir de la dirección de la trayectoria.

# **12. Tiempo y animación**

Dentro del ciclo principal tienes:

```
float now=static_cast<float>(glfwGetTime()),
      dt=std::min(now-lastFrame,0.1f);

lastFrame=now;
```

La variable `dt`, conocida como delta time, representa el tiempo transcurrido entre fotogramas, limitado a 0.1 segundos.

Esto permite que los movimientos sean aproximadamente independientes de los FPS en condiciones normales.

Por ejemplo, para un movimiento de seis unidades por segundo:

| FPS | Desplazamiento por fotograma | Desplazamiento por segundo |
| --- | ---------------------------- | -------------------------- |
| 30  | 0.2                          | 6                          |
| 60  | 0.1                          | 6                          |
| 120 | 0.05                         | 6                          |

## **12.1. Duración del recorrido**

```
tiempoAnimacion+=dt;

float t=std::fmod(
    tiempoAnimacion/24.0f,1.0f
);
```

Divides el tiempo acumulado entre 24 para establecer la duración de la animación.

La función `fmod()` obtiene el residuo de una división de números decimales.

Tiempo transcurrido

# **12.0 s**

Parámetro t

# **0.500**

Al completar 24 segundos, el parámetro vuelve a cero y comienza otro recorrido.

**Pregunta: ¿Cómo haría que el automóvil se moviera más rápido?**

Cambiaría el divisor de 24 a un valor menor, por ejemplo 12, para reducir la duración del recorrido a la mitad.

**Pregunta: ¿El automóvil tiene velocidad constante?**

No necesariamente. El parámetro avanza uniformemente con el tiempo, pero los segmentos tienen diferentes longitudes. Para conseguir una velocidad espacial constante necesitaría parametrizar la curva por longitud de arco.

**Pregunta: ¿La curva es cerrada?**

No. Tiene un punto inicial y otro final diferentes. Cuando termina el recorrido, el automóvil regresa directamente al inicio mediante el reinicio del parámetro.

# **13. Cámara libre**

La cámara está definida mediante:

```
glm::vec3 cameraPos(5.0f,24.0f,35.0f);
glm::vec3 cameraFront(0.0f,-0.574f,-0.819f);
glm::vec3 cameraUp(0,1,0);
```

| Variable      | Significado                       |
| ------------- | --------------------------------- |
| `cameraPos`   | Posición de la cámara.            |
| `cameraFront` | Dirección hacia donde mira.       |
| `cameraUp`    | Dirección vertical de referencia. |

Cuando presionas W:

```
cameraPos+=cameraFront*speed;
```

Cuando presionas S:

```
cameraPos-=cameraFront*speed;
```

Para desplazarte lateralmente calculas:

```
glm::vec3 right=glm::normalize(
    glm::cross(cameraFront,cameraUp)
);
```

El producto cruz genera un vector perpendicular a los otros dos.

## **13.1. Movimiento con el ratón**

La función `mouseCallback()` recibe las coordenadas del ratón.

```
yaw+=dx*0.1f;
pitch+=dy*0.1f;

pitch=glm::clamp(pitch,-89.0f,89.0f);
```

El desplazamiento horizontal modifica yaw y el vertical modifica pitch.

El valor 0.1 controla la sensibilidad del ratón.

Posteriormente utilizas seno y coseno para convertir estos ángulos en un vector de dirección.

## **13.2. Matriz de vista**

```
glm::mat4 view=glm::lookAt(
    cameraPos,
    cameraPos+cameraFront,
    cameraUp
);
```

Esta función construye la matriz de vista utilizando la posición de la cámara, el punto hacia el cual mira y la dirección vertical.

**Pregunta: ¿Por qué limita pitch entre -89 y 89 grados?**

Para evitar que la dirección frontal sea paralela a la dirección vertical de referencia, lo que provocaría problemas al calcular el vector lateral de la cámara.

**Pregunta: ¿Cómo aumentaría la velocidad de la cámara?**

Modificaría el valor 6.0 de `speed=6.0f*dt`. Actualmente, presionar Shift duplica esa velocidad.

# **14. Fuente luminosa interactiva**

La luz se inicializa mediante:

```
glm::vec3 lightPos(15.0f,5.0f,-8.0f);

glm::vec3 lightColor(1.0f,0.94f,0.84f);
```

La primera variable determina su posición inicial.

La segunda determina su color, ligeramente cálido.

El movimiento de la luz utiliza:

```
float ls=8.0f*dt;
```

Cada tecla modifica una componente de su posición.

| Tecla     | Modificación |
| --------- | ------------ |
| Izquierda | Disminuye X. |
| Derecha   | Aumenta X.   |
| Arriba    | Disminuye Z. |
| Abajo     | Aumenta Z.   |
| Q         | Aumenta Y.   |
| E         | Disminuye Y. |

## **14.1. Esfera luminosa**

La función `createSphere()` genera la geometría de la esfera mediante coordenadas esféricas.

```
const int stacks=16,slices=24;
```

```
verts.emplace_back(
    std::sin(phi)*std::cos(theta),
    std::cos(phi),
    std::sin(phi)*std::sin(theta)
);
```

La esfera se construye mediante divisiones verticales y horizontales conectadas por triángulos.

La esfera se posiciona utilizando:

```
glm::mat4 sphereModel=glm::scale(
    glm::translate(glm::mat4(1.0f),lightPos),
    glm::vec3(0.35f)
);
```

Se traslada a la posición de la luz y se escala a un radio de 0.35 unidades.

**Pregunta: ¿La esfera es la que ilumina el escenario?**

No. La esfera únicamente representa visualmente la posición de la luz. La iluminación se calcula mediante el fragment shader y la variable `lightPos`.

**Pregunta: ¿Qué pasaría si elimina el dibujo de la esfera?**

La iluminación seguiría funcionando, pero ya no tendría una representación visual de la posición de la fuente luminosa.

**Pregunta: ¿Qué tipo de luz utiliza?**

Una luz puntual, porque tiene una posición específica y su dirección respecto a cada fragmento se calcula a partir de esa posición.

# **15. ¿Cómo maneja dos modelos diferentes?**

Esta función puede llamar la atención del profesor:

```
void cambiarModelo(){
    std::swap(scene,nieve);
    gpuMeshes.swap(mallasNieve);
    gpuTextures.swap(texturasNieve);
    esNieve=!esNieve;
}
```

Su función es intercambiar los datos activos del automóvil y del escenario.

**Automóvil**

Modelo activo

**Escenario**

Datos en espera

Variable scene

**Aston Martin**

esNieve = false

&#x20;Ejecutar cambiarModelo()

Esto permite reutilizar las funciones `uploadScene()`, `drawMesh()` y `drawNode()`.

**Pregunta: ¿Está cargando los modelos nuevamente en cada fotograma?**

No. Los modelos y sus texturas se cargan inicialmente. Durante el renderizado solamente intercambio las estructuras que contienen sus datos y referencias a los recursos gráficos.

**Pregunta: ¿Por qué el escenario no se mueve junto con el automóvil?**

Porque el escenario utiliza una matriz de transformación fija, mientras que la matriz del automóvil se actualiza continuamente mediante la curva de Bézier.

# **16. Materiales transparentes**

En el código manejas tres modos de transparencia.

OPAQUE

Opaco

MASK

Recorte

BLEND

Mezcla

OPAQUE representa superficies completamente opacas.

MASK descarta fragmentos dependiendo de un umbral de transparencia.

BLEND mezcla el color del fragmento con el color que ya existe detrás de él.

En tu código:

```
if(alpha==2 && !transparentPass){
    transparentes.push_back({&g,world,esNieve});
    return;
}
```

Las mallas con transparencia BLEND se almacenan temporalmente para dibujarlas después de los objetos opacos.

```
glEnable(GL_BLEND);

glBlendFunc(
    GL_SRC_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA
);

glDepthMask(GL_FALSE);
```

Se activa la mezcla de colores y se desactiva temporalmente la escritura en el buffer de profundidad.

**Pregunta: ¿Por qué ordena los objetos transparentes de lejos a cerca?**

Porque la mezcla de colores depende del orden de dibujo. Dibujar primero los objetos transparentes más alejados mejora la composición visual.

En este proyecto el ordenamiento se realiza aproximadamente mediante la distancia entre el centro de cada malla y la cámara.

# **17. Ciclo principal de renderizado**

Debes poder explicar este ciclo sin necesidad de leer todas sus instrucciones.

```
while(!glfwWindowShouldClose(window))
```

Significa que el programa seguirá ejecutándose mientras la ventana no deba cerrarse.

En cada fotograma ocurre lo siguiente:

1

Calcular delta time

2

Detectar entradas del teclado y ratón

3

Actualizar cámara y posición de la luz

4

Limpiar los buffers de color y profundidad

5

Calcular matrices y enviar uniforms

6

Dibujar el escenario nevado

7

Calcular posición y orientación mediante Bézier

8

Dibujar el automóvil

9

Dibujar materiales transparentes

10

Dibujar esfera luminosa

11

Mostrar el fotograma

&#x20;Repetir el proceso

## **Funciones importantes del ciclo**

| Función              | Propósito                          |
| -------------------- | ---------------------------------- |
| `glfwPollEvents()`   | Procesar eventos de entrada.       |
| `glClearColor()`     | Establecer el color del fondo.     |
| `glClear()`          | Limpiar los buffers.               |
| `glm::lookAt()`      | Construir la matriz de vista.      |
| `glm::perspective()` | Construir la matriz de proyección. |
| `glUseProgram()`     | Seleccionar el programa gráfico.   |
| `glUniform()`        | Enviar valores al shader.          |
| `glDrawElements()`   | Dibujar geometría indexada.        |
| `glfwSwapBuffers()`  | Presentar el fotograma generado.   |

**Pregunta: ¿Por qué limpia la pantalla en cada fotograma?**

Porque la escena cambia continuamente. Necesito eliminar los datos del fotograma anterior para dibujar correctamente el nuevo estado de la escena.

**Pregunta: ¿Qué es el doble buffer?**

Es una técnica que permite mostrar un fotograma mientras se construye el siguiente en otro buffer. Cuando termina el renderizado, ambos se intercambian mediante `glfwSwapBuffers()`.

# **18. Liberación de recursos**

Al finalizar el programa utilizas funciones como:

```
glDeleteVertexArrays(...)
glDeleteBuffers(...)
glDeleteTextures(...)
glDeleteProgram(...)
```

Estas funciones liberan los recursos gráficos.

```
tg3_model_free(&nieve);
tg3_model_free(&scene);
```

Liberan la información de los modelos almacenada mediante TinyGLTF.

Finalmente:

```
glfwDestroyWindow(window);
glfwTerminate();
return 0;
```

Destruyen la ventana, finalizan GLFW y terminan el programa correctamente.

**Pregunta: ¿Por qué es importante liberar los recursos?**

Para evitar fugas de memoria y mantener una gestión adecuada de los recursos de la CPU y la GPU.

# **19. Preguntas difíciles que podría hacerte el profesor**

| Pregunta                                    | Respuesta                                                                                         |
| ------------------------------------------- | ------------------------------------------------------------------------------------------------- |
| ¿Qué versión de OpenGL utiliza?             | OpenGL 3.3 Core Profile.                                                                          |
| ¿Qué lenguaje utilizan los shaders?         | GLSL 330.                                                                                         |
| ¿Qué es un mesh?                            | Un conjunto de geometría que contiene primitivas.                                                 |
| ¿Qué es un nodo?                            | Un elemento de la jerarquía que puede contener transformaciones, mallas e hijos.                  |
| ¿Qué es un fragmento?                       | Un elemento generado durante la rasterización que puede contribuir a un píxel.                    |
| ¿Qué es el producto punto?                  | Una operación escalar que, entre vectores unitarios, representa el coseno del ángulo entre ellos. |
| ¿Qué es el producto cruz?                   | Una operación que genera un vector perpendicular a otros dos.                                     |
| ¿Qué es interpolar?                         | Calcular valores intermedios entre valores conocidos.                                             |
| ¿Qué grado tiene su Bézier?                 | Grado tres, es cúbica.                                                                            |
| ¿Cuántos segmentos utiliza?                 | Once.                                                                                             |
| ¿Cuántos puntos de ruta utiliza?            | Doce.                                                                                             |
| ¿Cuántos controles tiene cada segmento?     | Cuatro.                                                                                           |
| ¿Implementó sombras?                        | No, solamente iluminación Phong con atenuación.                                                   |
| ¿Implementó físicas?                        | No, el movimiento es paramétrico.                                                                 |
| ¿Implementó colisiones?                     | No.                                                                                               |
| ¿Implementó PBR?                            | No, utilicé Phong.                                                                                |
| ¿Las ruedas tienen animación independiente? | No, se transforma el automóvil completo.                                                          |
| ¿La luz sigue al automóvil?                 | No, se controla mediante el teclado.                                                              |
| ¿Qué genera mediante código?                | La esfera luminosa y la trayectoria del automóvil.                                                |
| ¿Qué importa desde Sketchfab?               | El automóvil y el escenario con sus recursos.                                                     |

# **20. Simulacro de sustentación**

Intenta responder estas preguntas sin mirar las respuestas. Después presiona cada pregunta para comprobar si respondiste correctamente.

## **Examen de práctica**

0/20 dominadas

1.¿Qué función cumple main()?

2.¿Qué diferencia hay entre GLAD y GLFW?

3.¿Qué diferencia hay entre VAO, VBO y EBO?

4.¿Qué función cumple uploadScene()?

5.¿Qué son las coordenadas UV?

6.¿Cuáles son las tres componentes de Phong?

7.¿Qué representa la normal de una superficie?

8.¿Por qué utiliza el producto punto en Phong?

9.¿Para qué sirve la atenuación?

10.¿Qué ocurre al aumentar el exponente especular?

11.¿Por qué utiliza Bézier compuesta?

12.¿Qué diferencia hay entre puntos de ruta y puntos de control?

13.¿Para qué sirve tangente()?

14.¿Por qué resta el vector origen?

15.¿Para qué sirve delta time?

16.¿Qué hace cambiarModelo()?

17.¿Cómo se determina la escala del automóvil?

18.¿Por qué la esfera utiliza otro shader?

19.¿Por qué dibuja las transparencias al final?

20.¿El automóvil mantiene una velocidad constante?Reiniciar simulacro

