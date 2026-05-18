# Taller de Programación I (75.42)
**Facultad de Ingeniería — Universidad de Buenos Aires**

# Argentum Online

---

## Índice

- [Introducción](#introducción)
- [Descripción](#descripción)
  - [Tierras de Argentum](#tierras-de-argentum)
  - [Puntos de vida](#puntos-de-vida)
  - [Puntos de maná](#puntos-de-maná)
  - [Oro](#oro)
  - [Inventario](#inventario)
  - [Puntos de experiencia](#puntos-de-experiencia)
  - [Ataque](#ataque)
  - [Defensa](#defensa)
  - [Razas y Clases](#razas-y-clases)
  - [Muerte](#muerte)
  - [Fair play](#fair-play)
  - [Ciudades y pueblos](#ciudades-y-pueblos)
  - [Cavernas y Mazmorras](#cavernas-y-mazmorras)
  - [Items](#items)
  - [Criaturas o NPC](#criaturas-o-npc)
  - [Clanes](#clanes)
- [Interfaz de usuario](#interfaz-de-usuario)
  - [Mini-chat](#mini-chat)
  - [Vestimenta](#vestimenta)
- [Persistencia](#persistencia)
- [Configuración](#configuración)
- [Sonidos](#sonidos)
- [Música](#música)
- [Animaciones](#animaciones)
- [Cheats](#cheats)
- [Entregables](#entregables)
- [Aplicaciones Requeridas](#aplicaciones-requeridas)
- [Unit tests](#unit-tests)
- [Restricciones](#restricciones)
- [Referencias](#referencias)

---

## Introducción

El presente trabajo consistirá en recrear el mítico y entrañable juego **Argentum Online** [1], un juego multijugador en donde los participantes controlan a un personaje de rol en un mundo fantástico de magia y lleno de criaturas salvajes.

---

## Descripción

### Tierras de Argentum

Argentum es un lugar extenso, rico y variado: desde zonas boscosas hasta desiertos. El tipo de terreno no afecta la jugabilidad (da lo mismo caminar sobre un camino que sobre arena).

Esparcidas sobre Argentum hay edificaciones y construcciones (fuera de ciudades y pueblos) que obstaculizan el paso. Los personajes **no pueden atravesar paredes**, **ni atravesarse entre sí ni con los NPC**: las personas colisionan, incluso los fantasmas.

---

### Puntos de vida

Son los puntos que determinan la salud del personaje. Si llegan a 0, el personaje muere y se transforma en fantasma.

```
VidaMax = Constitución * FClaseVida * FRazaVida * Nivel
```

Los puntos de vida pueden recuperarse tomando pociones curativas, con hechizos de curación, o con el paso del tiempo:

```
Vida = FRazaRecuperacion * segundos
```

---

### Puntos de maná

Todos los personajes salvo el **Guerrero** pueden realizar hechizos de magia. Cada hechizo requiere y consume una cantidad mínima de maná.

```
ManaMax = Inteligencia * FClaseMana * FRazaMana * Nivel
```

Los puntos de maná pueden recuperarse tomando pociones de maná, meditando, o con el paso del tiempo.

Un personaje puede meditar con el comando `/meditar`. En estado de meditación no puede realizar otra acción; cualquier acción lo sacará de ese estado.

**Recuperación por meditación:**
```
Mana = FClaseMeditacion * Inteligencia * segundos
```

**Recuperación por el paso del tiempo:**
```
Mana = FRazaRecuperacion * segundos
```

---

### Oro

La moneda de comercio en Argentum es el oro.

Al matar un NPC, el jugador obtiene:
```
Oro = rand(0, 0.2) * VidaMaxNPC
```

Al matar a otro jugador, el oro obtenido es exactamente el oro que el jugador muerto tenía "en exceso". Cada personaje tiene un máximo de oro seguro; puede tener hasta un 50% adicional, considerado "en exceso".

> **Ejemplo:** Si el máximo es 1000, el personaje puede tener hasta 1500. Si muere con 900, no pierde nada. Si muere con 1100, conserva 1000 y deja caer 100.

```
OroMax = 100 * Nivel^1.1
```

---

### Inventario

Cada jugador puede tener hasta un máximo de N objetos en su inventario (armas, armaduras, cascos, escudos, pociones, báculos, etc.).

Algunos objetos pueden ser **equipados**. Un jugador puede tener múltiples armas, armaduras, cascos, escudos y varas/báculos, pero solo podrá tener **equipado uno de cada tipo**. No se puede tener un arma y una vara/báculo equipados a la vez.

Las pociones, al equiparse, son consumidas inmediatamente.

- `/tomar` — Recoge un objeto del suelo al inventario (el jugador debe estar posicionado sobre él).
- `/tirar` — Deja caer al suelo el objeto seleccionado del inventario.

---

### Puntos de experiencia

```
Limite (para subir de nivel) = 1000 * Nivel^1.8
```

**Experiencia por ataque:**
```
Exp = Daño * max(NivelDelOtro - Nivel + 10, 0)
```

**Experiencia adicional por matar:**
```
Exp = rand(0, 0.1) * VidaMaxDelOtro * max(NivelDelOtro - Nivel + 10, 0)
```

---

### Ataque

Un personaje puede atacar a un NPC o a otro jugador restándole puntos de vida.

```
Daño = Fuerza * rand(DañoArmaMin, DañoArmaMax)
```

Existe cierta probabilidad de realizar un **ataque crítico** que causa el doble de daño y no puede ser esquivado.

- Si el arma es de rango (arco) o es un hechizo, el ataque puede realizarse **a distancia** (click sobre el objetivo).
- En otro caso, el ataque solo se realiza si el jugador está **al lado** del adversario.

---

### Defensa

Ante un ataque, primero existe la posibilidad de **esquivar**:

```
Esquivar si rand(0, 1) ^ Agilidad < 0.001
```

Si no se esquiva, el daño puede ser reducido según la defensa provista por armadura, escudo y/o casco:

```
Defensa = rand(ArmaduraMin, ArmaduraMax) + rand(EscudoMin, EscudoMax) + rand(CascoMin, CascoMax)
```

> **Ejemplo:** Si un jugador recibe 10 de daño pero tiene 8 de defensa, solo pierde 2 puntos de vida.

---

### Razas y Clases

Al crear el personaje se elige **raza** y **clase**, que afectan las ecuaciones del juego.

#### Razas

| Raza    | Características                                              |
|---------|--------------------------------------------------------------|
| Humano  | Raza equilibrada.                                            |
| Elfo    | Muy inteligente y ágil, pero de constitución física frágil.  |
| Enano   | Muy fuerte y resistente, pero poco ágil.                     |
| Gnomo   | Inteligente y resistente, pero menos ágil que los elfos.     |

#### Clases

| Clase    | Características                                                                                       |
|----------|-------------------------------------------------------------------------------------------------------|
| Mago     | Mente muy desarrollada, cuerpo menos.                                                                 |
| Clérigo  | Menos inteligente y hábil que el mago, compensa con mayor desempeño físico.                           |
| Paladín  | Fuerte y resistente, entrenado para el combate, pero con menor inteligencia. Puede usar magia.        |
| Guerrero | El más fuerte y resistente. **No puede usar magia ni meditar. Su maná es siempre 0.**                |

---

### Muerte

Al morir, el jugador puede perder **oro** y/o **experiencia** (ver secciones correspondientes). Además, **todos los objetos de su inventario caen al piso**.

Tras la muerte, el personaje se convierte en **fantasma**. Un fantasma puede moverse pero no puede interactuar con nadie ni con nada.

Para resucitar, el jugador puede:
1. Dirigirse al sanador de la ciudad más próxima.
2. Ejecutar el comando `/resucitar`, lo que lo trasladará al sanador de la ciudad más próxima. Esta operación **no es inmediata**: el fantasma estará inmovilizado un tiempo proporcional a la distancia antes de trasladarse y resucitar.

---

### Fair play

- Un jugador con **nivel 12 o menor** es considerado **newbie**. Los newbies no pueden atacar ni ser atacados por otros jugadores.
- Un jugador no podrá atacar ni ser atacado por otro jugador si la **diferencia de niveles entre ellos es superior a 10**.

---

### Ciudades y pueblos

A lo largo de las tierras de Argentum hay ciudades y pueblos con al menos un **sacerdote**, un **comerciante** y un **banquero**.

Los jugadores interactúan con ellos seleccionándolos (click) y escribiendo comandos:

- **Sacerdote:** Puede resucitar (`/resucitar`), curar vida y maná (`/curar`) y vender báculos, varas y pociones (`/comprar <objeto>`).
- **Comerciante:** Compra y vende armas, armaduras, cascos y pociones (nunca hechizos). Comandos: `/comprar <objeto>` y `/vender <objeto>`.
- **Banquero:** Permite guardar y retirar pertenencias e oro.
  - `/depositar <objeto>` / `/retirar <objeto>`
  - `/depositar oro <cant>` / `/retirar oro <cant>`
  - El sistema bancario permite retiros desde cualquier sucursal en todo Argentum sin cargo.

> **Importante:** Las ciudades y pueblos son **zonas seguras**: los jugadores no pueden atacar ni ser atacados, y los NPC no pueden entrar.

---

### Cavernas y Mazmorras

Esparcidas por Argentum hay entradas a cavernas o mazmorras con NPC exclusivos, mucho más fuertes que el promedio, que custodian grandes tesoros.

---

### Items

| Item                | Tipo            | Detalle                                                              |
|---------------------|-----------------|----------------------------------------------------------------------|
| Espada              | Arma cuerpo a cuerpo | Daño: 2–5                                                      |
| Hacha               | Arma cuerpo a cuerpo | Daño: 4–5                                                      |
| Martillo            | Arma cuerpo a cuerpo | Daño: 1–9                                                      |
| Vara de fresno      | Hechizo (distancia)  | Hechizo "flecha mágica", daño: 2–4. Consume 5 de maná.         |
| Flauta élfica       | Hechizo (curación)   | Hechizo "curar" sobre el jugador, restaura vida. Consume 100 de maná. |
| Báculo nudoso       | Hechizo (distancia)  | Hechizo "misil", daño: 4–8. Consume 15 de maná.                |
| Báculo engarzado    | Hechizo (distancia)  | Hechizo "explosión", daño: 8–20. Consume 30 de maná.           |
| Arco simple         | Arma a distancia     | Daño: 1–4. Flechas infinitas.                                   |
| Arco compuesto      | Arma a distancia     | Daño: 4–16. Flechas infinitas.                                  |
| Armadura de cuero   | Armadura        | Defensa: 2–6                                                         |
| Armadura de placas  | Armadura        | Defensa: 15–30                                                       |
| Túnica azul         | Armadura        | Defensa: 6–10                                                        |
| Capucha             | Casco           | Defensa: 1–4                                                         |
| Casco de hierro     | Casco           | Defensa: 4–8                                                         |
| Escudo de tortuga   | Escudo          | Defensa: 1–2                                                         |
| Escudo de hierro    | Escudo          | Defensa: 1–4                                                         |
| Sombrero mágico     | Casco           | Defensa: 4–12                                                        |

---

### Criaturas o NPC

Las criaturas atacan al jugador más cercano si este se encuentra lo suficientemente cerca, incluso si el jugador no inicia el ataque. Todas tienen **ataque cuerpo a cuerpo**.

Periódicamente se crean nuevas criaturas hasta un límite poblacional, garantizando que siempre haya criaturas en Argentum sin saturar el mundo.

**Drop al morir:**

| Probabilidad | Item dejado |
|---|---|
| 0.80 | Nada |
| 0.08 | Oro: `rand(0.01, 0.2) * VidaMaxNPC` |
| 0.01 | Poción de vida o maná (al azar) |
| 0.01 | Cualquier otro objeto al azar |

**Criaturas existentes:**

- Goblin (varios tipos)
- Esqueleto (varios tipos)
- Zombie
- Araña (varios tipos)
- Orco
- Golem (varios tipos)

Algunos NPC aparecen en cualquier zona; otros son específicos de ciertas zonas o mazmorras.

---

### Clanes

Un jugador con **nivel 6 o más** puede fundar un clan (nombre único). Una vez fundado, no puede irse ni fundar otro.

**Comandos:**

| Comando | Descripción |
|---|---|
| `/fundar-clan <nombre>` | Funda un clan (nivel ≥ 6) |
| `/unirse <nombre>` | Pide unirse a un clan |
| `/revisar-clan` | Ver miembros y pedidos pendientes (solo fundador) |
| `/clan-aceptar <nick>` | Acepta un pedido de ingreso (solo fundador) |
| `/clan-rechazar <nick>` | Rechaza un pedido de ingreso (solo fundador) |
| `/clan-ban <nick>` | Rechaza y banea al usuario (solo fundador) |
| `/clan-kick <nick>` | Echa a un miembro del clan sin banearlo (solo fundador) |
| `/dejar-clan` | El jugador deja el clan (el fundador no puede) |

**Reglas:**
- Miembros del mismo clan **no pueden atacarse entre sí**.
- Cuando jugadores del clan están cerca, reciben **bonificación de defensa y ataque** proporcional al número de miembros presentes.
- Los miembros reciben notificaciones cuando un compañero entra/sale del juego o está siendo atacado.
- **Límite:** 16 jugadores (incluido el fundador).

---

## Interfaz de usuario

El juego muestra al personaje desde una **vista de águila** con el personaje en el centro. El jugador se mueve con el teclado y ataca haciendo click sobre el objetivo.

La interfaz debe mostrar: inventario, objetos equipados, oro, vida, maná, experiencia y nivel.

El cliente debe poder ejecutarse en **fullscreen** y en **modo ventana** (tamaño arbitrario, configurable vía archivo o parámetro de línea de comandos).

---

### Mini-chat

La interfaz incluye un mini-chat con los últimos mensajes relevantes y un campo de texto para escribir.

**Mensajes relevantes:**
- Daño provocado
- Daño recibido
- Evasión (si el jugador o el contrincante esquivó un ataque)
- Mensajes privados de otros jugadores

**Comandos disponibles:**

| Comando | Descripción |
|---|---|
| `/meditar` | Entra en estado de meditación (recupera maná) |
| `/resucitar` | Si es fantasma, se traslada al sanador más cercano y resucita |
| `/curar` | Recupera vida y maná (requiere sacerdote) |
| `/depositar <objeto>` | Deposita objeto en el banco (requiere banquero) |
| `/retirar <objeto>` | Retira objeto del banco (requiere banquero) |
| `/listar` | Lista objetos del comerciante o banquero |
| `/comprar <objeto>` | Compra un objeto (sacerdote o comerciante) |
| `/vender <objeto>` | Vende un objeto (comerciante) |
| `/tomar` | Recoge un objeto del suelo |
| `/tirar` | Tira el objeto seleccionado del inventario |
| `@<nick> <msj>` | Envía un mensaje privado a otro jugador |
| `/fundar-clan <nombre>` | Funda un clan |
| `/unirse <nombre>` | Pide unirse a un clan |
| `/revisar-clan` | Revisa pedidos y miembros (solo fundador) |
| `/clan-aceptar <nick>` | Acepta un pedido (solo fundador) |
| `/clan-rechazar <nick>` | Rechaza un pedido (solo fundador) |
| `/clan-ban <nick>` | Banea a un usuario (solo fundador) |
| `/dejar-clan` | El jugador deja el clan |
| `/clan-kick <nick>` | Echa a un miembro sin banearlo (solo fundador) |

---

### Vestimenta

Todo personaje tendrá una vestimenta que cambia visualmente según la armadura, casco, arma y/o báculo equipado. Si no tiene armadura equipada, se muestra una vestimenta común.

---

## Persistencia

Los avances de los jugadores deben ser **persistidos** para que puedan continuar desde donde estaban. La persistencia debe realizarse **periódicamente** para cubrir desconexiones abruptas.

Se recomienda usar **2 archivos**:
1. **Archivo de datos:** structs de tamaño fijo y constante con los datos de cada jugador.
2. **Archivo índice:** diccionario (map) que mapea el nombre del jugador con su offset en el archivo de datos.

> No se puede cargar el archivo completo en memoria (se asume que habrá miles de jugadores, pero solo cientos conectados a la vez). El archivo índice sí puede mantenerse en memoria.

Los archivos deben ser **binarios**. El borrado de usuarios no es requerido.

---

## Configuración

- Todas las ecuaciones deben estar en **métodos o funciones propios** y concentradas en **una única clase o módulo**.
- Todos los valores numéricos deben provenir de un **archivo de configuración en formato TOML**.
- No se debe implementar un parser TOML propio; se debe utilizar uno existente.

---

## Sonidos

Se deben reproducir sonidos para darle realismo al juego:
- Sonido al producirse un disparo/ataque.
- Sonido al producirse una muerte.

Si la cantidad de eventos es muy grande, algunos sonidos deben omitirse para no saturar al jugador.

El **volumen** de los sonidos debe estar modulado por la distancia: eventos cercanos suenan más fuerte.

---

## Música

Se debe reproducir una **música de fondo**.

---

## Animaciones

- Las unidades se muestran con **animaciones de movimiento**, no imágenes estáticas.
- Las explosiones, disparos y todo elemento dinámico deben ser **animados**.

---

## Cheats

El juego debe aceptar combinaciones de teclas para activar:
- Vida infinita
- Maná infinito
- Morir automáticamente
- (otros)

> Se recomienda implementar esto lo antes posible para facilitar las pruebas (ej: para testear `/resucitar`, usar el cheat de morir en lugar de esperar que un NPC mate al personaje).

---

## Entregables

- **Documentación:**
  - Manual del usuario
  - Documentación técnica
  - Manual de proyecto
- **Instalador** (makefile, cmake o script de shell) que debe:
  - Descargar e instalar dependencias (ej: SDL)
  - Compilar el proyecto
  - Correr los tests unitarios
  - Copiar binarios a `~/.local/bin`, assets a `~/.local/share/NAME/` y configuración a `~/.config/NAME/`
- **Repositorio en GitHub** con código, instalador, documentación y todo el material necesario.
- **Video promocional** mostrando todos los features implementados (incluyendo el editor), orientado a una audiencia potencialmente compradora.

---

## Aplicaciones Requeridas

El proyecto consta de **3 aplicaciones**:

1. **Cliente gráfico**
2. **Servidor**
3. **Editor gráfico**

> El editor no necesita conectarse al servidor; los niveles editados pueden moverse manualmente. El cliente sí debe descargar los niveles del servidor.

El proyecto debe compilar y ejecutar en **Ubuntu 24.04** (o Xubuntu 24.04), el mismo entorno del Sercom durante la cursada.

---

## Unit tests

El **protocolo de comunicación** debe estar testeado con **GoogleTest** u otro framework de testing para C++. Los tests pueden mockear los sockets o no.

---

## Restricciones

1. El sistema debe realizarse en **C++ (C++20)** con el estándar **POSIX 2008**, usando librerías **SDL** y **Qt**. Se permite el uso de QtCreator.
2. El **protocolo de comunicación** debe estar en formato **binario**.
3. El **estado del juego/escenario** debe estar en formato **binario**.
4. Los **archivos de configuración** deben estar en formato **TOML**.
5. Todo socket utilizado debe ser **bloqueante** (comportamiento por defecto).

---

## Referencias

- [1] Argentum Online: https://www.argentumonline.com.ar/
- [2] Assets (imágenes): https://github.com/ao-org/Recursos
