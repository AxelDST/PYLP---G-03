# Ejercicio OpenMP — Procesamiento Paralelo de Transacciones de Ventas

**Materia:** Paradigmas y Lenguajes de Programación  
**Alumnos:**
- Dos Santos Axel
- Escalada Leandro
- Mittelstedt Gabriel
- Rodriguez Kevin

**Tecnología:** OpenMP (Open Multi-Processing) con C++

---

## ¿De qué trata?

El programa simula **10 millones de transacciones de ventas** con montos aleatorios y compara una versión secuencial contra una paralela con OpenMP. Calcula el total vendido, el promedio y la cantidad de ventas que superan un umbral, midiendo el tiempo de cada versión y el speedup obtenido.

---

## ¿Qué calcula el programa?

Sobre un conjunto de **10 millones de transacciones** con montos aleatorios entre \$1.000 y \$100.000:

| Métrica | Descripción |
|---|---|
| **Total vendido** | Suma de todos los montos |
| **Venta promedio** | Total dividido por la cantidad de ventas |
| **Ventas grandes** | Cantidad de ventas que superan los \$50.000 |
| **Tiempo de ejecución** | Medido con el reloj de alta resolución de OpenMP |
| **Speedup** | Cuántas veces más rápido es el paralelo respecto al serial |

---

## Conceptos de OpenMP aplicados

- **`parallel for`** — divide el bucle entre los hilos disponibles automáticamente.
- **`reduction`** — cada hilo acumula en una copia privada; OpenMP las suma al final, evitando condiciones de carrera sin usar mutex.
- **`default(none)`** — obliga a declarar explícitamente el alcance de cada variable.
- **`schedule(static)`** — reparte bloques iguales de iteraciones, ideal cuando cada una cuesta lo mismo.
- **`omp_get_wtime()`** — reloj de alta resolución para medir tiempos.

---

## Compilación y ejecución

```bash
g++ -std=c++17 -O2 -Wall -Wextra -fopenmp main.cpp -o ventas
./ventas
```

---

## Ejemplo de salida

```
Ventas: 10000000 | procesadores logicos: 8 | repeticiones: 5 (se descarta la primera)

RESULTADO SERIAL
  Total vendido        : $505123456789.00
  Venta promedio       : $50512.35
  Ventas mayores a $50000.00 : 4998231
  Tiempo               : 0.052341 s

RESULTADOS PARALELOS
Hilos  Sobresusc.   Tiempo (s)   Speedup   Validacion
----------------------------------------------------
1      no           0.054102     0.97x     OK
2      no           0.028451     1.84x     OK
4      no           0.015230     3.44x     OK
8      no           0.009871     5.30x     OK

CONCLUSION
  Mejor tiempo medido: 8 hilo(s), 0.009871 s, speedup 5.30x respecto al serial.
```

> Los valores exactos dependen del hardware. Usar más hilos no siempre garantiza mayor velocidad: el speedup real está limitado por la Ley de Amdahl, el ancho de banda de memoria y el overhead del propio runtime de OpenMP.
