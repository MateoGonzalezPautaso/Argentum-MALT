# Manual de Proyecto

## Integrantes y roles

| Nombre | Rol principal | Contribuciones |
|--------|--------------|----------------|
| Mateo Gonzalez Pautaso | Servidor, Protocolo, Game Design | Arquitectura multithread (GameLoop, ClientHandler, Acceptor), Protocolo binario completo, GameFormulas como clase unificada (Vida, Maná, Daño, XP, Oro, defensa), Persistencia de jugadores y banco, Cheats, Tests, ADR Socket data race, EntityEventFactory, descarga de niveles del servidor por red, MapLevelDataBuilder y MapDataService en el servidor, refactors generales del servidor, bug fixes (mobs en zona segura, fantasmas, XP duplicada), documentación técnica (documentación técnica, protocolo, manual de usuario, ADRs). |
| Tiago Quemero | NPCs, Combate PvE | Sistema de NPCs con IA autónoma (visión, movimiento, ataque), Spawn periódico configurable por mapa, Sistema de ítems/equipamiento, Penalizaciones de defensa por armadura, Golpes críticos y evasión, Skins y animaciones de NPCs por tipo con velocidad configurable, Nombres coloreados. |
| Agustin Bermudez | Cliente (UI/UX), Servidor, Refactors | Interfaz de usuario completa (login, HUD, inventario, equipamiento, stats, chat, mercader, banco, audio), Sistema de inventario con persistencia y pociones, Visualización de equipamiento con capas (skins, overlays), Refactors del lado servidor (extracción de 12 servicios, resolvers PvP/ataque, ChatCommandParser) y cliente (SpriteRenderer, EntitySpriteRegistry, EffectOverlaySystem, TextureCache, UI widgets), configuraciones TOML. |
| Lautaro France | Cliente (Render/Editor), Audio, Mapas | Renderizado SDL2 (sprites, animaciones, mapa, criaturas con frames irregulares), Editor de mapas en Qt (tiles, props, zonas de spawn, portales, zoom), Sistema de audio (música, SFX, modulación por distancia), Sistema de chat, Combate (cuerpo a cuerpo y hechizos), Mapas y mazmorras, GitHub Pages (sitio Jekyll, estilos), video promocional. |

## Organización semanal

| Semana | Planificado | Realizado | Desvíos |
|--------|------------|-----------|---------|
| **1** (11/05 – 17/05) | Setup del proyecto, CMake, protocolo mínimo, render básico | Setup CMake + FetchContent, clases Socket/Thread/Queue de cátedra, esqueleto servidor-cliente, movimiento básico con render, assets iniciales. | La integración cliente-servidor para movimiento tomó más tiempo del esperado. |
| **2** (18/05 – 24/05) | Server multithread, login, editor, HUD básico | GameLoop + Acceptor + ClientHandler + ClientListMonitor, Sender/Receiver en servidor y cliente, login con persistencia binaria, editor Qt con tiles/props/save-load, barras de HP/MP/XP. | Sin desvíos significativos. |
| **3** (25/05 – 31/05) | Chat, muerte, clanes, fórmulas, mazmorras | Chat completo con comandos, sistema de muerte/fantasma/resurrección, cheats (vida infinita, maná infinito, nivel, oro), sistema de clanes completo (fundar, invitar, banear, bonus por cercanía, anti-friendly-fire), skins por raza/clase, fórmulas de VidaMax/ManaMax/regeneración, pantalla de crear personaje, editor de mazmorras con portales. | El sistema de clanes requirió cambios en el protocolo y persistencia que no estaban en el plan original. |
| **4** (01/06 – 07/06) | Inventario, equipamiento, hechizos, audio, oro | Inventario completo con persistencia y equipamiento por slots, visualización de armas/armaduras/cascos/escudos en el personaje, sistema de hechizos con 4 animaciones (flecha mágica, misil, explosión, curar), música y SFX con modulación por distancia, arco, sistema de oro con tope seguro y pérdida al morir, NPC comerciante/ sacerdote/banquero, zonas de spawn en editor. | La integración del inventario con el servidor y la persistencia fue más compleja de lo estimado. |
| **5** (08/06 – 14/06) | NPCs con IA, mazmorras, stats UI, comercio | IA de NPCs autónoma (visión 250px, persecución, ataque cuerpo a cuerpo), spawn periódico configurable por mapa con límites poblacionales, drops diferenciados por zona (normal vs mazmorra), editor de portales con zoom, UI de stats (fuerza, agilidad, daño, defensa, crítico, evasión), UI de mercader. | El merge de NPC-AI con el inventario existente generó conflictos de compilación que retrasaron el avance. |
| **6** (15/06 – 21/06) | Banco, drops en suelo, audio, refactors, documentación | Sistema de banco (depósito/retiro de ítems y oro con persistencia y sucursal universal), drops en suelo con animación flotante, comando /tomar y /tirar con soporte para múltiples ítems apilados, botón de audio en login y juego, animaciones de NPC por tipo, nombres coloreados (rojo NPCs, verde clan), ADR y fix de data race en Socket::shutdown, refactors generales del lado servidor y cliente, GameFormals como clase unificada, manual de usuario y documentación. | Se detectó una data race con Helgrind que requirió un ADR y refactor de `Socket` (shutdown_from_other_thread + atomic). |
| **7** (22/06 – 28/06) | Correcciones finales, refactors y entrega | Refactor masivo del servidor: extracción de 12 servicios (BankService, MerchantService, SpawnService, GroundItemService, MapTransitionService, MapDataService, PlayerSessionService, CheatService, MovementService, RegenService, ResurrectionService, SpellService) reduciendo Game de ~1500 líneas; GameFormulas como clase unificada con todas las fórmulas y variables externalizadas a TOML; EntityEventFactory para reducir creación duplicada de eventos; ChatCommandParser para reemplazar cadena de if-else; resolvers separados para PvP y ataque. Refactor del cliente: downsizing de SpriteRenderer (~900 líneas menos), creación de EntitySpriteRegistry, EffectOverlaySystem, TextureCache; mejoras en UI (form_widgets, credential_form, merchant_renderer). Bug fixes: visibilidad de fantasmas, renderizado al resucitar desde mazmorra, NPC en bordes de zona segura. Tests: test_game_formulas (+349 líneas), test_protocol_socketpair (+582 líneas), test_player (+34 líneas). Linting: clang-format + cppcheck + cpplint aplicados a todo el codebase. Editor: zoom, mejoras en tile_palette, main_window y dialogs. Documentación final y video promocional. | La magnitud de los refactors (especialmente la extracción de servicios y el downsizing de SpriteRenderer) requirió merges cuidadosos para no romper features existentes. |

