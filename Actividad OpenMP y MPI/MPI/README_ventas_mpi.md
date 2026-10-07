# Ejercicio MPI — Ventas Distribuidas por Sucursal

**Materia:** Paradigmas y Lenguajes de Programación  
**Alumnos:**  - Dos Santos Axel
              - Escalada Leandro
              - Mittelstedt Gabriel
              - Rodriguez Kevin

**Tecnología:** MPI (Message Passing Interface) con Python y mpi4py

---

## ¿De qué trata?

Simula una cadena de supermercados con sucursales en distintas ciudades.  
Cada **proceso MPI** representa una sucursal que trabaja de forma **independiente y paralela**:
genera sus propias ventas diarias, calcula su total local, y luego envía el resultado
a la **Casa Central (proceso 0)** que consolida el reporte final.

> **Aplicación real:** Sistemas ERP, consolidación de datos financieros distribuidos,
> reportes en tiempo real de cadenas de retail.

---

## Conceptos MPI aplicados

| Concepto | Descripción |
|---|---|
| `MPI.COMM_WORLD` | Canal de comunicación global entre todos los procesos |
| `comm.Get_rank()` | Número de identificación único de cada proceso (sucursal) |
| `comm.Get_size()` | Cantidad total de procesos activos |
| `comm.gather()` | Operación **colectiva**: recolecta datos de todos los procesos en el proceso 0 |

---

## Flujo de ejecución

```
Proceso 0 (Buenos Aires) ──┐
Proceso 1 (Rosario)      ──┤  Cada uno calcula su total LOCAL en paralelo
Proceso 2 (Córdoba)      ──┤
Proceso 3 (Mendoza)      ──┘
              │
              │  comm.gather()  ← todos envían su total al proceso 0
              ▼
       Proceso 0 consolida y muestra el reporte final
```

### ¿Cómo se sabe que corre en paralelo?

Al ejecutar, las 4 sucursales imprimen sus totales en **orden aleatorio** (no siempre 0→1→2→3).
Eso demuestra que los procesos corren al mismo tiempo y el que termina primero imprime primero.

---

## Requisitos

- Python 3
- mpi4py y OpenMPI instalados:

```bash
sudo apt install python3-mpi4py -y
```

---

## Cómo ejecutar

```bash
# Ejecutar con 4 sucursales (recomendado)
mpiexec -n 4 --use-hwthread-cpus python3 ventas_sucursales_mpi.py
```

### Variantes

```bash
# Con 2 sucursales (solo Buenos Aires y Rosario)
mpiexec -n 2 --use-hwthread-cpus python3 ventas_sucursales_mpi.py

# Con 1 proceso (sin paralelismo, solo Buenos Aires)
mpiexec -n 1 python3 ventas_sucursales_mpi.py
```

> **Nota sobre `--use-hwthread-cpus`:** En WSL, Open MPI detecta solo los núcleos físicos
> por defecto. Esta flag le indica que también use los threads lógicos (Hyper-Threading),
> habilitando el uso completo de los 6 CPUs disponibles.

---

## Salida esperada (con -n 4)

```
[Sucursal Buenos Aires ] Primeras 3 ventas: [30247, 32562, 7653] ... | TOTAL LOCAL: $288,894
[Sucursal Rosario      ] Primeras 3 ventas: [46905, 12296, 6639] ... | TOTAL LOCAL: $245,491
[Sucursal Córdoba      ] Primeras 3 ventas: [23585, 7446, 37055] ... | TOTAL LOCAL: $277,519
[Sucursal Mendoza      ] Primeras 3 ventas: [42524, 8946, 42883] ... | TOTAL LOCAL: $330,177

=======================================================
         REPORTE CONSOLIDADO - EMPRESA
=======================================================
  Buenos Aires   :   $288,894  (25.3% del total)
  Rosario        :   $245,491  (21.5% del total)
  Córdoba        :   $277,519  (24.3% del total)
  Mendoza        :   $330,177  (28.9% del total)
-------------------------------------------------------
  TOTAL EMPRESA  : $1,142,081
  PROMEDIO       :   $285,520.25
  MEJOR SUCURSAL : Mendoza ($330,177)
  PEOR SUCURSAL  : Rosario ($245,491)
=======================================================
  Procesos MPI utilizados: 4 (1 por sucursal)
  Periodo analizado      : 10 dias
=======================================================
```

> El orden de las primeras 4 líneas puede variar en cada ejecución — eso es el paralelismo en acción.
