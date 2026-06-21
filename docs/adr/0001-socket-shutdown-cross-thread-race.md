# ADR 0001: Eliminar la data race entre `Socket::shutdown` y `recvsome`/`sendsome`

## Status

Aceptado e implementado.

## Contexto

Corriendo `valgrind --tool=helgrind` sobre el server (escenario: matar el server con
`SIGINT` mientras un cliente sigue conectado) apareció una data race real:

```
Possible data race during write of size 4 at 0x560FE30 by thread #3 (GameLoop)
  at Socket::shutdown(int)
  by ClientHandler::stop()
  by ClientListMonitor::clean_dead()
  Locks held: 1 (mutex de ClientListMonitor)

This conflicts with a previous write of size 4 by thread #5 (Receiver)
  at Socket::recvall(void*, unsigned int)
  Locks held: none
```

### Por qué pasa

`Socket` guarda el estado de la conexión en un campo `stream_status` (flags
`SEND_CLOSED`/`RECV_CLOSED`), actualizado con `stream_status |= ...` en tres lugares:

| Quién escribe | Cuándo |
|---|---|
| `Socket::recvsome()` | `recv()` devuelve `0` (el peer cerró el envío) |
| `Socket::sendsome()` | `send()` falla con `EPIPE` (el peer cerró la recepción) |
| `Socket::shutdown()` | alguien llama `shutdown()` explícitamente |

Las primeras dos siempre corren en el thread dueño del socket (`Receiver` o `Sender`
de ese cliente) — nunca hay dos threads escribiendo ahí a la vez.

El problema es la tercera: en dos puntos del código, `shutdown()` se llama desde un
thread **distinto** al que está bloqueado en `recv()`/`send()`, justamente para
destrabarlo:

- **Server**: `GameLoop` detecta un cliente muerto (`ClientListMonitor::clean_dead()`)
  y llama `ClientHandler::stop()` → `protocol.shutdown()`, mientras el `Receiver` de
  ese mismo cliente puede estar bloqueado en `recvall()`.
- **Cliente**: `Client::shutdown()` corre en el thread principal y llama
  `protocol.shutdown()` para destrabar al `Receiver`, que puede estar bloqueado en
  `recv_event()`.

Cuando el `shutdown()` externo desbloquea el `recv()`/`send()` del thread dueño, los
dos threads terminan escribiendo `stream_status` casi en simultáneo: uno desde
`shutdown()`, el otro desde `recvsome()`/`sendsome()` procesando el resultado de la
syscall que se acaba de desbloquear. Como `stream_status` es un `int` común (no
atómico) y la operación es `|=` (lectura-modificación-escritura), es una data race
real, no solo un falso positivo de Helgrind.

Que los sockets sean bloqueantes (requisito de la cátedra) no evita esto — al
contrario, es la causa: para destrabar un `recv()` bloqueante desde otro thread no
hay más opción que actuar sobre el socket desde afuera.

### Impacto práctico

En las corridas de valgrind (memcheck, 5 escenarios) no se observó ningún crash ni
dato corrupto visible — la ventana de la carrera es chica. Pero es comportamiento
indefinido en el modelo de memoria de C++: no hay garantía de que `stream_status`
termine con el valor correcto. Un bit perdido ahí podría hacer que, por ejemplo, el
`Sender` no se entere de que el lado de envío ya está cerrado e intente mandar sobre
un socket muerto, generando una excepción inesperada en un momento distinto al
esperado. Es el tipo de bug que se manifiesta como crash intermitente y difícil de
reproducir, no como una falla determinística.

## Decisión

La decisión tiene dos partes: separar responsabilidades (para no necesitar un mutex)
y, sobre eso, hacer atómico el campo compartido (para no depender de qué par de
threads termine escribiéndolo).

### 1. Separar "destrabar la syscall" de "anotar que el stream se cerró"

Se agregó `Socket::shutdown_from_other_thread(int how)`, que ejecuta únicamente el
syscall `::shutdown(fd, how)` y **no toca `stream_status`**:

```cpp
void Socket::shutdown_from_other_thread(int how) {
    chk_skt_or_fail();
    if (::shutdown(this->skt, how) == -1) {
        throw LibError(errno, "socket shutdown failed");
    }
    // No tocamos `stream_status` acá a propósito.
}
```

El `shutdown()` original (que sí actualiza `stream_status`) queda para cuando el
propio thread dueño del socket lo cierra por su cuenta. La distinción importa porque
documenta, por el nombre del método, quién tiene permitido tocar el estado
compartido: si estás en el thread que usa el socket, `shutdown()`; si estás
destrabando el socket de otro thread, `shutdown_from_other_thread()`.

Se migraron los tres call sites que cruzan threads al método nuevo:

- `ClientHandler::stop()` (server, llamado por el thread del `GameLoop`) →
  `protocol.shutdown_from_other_thread()`.
- `Client::shutdown()` (cliente, llamado por el thread principal) →
  `protocol.shutdown_from_other_thread()`.
- `Acceptor::stop()` (server, llamado por el thread principal vía `Server::stop()`)
  → `listener.shutdown_from_other_thread()`.

Este último caso (`Acceptor`) no tenía la data race en la práctica: `Socket::accept()`
nunca lee ni escribe `stream_status`, así que no había ningún campo compartido en
juego. Pero es exactamente el mismo patrón de fondo (un thread destraba el socket
bloqueante de otro) y dejarlo con el `shutdown()` viejo dependía de un invariante
frágil e implícito ("`accept()` nunca va a tocar `stream_status`") que un cambio
futuro en `Socket::accept()` podría romper en silencio. Se migró por consistencia: con
este cambio, **todo** call site cruzado usa `shutdown_from_other_thread()`, y el
`shutdown()` original queda libre para que lo use el thread dueño si en algún momento
hace falta.

### Flujo resultante (caso server)

1. `Receiver::run()` queda bloqueado dentro de `recvall()` → `recvsome()` → `recv()`.
2. `GameLoop` detecta al cliente muerto y llama `clean_dead()` → `ClientHandler::stop()`.
3. `stop()` llama `protocol.shutdown_from_other_thread()`, que solo ejecuta el
   syscall. No escribe nada en el objeto `Socket`.
4. El `recv()` bloqueado se desbloquea, devuelve `0`.
5. `recvsome()` —corriendo en el thread `Receiver`, no en el del `GameLoop`— ve el `0`
   y ahí sí actualiza `stream_status`. Un solo escritor para ese bit.
6. `Receiver::run()` termina su loop, el `GameLoop` hace `join()`.

Mismo patrón en el cliente: `Client::shutdown()` joinea al `Sender` primero (así nadie
más escribe el lado "send"), después llama `shutdown_from_other_thread()` para
destrabar al `Receiver`, que anota su propio cierre cuando corresponda.

### 2. Hacer `stream_status` atómico (`std::atomic<int>` + `fetch_or`)

La separación de responsabilidades del punto 1 elimina la race contra el thread
*externo* (`GameLoop`/`Server`), pero queda una grieta más chica: `shutdown_from_other_thread(SHUT_RDWR)`
puede desbloquear **a la vez** tanto al `Receiver` como al `Sender` del mismo cliente
(si el `Sender` justo estaba bloqueado en `send()` por un cliente lento, no en
`queue.pop()`). En ese caso, los dos threads dueños —no ya el externo— escriben
`stream_status` casi en simultáneo: uno setea el bit de `RECV_CLOSED`, el otro el de
`SEND_CLOSED`. Aunque son bits distintos, `|=` es una operación de
lectura-modificación-escritura no atómica sobre el mismo `int`, así que un `|=` puede
pisar al otro (lost update) igual que en la race original.

Por eso `stream_status` pasó de `int` a `std::atomic<int>`, y los `|=` pasaron a
`fetch_or` (atómico). Las asignaciones simples (`= STREAM_BOTH_CLOSED`, etc.) en
constructores/move no necesitan cambios de lógica, solo se ajustó una copia
(`other.stream_status.load()`) porque `std::atomic` no tiene operator= entre dos
atómicos. Esto cierra la race sin importar qué combinación de threads termine
escribiendo el campo, no solo el par puntual que vio Helgrind.