## Herramientas utilizadas

- **IDEs:** VS Code
- **Linters / analizadores estáticos:** clang-format, cppcheck, valgrind (memcheck + helgrind)
- **Testing:** GoogleTest
- **Compilación:** CMake + Ninja
- **Control de versiones:** Git + GitHub (pull requests con revisión cruzada)
- **Edición de imágenes:** PAINT.NET, GIMP
- **Documentación:** Markdown, Jekyll + GitHub Pages, PlantUML

## Documentación de referencia

- [Sockets en C++ (cátedra)](https://github.com/eldipa/sockets-en-cpp)
- [Hands-on Threads (cátedra)](https://github.com/eldipa/hands-on-threads)
- [SDL2 docs](https://wiki.libsdl.org/)
- [Qt6 docs](https://doc.qt.io/qt-6/)
- [toml++](https://marzer.github.io/tomlplusplus/)
- Assets: [ao-org/Recursos](https://github.com/ao-org/Recursos)

## Puntos más problemáticos

- **Data race en `Socket::shutdown`:** detectada por Helgrind al matar el servidor con clientes conectados. Se documentó en un ADR y se resolvió separando `shutdown_from_other_thread()` y haciendo `stream_status` atómico.
- **Integración inventario-servidor:** el sistema de inventario requirió cambios en el protocolo, persistencia, y lógica de juego que no estaban completamente especificados al inicio.
- **Merge de ramas concurrentes:** al trabajar en paralelo (NPC-AI, inventario, hechizos), los merges generaron conflictos de compilación que retrasaron el avance.
- **Sincronización de hilos:** coordinar el GameLoop, los threads de clientes (Receiver/Sender) y la persistencia periódica requirió atención constante para evitar condiciones de carrera.

## Estado actual y errores conocidos

- El juego es completamente funcional con todas las features requeridas.
- No hay errores conocidos al momento de la entrega.

## Retrospectiva

- **¿Qué cambiarían a nivel código?**
  - Arrancar con GameFormulas como clase separada desde el día 1 en vez de tener las fórmulas dispersas.
  - Usar `std::variant` + visita para los eventos de protocolo desde el principio en vez de switches grandes.

- **¿Qué cambiarían a nivel organizacional?**
  - Definir el protocolo binario completo antes de empezar a codificar features, para reducir los merges conflictivos.
  - Hacer integraciones más frecuentes (cada 2-3 días en vez de semanales).

- **¿Pudieron llegar con todo?**
  - Sí, se completaron todas las features requeridas: servidor multithread, cliente gráfico, editor de mapas, protocolo binario, persistencia, clanes, hechizos, audio, mazmorras, sistema de combate completo.

- **¿Algo que debería darse en Taller?**
  - Un ejemplo concreto de cómo estructurar un proyecto CMake con FetchContent y dependencias externas.

## Enlaces

- Repositorio: [GitHub](https://github.com/MateoGonzalezPautaso/Argentum-MALT)
- Video promocional: [YouTube](https://youtu.be/KZLKZuvK9tE)
- Manual de usuario: [manual-de-usuario](manual-de-usuario)
- Documentación técnica: [documentacion-tecnica](documentacion-tecnica)
