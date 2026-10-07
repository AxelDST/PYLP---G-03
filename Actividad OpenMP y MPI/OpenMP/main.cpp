// =============================================================================
//  Procesamiento Paralelo de Transacciones de Ventas con OpenMP
//
//  Genera 10 millones de transacciones, calcula el total, el promedio y cuántas
//  superan un umbral. Compara la versión secuencial con la paralela midiendo
//  tiempos y speedup para 1, 2, 4 y 8 hilos.
//
// =============================================================================

#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <omp.h>     // Expone omp_get_wtime(), omp_get_num_procs(), etc.

// Parámetros globales del ejercicio
constexpr long long CANTIDAD_VENTAS = 10'000'000;  // Número de transacciones a simular
constexpr double    LIMITE          = 50'000.0;    // "venta grande": monto mayor a este valor
constexpr int       REPETICIONES    = 5;           // La primera se descarta (calentamiento)
constexpr double    TOLERANCIA_REL  = 1e-9;        // Tolerancia para comparar totales en punto flotante

// Estructuras de datos

// Almacena los dos valores que calculamos sobre el conjunto de ventas.
struct Resultado {
    double    total;         // Suma de todos los montos
    long long ventasGrandes; // Cantidad de ventas que superan LIMITE
};

// Resultado de ejecutar una función varias veces y promediar los tiempos.
struct Medicion {
    double    tiempo;    // Tiempo promedio (en segundos) de las repeticiones válidas
    Resultado resultado; // Último resultado calculado (todos deben ser iguales)
};

// Representa una fila de la tabla de resultados paralelos.
struct Fila {
    int    hilos;           // Cantidad de hilos usados
    bool   sobresuscripcion; // true si hilos > procesadores físicos disponibles
    double tiempo;           // Tiempo promedio medido
    double speedup;          // Cuántas veces más rápido que la versión serial
    bool   ok;               // true si el resultado coincide con el serial
};

// Genera el vector de ventas fuera de la medición (semilla fija = datos reproducibles).
std::vector<double> generarVentas(long long cantidad)
{
    std::mt19937 rng(42); // Mersenne Twister: mejor distribución y reproducibilidad que rand()
    std::uniform_real_distribution<double> monto(1'000.0, 100'000.0);

    std::vector<double> ventas(static_cast<size_t>(cantidad));
    for (double& v : ventas) v = monto(rng); // Llena el vector con montos aleatorios
    return ventas;
}

// -----------------------------------------------------------------------------
//  procesarSerial()
//  Versión de referencia: un solo hilo recorre todas las ventas de forma
//  secuencial. Es el baseline contra el que medimos el speedup paralelo.
// -----------------------------------------------------------------------------
Resultado procesarSerial(const std::vector<double>& ventas, double limite)
{
    Resultado r{0.0, 0};
    for (double v : ventas) {
        r.total += v;
        if (v > limite) ++r.ventasGrandes;
    }
    return r;
}

// Versión paralela. Cada iteración es independiente, por lo que el bucle
// puede dividirse entre hilos sin condiciones de carrera.
Resultado procesarParalelo(const std::vector<double>& ventas, double limite, int hilos)
{
    double    total   = 0.0;
    long long grandes = 0;

    // parallel for  → divide el rango del bucle entre los hilos.
    // num_threads   → fija la cantidad de hilos solo para este bloque.
    // default(none) → obliga a declarar el alcance de cada variable explícitamente.
    // shared        → ventas y limite son de solo lectura: compartirlas es seguro.
    // reduction     → cada hilo acumula en una copia privada; OpenMP las suma al final.
    //                 Evita la condición de carrera sin necesidad de mutex.
    // schedule(static) → bloques iguales asignados antes de empezar; ideal cuando
    //                    todas las iteraciones tienen el mismo costo.
    #pragma omp parallel for num_threads(hilos) default(none) shared(ventas, limite) \
            reduction(+ : total, grandes) schedule(static)
    for (long long i = 0; i < static_cast<long long>(ventas.size()); ++i) {
        total += ventas[i];
        if (ventas[i] > limite) ++grandes;
    }
    return {total, grandes};
}

// Ejecuta la función REPETICIONES varias veces, descarta la primera (caché fría y
// creación inicial de hilos) y devuelve el tiempo promedio de las demás.
template <typename Funcion>
Medicion medir(Funcion funcion)
{
    double    suma  = 0.0;
    Resultado ultimo{0.0, 0};

    for (int rep = 0; rep < REPETICIONES; ++rep) {
        double inicio = omp_get_wtime(); // Reloj de alta resolución de OpenMP
        ultimo        = funcion();
        double fin    = omp_get_wtime();

        if (rep > 0) suma += fin - inicio; // La primera corrida (rep == 0) se descarta
    }
    // Promedio sobre las (REPETICIONES - 1) corridas válidas
    return {suma / (REPETICIONES - 1), ultimo};
}

// Verifica que el resultado paralelo sea equivalente al serial.
// El total flotante admite una diferencia relativa < 1e-9 (el orden de suma
// entre hilos varía, lo que altera el redondeo de IEEE 754).
// El contador de ventas grandes debe ser idéntico: es un entero, sin redondeo.
bool validar(const Resultado& ref, const Resultado& cand, int hilos)
{
    bool   ok  = true;
    double rel = std::abs(ref.total - cand.total) / ref.total; // Error relativo

    if (ref.ventasGrandes != cand.ventasGrandes) {
        std::cout << "ERROR (" << hilos << " hilos): ventas grandes "
                  << cand.ventasGrandes << " en vez de " << ref.ventasGrandes << "\n";
        ok = false;
    }
    if (rel >= TOLERANCIA_REL) {
        std::cout << "ERROR (" << hilos << " hilos): total difiere (relativo " << rel << ")\n";
        ok = false;
    }
    return ok;
}

