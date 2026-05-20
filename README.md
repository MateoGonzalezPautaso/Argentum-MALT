# Argentum Online

Recreación del juego multijugador **Argentum Online** [1] desarrollada como Trabajo Práctico Final para la materia **Taller de Programación (TA045)** - Facultad de Ingeniería, Universidad de Buenos Aires.

El proyecto consta de tres aplicaciones: un **servidor**, un **cliente gráfico** y un **editor gráfico** de mapas.

---

## Instalación

El script `install.sh` instala dependencias del sistema (SDL2, Qt6, CMake, etc.), compila el proyecto, corre los tests y copia los binarios y assets a los directorios estándar:

```bash
./install.sh
```

Opciones disponibles:

| Flag | Descripción |
|---|---|
| `--debug` | Compila en modo Debug (default: Release) |
| `--skip-deps` | Omite la instalación de dependencias del sistema |
| `--skip-build` | Omite la compilación |
| `--skip-tests` | Omite la ejecución de tests |
| `--skip-install` | Omite la copia de binarios y assets |

Los binarios quedan en `~/.local/bin`, los assets en `~/.local/share/argentum/` y la configuración en `~/.config/argentum/`.

---

## Compilación manual

```bash
make compile-debug
```

Para correr los tests:

```bash
make run-tests
```

Para limpiar los archivos de build:

```bash
make clean
```

---

## Arquitectura de threads

El servidor utiliza el siguiente modelo de concurrencia:

```
Server
├── Acceptor (Thread)          - acepta conexiones entrantes
├── GameLoop (Thread)          - tick fijo, dueño del estado del juego
└── ClientListMonitor          - registro protegido por mutex
    └── por cliente:
        ├── Sender  (Thread)   - drena Queue<ServerEvent>  → socket
        └── Receiver (Thread)  - socket → Queue<PlayerCommand>
```

`Receiver` y `Sender` comparten el `Socket` ya que el primero solo lee y el segundo solo escribe; el dueño es `ClientHandler`. `Acceptor` es dueño de su propio socket listener.

### Queue

Las `Queue<T>` usadas son MPMC bloqueantes. Al construirse sin `max_size` se inicializan con `UINT_MAX - 1`, comportándose como unbounded en la práctica.

- **`Queue<PlayerCommand>`** (entrada al GameLoop): usa `try_pop()` - retorna inmediatamente si no hay comandos para no bloquear el tick.
- **`Queue<ServerEvent>`** (salida por cliente): usa `pop()` bloqueante - `Sender` solo existe para esperar eventos y enviarlos. Una queue por cliente evita que un cliente lento bloquee a los demás.

### GameLoop y tasa constante

El `GameLoop` corre a tasa fija (configurable vía `tick_rate_hz` en `config/server.toml`, default 20 Hz). El algoritmo de temporización sigue el enfoque del artículo de Book of Gehn [3]: cuando un tick se atrasa, se saltan los ticks completos perdidos avanzando `next_tick` un múltiplo exacto de `tick_duration`, preservando la alineación con el inicio original en lugar de resetear al tiempo actual.

```cpp
if (next_tick < now) {
    auto behind = now - next_tick;
    next_tick += (behind / tick_duration) * tick_duration;
}
std::this_thread::sleep_until(next_tick);
```

### ClientListMonitor

El uso de `player_id` como clave única garantiza que clientes con el mismo nombre no se pisen entre sí. El mutex protege el acceso concurrente al map, permitiendo que el `Acceptor` agregue clientes mientras el `GameLoop` realiza broadcasts.

El `GameLoop` se detiene automáticamente cuando el último jugador se desconecta (requiere que al menos uno haya conectado antes).

---

## Tests

Los tests del protocolo de comunicación usan GoogleTest:

```bash
make run-tests
```

O directamente con CTest desde el directorio de build:

```bash
ctest --test-dir build --output-on-failure
```

---

## Créditos

### Sockets

La implementación de las clases `Socket`, `Resolver`, `LibError` y `ResolverError` está basada en el código provisto por la cátedra:

- https://github.com/eldipa/sockets-en-cpp
- Licencia: GPL v2

### Threads y Queue

La implementación de las clases `Thread` y `Queue` está basada en el código provisto por la cátedra:

- https://github.com/eldipa/hands-on-threads
- Licencia: GPL v2

### Algoritmo de tasa constante

El algoritmo de temporización del GameLoop está basado en el artículo:

- *Constant Rate Loop* - Book of Gehn (2019): https://book-of-gehn.github.io/articles/2019/10/23/Constant-Rate-Loop.html

---

## Referencias

- [1] Argentum Online: https://www.argentumonline.com.ar/
- [2] Assets (imágenes): https://github.com/ao-org/Recursos
- [3] Constant Rate Loop: https://book-of-gehn.github.io/articles/2019/10/23/Constant-Rate-Loop.html