### Alternativas consideradas

- **Mutex dedicado en `Socket`**: resuelve la race, pero agrega un lock más al
  proyecto cuando alcanza con que cada campo tenga un único escritor por bit y,
  ahora, una escritura atómica.
- **Solo `std::atomic<int>` + `fetch_or`, sin separar `shutdown`**: hace segura la
  memoria, pero no resuelve que el thread externo siga anotando un estado que
  conceptualmente le pertenece al thread dueño — se mezclan responsabilidades
  igual. Se prefirió combinarlo con la separación del punto 1.
- **Bandera atómica + señal (`EINTR`) para que el propio thread dueño se cierre solo**:
  es la versión "más pura" de ownership único, pero no es implementable sin cambiar la
  naturaleza bloqueante del socket (no hay un loop con polling donde revisar la
  bandera) o sin agregar manejo de señales por thread, que trae sus propios riesgos
  (lifetime del `pthread_t`, señales enmascaradas en threads incorrectos, conflictos
  con el manejo de señales que ya hace SDL del lado del cliente). Se descartó por
  complejidad desproporcionada al problema.

## Consecuencias

- Se eliminó la data race confirmada por Helgrind, y la grieta residual equivalente
  entre `Receiver` y `Sender`, sin agregar un mutex.
- `stream_status` cambió de `int` a `std::atomic<int>` en el TDA `Socket`
  (`common/socket.h`/`.cpp`); cualquier código nuevo que lo modifique debe usar
  `fetch_or`/`store` en vez de `|=`/`=` directo donde pueda haber más de un thread
  involucrado.
- `ServerProtocol::shutdown()` y `ClientProtocol::shutdown()` (las versiones viejas)
  quedaron sin ningún call site después de la migración y se borraron para no dejar
  código muerto.
- `Socket::shutdown(int how)` (la versión original) queda hoy sin ningún call site en
  el código del proyecto, ya que los tres llamadores cruzados (`ClientHandler`,
  `Client`, `Acceptor`) migraron a `shutdown_from_other_thread()`. Se mantiene en el
  TDA `Socket` como primitiva pública para el caso en que el thread dueño necesite
  cerrar su propio socket en el futuro, en vez de borrarla.
- Quedan 194/194 tests (`taller_tests`) pasando y el build sin warnings nuevos.
- Verificación: una corrida posterior de Helgrind sobre el mismo escenario (matar el
  server con un cliente conectado) ya no muestra el conflicto sobre `stream_status`;
  los únicos "possible data race" restantes son ruido conocido de `libgcc`
  (inicialización lazy de las tablas de unwind de excepciones vía `pthread_once`,
  sin relación con este código).

## Limitaciones conocidas

- **Sin test de regresión automatizado.** La verificación es manual (correr Helgrind
  y leer el log); no hay nada en `taller_tests` que falle si en el futuro alguien
  vuelve a llamar `shutdown()` en vez de `shutdown_from_other_thread()` desde un call
  site cruzado, o si se revierte el `std::atomic`. Un test de timing real para esto
  sería frágil (puede pasar en verde con el bug presente); una alternativa más
  confiable sería un test "de diseño" que falle si aparece una llamada a
  `protocol.shutdown()`/`Socket::shutdown()` fuera del thread dueño, pero no se
  implementó. Decisión explícita: se deja así por ahora.
- **`Socket::shutdown(int how)` sin call sites no está confirmado contra la consigna.**
  Se mantiene en el TDA por criterio propio (documenta intención, es una primitiva
  razonable), pero no se verificó si la cátedra espera esa firma específica como
  parte del contrato público de `Socket`. Si en algún momento se confirma que no hace
  falta, es candidato a borrarse siguiendo la convención del proyecto de no dejar
  código sin uso. Decisión explícita: se deja así por ahora.