// Imprime la tabla de resultados y una conclusión basada en las mediciones reales.
void mostrarResultados(const Resultado& serial, double tSerial,
                       const std::vector<Fila>& filas, int procesadores)
{
    std::cout << std::fixed << std::setprecision(2);

    // -- Resultados del procesamiento serial ----------------------------------
    std::cout << "\nRESULTADO SERIAL\n";
    std::cout << "  Total vendido        : $" << serial.total << "\n";
    std::cout << "  Venta promedio       : $" << serial.total / CANTIDAD_VENTAS << "\n";
    std::cout << "  Ventas mayores a $"  << LIMITE << " : " << serial.ventasGrandes << "\n";
    std::cout << std::setprecision(6) << "  Tiempo               : " << tSerial << " s\n";

    // -- Tabla de resultados paralelos ----------------------------------------
    std::cout << "\nRESULTADOS PARALELOS\n";
    std::cout << std::left << std::setw(7)  << "Hilos"
                           << std::setw(13) << "Sobresusc."  // ¿Más hilos que CPUs?
                           << std::setw(13) << "Tiempo (s)"
                           << std::setw(10) << "Speedup"
                           << "Validacion\n"
              << std::string(52, '-') << "\n";

    const Fila* mejor     = nullptr; // Apunta a la fila con menor tiempo
    bool        hayExceso = false;   // Indica si alguna fila tiene sobresuscripción

    for (const Fila& f : filas) {
        std::cout << std::left << std::setw(7)  << f.hilos
                               << std::setw(13) << (f.sobresuscripcion ? "SI" : "no")
                               << std::setw(13) << std::setprecision(6) << f.tiempo
                               << std::setw(10) << std::setprecision(2) << f.speedup
                               << (f.ok ? "OK" : "ERROR") << "\n";

        if (f.sobresuscripcion) hayExceso = true;
        if (!mejor || f.tiempo < mejor->tiempo) mejor = &f; // Actualiza el mejor
    }

    // -- Conclusión empírica --------------------------------------------------
    std::cout << "\nCONCLUSION\n";
    std::cout << "  Mejor tiempo medido: " << mejor->hilos << " hilo(s), "
              << std::setprecision(6) << mejor->tiempo << " s, speedup "
              << std::setprecision(2) << mejor->speedup << "x respecto al serial.\n";
    std::cout << "  Equipo con " << procesadores << " procesador(es) logico(s).\n";

    // Aviso si se usaron más hilos que procesadores reales disponibles
    if (hayExceso)
        std::cout << "  AVISO: las filas con 'SI' usan mas hilos que procesadores; sus\n"
                     "         resultados no son concluyentes.\n";

    // Aclaración sobre la diferencia entre serial y OpenMP con 1 hilo
    std::cout << "  Con 1 hilo el programa OpenMP no es identico al serial (overhead y\n"
                 "  optimizaciones distintas del compilador), asi que su speedup puede\n"
                 "  desviarse de 1.0x. El rendimiento depende de la maquina.\n";
}

// -----------------------------------------------------------------------------
//  main()
//  Orquesta el ejercicio en cuatro pasos claros:
//    1. Generación de datos
//    2. Medición de la versión serial (baseline)
//    3. Medición de la versión paralela con distintas cantidades de hilos
//    4. Presentación de resultados y conclusión
// -----------------------------------------------------------------------------
int main()
{
    // Núcleos físicos × threads por núcleo (si hay hyperthreading)
    const int procesadores = omp_get_num_procs();

    std::cout << "Ventas: "          << CANTIDAD_VENTAS
              << " | procesadores logicos: " << procesadores
              << " | repeticiones: " << REPETICIONES << " (se descarta la primera)\n";

    // Paso 1: generar datos FUERA de la medición para no contaminar los tiempos
    const std::vector<double> ventas = generarVentas(CANTIDAD_VENTAS);

    // Paso 2: medir la versión serial → es el baseline del speedup
    const Medicion serial = medir([&] { return procesarSerial(ventas, LIMITE); });

    // Paso 3: medir la versión paralela con 1, 2, 4 y 8 hilos
    // Se prueban TODOS los casos; si hilos > procesadores se marca como
    // sobresuscripción pero NO se omite: ver qué pasa en ese escenario es útil.
    std::vector<Fila> filas;
    for (int hilos : {1, 2, 4, 8}) {
        Medicion m = medir([&] { return procesarParalelo(ventas, LIMITE, hilos); });

        // speedup = tiempo_serial / tiempo_paralelo
        // Si speedup > 1: el paralelo ganó; si < 1: el paralelo perdió.
        filas.push_back({
            hilos,
            hilos > procesadores,          // ¿Sobresuscripción?
            m.tiempo,
            serial.tiempo / m.tiempo,      // Speedup
            validar(serial.resultado, m.resultado, hilos)
        });
    }

    // Paso 4: mostrar tabla de resultados y conclusión empírica
    mostrarResultados(serial.resultado, serial.tiempo, filas, procesadores);
    return 0;
}