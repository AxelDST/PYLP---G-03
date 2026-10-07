# =============================================================================
# EJERCICIO MPI - Ventas Distribuidas por Sucursal
# Materia : Paradigmas y Lenguajes de Programación
# Alumnos  : - Dos Santos Axel
#            - Escalada Leandro
 #           - Mittelstedt Gabriel
  #          - Rodriguez Kevin
# 
#
# DESCRIPCIÓN:
#   Simula una cadena de supermercados con sucursales en distintas ciudades.
#   Cada proceso MPI representa una sucursal que trabaja de forma INDEPENDIENTE:
#   genera sus propias ventas y calcula su total local.
#   Luego, todos los procesos envían sus resultados al proceso central (rank 0)
#   que consolida el reporte final de toda la empresa.
#
# EJECUCIÓN:
#   mpiexec -n 4 python ventas_sucursales_mpi.py
#   (el -n 4 indica que se lanzan 4 procesos = 4 sucursales)
# =============================================================================

from mpi4py import MPI
import random

# =============================================================================
# PASO 1 - Inicializar el entorno MPI
# =============================================================================

# MPI.COMM_WORLD es el "canal de comunicación" que conecta a todos los procesos.
# Es el punto de partida obligatorio en cualquier programa MPI.
comm = MPI.COMM_WORLD

# rank: número de identificación único de cada proceso (0, 1, 2, 3, ...)
# Es como el número de sucursal. El proceso 0 será la "Casa Central".
rank = comm.Get_rank()

# size: cantidad total de procesos que están corriendo en paralelo.
# Si ejecutamos con -n 4, size = 4 (4 sucursales en total).
size = comm.Get_size()

# =============================================================================
# PASO 2 - Definir los datos de cada sucursal
# =============================================================================

# Cada sucursal tiene un nombre. El índice coincide con el rank del proceso.
# Ejemplo: el proceso con rank=2 representa la sucursal "Córdoba".
nombres_sucursales = ["Buenos Aires", "Rosario", "Córdoba", "Mendoza"]

# Cantidad de días de ventas que registra cada sucursal
DIAS = 10

# =============================================================================
# PASO 3 - Cada proceso genera sus propias ventas (trabajo local en paralelo)
# =============================================================================

# Usamos rank como semilla del generador aleatorio para que cada sucursal
# tenga datos diferentes pero reproducibles (mismo resultado cada ejecución).
random.seed(rank * 42)

# Cada proceso genera una lista de ventas diarias (montos en pesos).
# Esto simula que cada sucursal tiene su propio sistema de caja registradora.
ventas_locales = [random.randint(5000, 50000) for _ in range(DIAS)]

# Cada proceso calcula su propio total — sin comunicarse con nadie aún.
# ¡Aquí está el paralelismo real! Los 4 procesos suman AL MISMO TIEMPO.
total_local = sum(ventas_locales)

# Cada sucursal imprime su información (los prints pueden salir en cualquier orden)
print(f"[Sucursal {nombres_sucursales[rank]:15s}] "
      f"Primeras 3 ventas: {ventas_locales[:3]} ... "
      f"| TOTAL LOCAL: ${total_local:,}")

# =============================================================================
# PASO 4 - Operación colectiva: GATHER
# Todos los procesos envían su total_local al proceso 0 (la Casa Central)
# =============================================================================

# comm.gather() es una operación COLECTIVA: todos los procesos participan
# al mismo tiempo. No es necesario hacer send/recv manual entre pares.
#
# ¿Qué hace gather?
#   - Cada proceso (incluido el 0) envía su "total_local"
#   - El proceso root=0 recibe una lista con todos los totales ordenados por rank
#   - Los demás procesos reciben None (no son la central)
#
#  Proceso 0 -> $25.000  --.
#  Proceso 1 -> $18.500  --| gather() -> Proceso 0 recibe: [25000, 18500, 31200, 14800]
#  Proceso 2 -> $31.200  --|
#  Proceso 3 -> $14.800  --'
todos_los_totales = comm.gather(total_local, root=0)

# =============================================================================
# PASO 5 - Solo el proceso 0 (Casa Central) genera el reporte final
# =============================================================================

# La condición "if rank == 0" asegura que solo la Casa Central ejecute este bloque.
# Los otros procesos (sucursales) ya terminaron su trabajo con el gather().
if rank == 0:

    # Calcular estadísticas globales de toda la cadena
    total_empresa  = sum(todos_los_totales)
    promedio       = total_empresa / size
    indice_mejor   = todos_los_totales.index(max(todos_los_totales))
    indice_peor    = todos_los_totales.index(min(todos_los_totales))
    mejor_sucursal = nombres_sucursales[indice_mejor]
    peor_sucursal  = nombres_sucursales[indice_peor]

    # Imprimir el reporte consolidado
    print()
    print("=" * 55)
    print("         REPORTE CONSOLIDADO - EMPRESA")
    print("=" * 55)

    # Mostrar el detalle de cada sucursal
    for i in range(size):
        porcentaje = (todos_los_totales[i] / total_empresa) * 100
        print(f"  {nombres_sucursales[i]:15s}: ${todos_los_totales[i]:>10,}  "
              f"({porcentaje:.1f}% del total)")

    print("-" * 55)
    print(f"  {'TOTAL EMPRESA':15s}: ${total_empresa:>10,}")
    print(f"  {'PROMEDIO':15s}: ${promedio:>10,.2f}")
    print(f"  {'MEJOR SUCURSAL':15s}: {mejor_sucursal} (${max(todos_los_totales):,})")
    print(f"  {'PEOR SUCURSAL':15s}: {peor_sucursal} (${min(todos_los_totales):,})")
    print("=" * 55)
    print(f"\n  Procesos MPI utilizados: {size} (1 por sucursal)")
    print(f"  Periodo analizado      : {DIAS} dias")
    print("=" * 55)
