# Aston Martin One-77 – OpenGL


## Bibliotecas utilizadas

- **OpenGL:** renderizado de gráficos tridimensionales.
- **GLFW:** creación de ventanas y control del teclado y ratón.
- **GLAD:** acceso a las funciones de OpenGL.
- **GLM:** operaciones matemáticas con vectores y matrices.
- **TinyGLTF:** importación de modelos en formato glTF.
- **stb_image:** carga de imágenes y texturas.

## Importación de modelos y texturas

Mediante TinyGLTF se recuperan los vértices, normales, coordenadas UV, índices y materiales de ambos modelos.

La información geométrica se almacena mediante VAO, VBO y EBO, mientras que las texturas se cargan utilizando stb_image.

La función `cambiarModelo()` permite reutilizar las mismas funciones de carga y renderizado para el automóvil y el escenario, evitando duplicar código.

## Shaders e iluminación Phong

El proyecto utiliza dos shaders principales:

- **Vertex Shader:** transforma los vértices mediante las matrices de modelo, vista y proyección.
- **Fragment Shader:** calcula el color final de las superficies mediante el modelo de iluminación Phong.

La iluminación Phong está compuesta por tres componentes:

- **Ambiental:** proporciona una iluminación mínima al escenario.
- **Difusa:** depende de la orientación de la superficie respecto a la fuente luminosa.
- **Especular:** representa los reflejos brillantes dependiendo de la posición de la cámara.

También se implementa una atenuación por distancia:

`A(d) = 1 / (1 + 0.035d + 0.008d²)`

El ambiente nocturno permite apreciar mejor los efectos de la iluminación dinámica.

## Movimiento mediante curvas de Bézier

El desplazamiento del automóvil se realiza mediante una curva de Bézier cúbica compuesta por 11 segmentos conectados, construidos a partir de 12 puntos tridimensionales.

La fórmula utilizada es:

`B(t) = (1-t)³P₀ + 3(1-t)²tP₁ + 3(1-t)t²P₂ + t³P₃`

Los puntos de control internos se calculan utilizando los puntos vecinos de la trayectoria, permitiendo una transición suave entre segmentos.

La función `bezier()` calcula la posición del automóvil, mientras que la función `tangente()` determina la dirección aproximada del movimiento.

A partir del vector tangente se calcula la rotación horizontal y la inclinación vertical del automóvil, permitiendo que se adapte a las curvas y pendientes de la carretera.

El recorrido tiene una duración aproximada de 24 segundos y se repite automáticamente.

## Controles

| Tecla | Acción |
|---|---|
| W | Avanzar |
| S | Retroceder |
| A | Desplazarse a la izquierda |
| D | Desplazarse a la derecha |
| Ratón | Orientar la cámara |
| Shift | Aumentar velocidad de la cámara |
| Flechas | Desplazar la fuente luminosa |
| Q / E | Modificar la altura de la luz |
| ESC | Liberar el cursor |
| Clic | Capturar el cursor |

## Ciclo de renderizado

Durante cada fotograma, el programa realiza las siguientes operaciones:

1. Calcula el tiempo transcurrido entre fotogramas.
2. Procesa las entradas del usuario.
3. Actualiza la cámara y la fuente luminosa.
4. Renderiza el escenario.
5. Calcula la posición y orientación del automóvil mediante Bézier.
6. Renderiza el automóvil y los materiales transparentes.
7. Dibuja la esfera que representa la fuente luminosa.
8. Muestra el fotograma resultante.

## Resultados

El proyecto integra la importación de modelos tridimensionales, texturizado, iluminación dinámica, transformaciones matriciales y curvas paramétricas dentro de un escenario interactivo.

El movimiento del automóvil se calcula matemáticamente, sin utilizar motores de físicas ni sistemas de detección de colisiones.

## Créditos

- Modelo: 2011 Aston Martin One-77 — Sketchfab.
- Escenario: Snowy Mountain Road — Sketchfab.

Proyecto académico de programación gráfica desarrollado en C++ y OpenGL.
