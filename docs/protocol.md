# Argentum Online — Protocolo de Comunicación Binario

## Índice

1. [Convenciones generales](#1-convenciones-generales)
2. [Tipos de datos primitivos](#2-tipos-de-datos-primitivos)
3. [Tabla de OpCodes](#3-tabla-de-opcodes)
4. [Mensajes Cliente → Servidor](#4-mensajes-cliente--servidor)
5. [Mensajes Servidor → Cliente](#5-mensajes-servidor--cliente)
6. [Flujos de sesión](#6-flujos-de-sesión)
7. [Enums](#7-Enums)
8. [Notas de implementación](#8-notas-de-implementación)

---

## 1. Convenciones generales

- Todo mensaje comienza con **1 byte de OpCode** que identifica el tipo de mensaje.
- Los enteros de 2 bytes (`uint16_t`) se transmiten en **Big-Endian** (network byte order). Usar `htons`/`ntohs`.
- Los enteros de 4 bytes (`uint32_t`) se transmiten en **Big-Endian**. Usar `htonl`/`ntohl`.
- Los strings se transmiten como: `uint16_t length` y luego `length` caracteres. **Sin null terminator**.
- Los valores booleanos se transmiten como `uint8_t`: `0x00` = false, `0x01` = true.

---

## 2. Tipos de datos primitivos

| Tipo       | Tamaño | Descripción                                      |
|------------|--------|--------------------------------------------------|
| `uint8_t`  | 1 byte | Entero sin signo de 8 bits. OpCodes, enums, bool |
| `uint16_t` | 2 bytes | Entero sin signo de 16 bits. Big-Endian          |
| `uint32_t` | 4 bytes | Entero sin signo de 32 bits. Big-Endian          |
| `int16_t`  | 2 bytes | Entero con signo de 16 bits. Big-Endian          |
| `int32_t`  | 4 bytes | Entero con signo de 32 bits. Big-Endian          |
| `string`   | variable | `uint16_t length` + `length` bytes UTF-8        |
| `bool`     | 1 byte | `uint8_t`: 0x00 = false, 0x01 = true             |

---

## 3. Tabla de OpCodes

### 3.1 Cliente → Servidor (`0x01` – `0x1D`)

| OpCode | Nombre                  | Descripción                            |
|--------|-------------------------|----------------------------------------|
| `0x01` | `LOGIN`                 | Envía credenciales de login            |
| `0x02` | `CREATE_CHARACTER`      | Crea un nuevo personaje                |
| `0x03` | `MOVE`                  | Movimiento en una dirección            |
| `0x04` | `ATTACK`                | Ataque al objetivo seleccionado        |
| `0x05` | `CAST_SPELL`            | Lanza hechizo al objetivo              |
| `0x06` | `PICKUP_ITEM`           | Recoger item del suelo (`/tomar`)      |
| `0x07` | `DROP_ITEM`             | Tirar item del inventario (`/tirar`)   |
| `0x08` | `EQUIP_ITEM`            | Equipar item del inventario            |
| `0x09` | `UNEQUIP_ITEM`          | Desequipar item                        |
| `0x0A` | `MEDITATE`              | Entrar/salir del estado de meditación  |
| `0x0B` | `RESURRECT`             | Solicitar resurrección (`/resucitar`)  |
| `0x0C` | `NPC_BUY`               | Comprar item a NPC (`/comprar`)        |
| `0x0D` | `NPC_SELL`              | Vender item a NPC (`/vender`)          |
| `0x0E` | `NPC_HEAL`              | Pedir curación a sacerdote (`/curar`)  |
| `0x0F` | `BANK_DEPOSIT`          | Depositar en banco (`/depositar`)      |
| `0x10` | `BANK_WITHDRAW`         | Retirar del banco (`/retirar`)         |
| `0x11` | `NPC_LIST`              | Listar inventario de NPC (`/listar`)   |
| `0x12` | `PRIVATE_MSG`           | Mensaje privado a jugador (`@nick`)    |
| `0x13` | `CLAN_FOUND`            | Fundar clan                            |
| `0x14` | `CLAN_JOIN_REQUEST`     | Pedir unirse a clan                    |
| `0x15` | `CLAN_REVIEW`           | Ver pedidos y miembros del clan        |
| `0x16` | `CLAN_ACCEPT`           | Aceptar pedido de clan                 |
| `0x17` | `CLAN_REJECT`           | Rechazar pedido de clan                |
| `0x18` | `CLAN_BAN`              | Banear jugador del clan                |
| `0x19` | `CLAN_KICK`             | Expulsar jugador del clan              |
| `0x1A` | `CLAN_LEAVE`            | Dejar el clan                          |
| `0x1B` | `CHEAT_INFINITE_HP`     | Cheat: vida infinita (toggle)          |
| `0x1C` | `CHEAT_INFINITE_MANA`   | Cheat: maná infinito (toggle)          |
| `0x1D` | `CHEAT_DIE`             | Cheat: morir instantáneamente          |

### 3.2 Servidor → Cliente (`0x80` – `0x9B`)

| OpCode | Nombre                  | Descripción                                      |
|--------|-------------------------|--------------------------------------------------|
| `0x80` | `LOGIN_OK`              | Login aceptado, envía player_id y datos iniciales|
| `0x81` | `LOGIN_ERROR`           | Login rechazado con motivo                       |
| `0x82` | `CHARACTER_CREATED`     | Personaje creado exitosamente                    |
| `0x83` | `CHARACTER_ERROR`       | Error al crear personaje con motivo              |
| `0x84` | `MAP_INFO`              | Datos del mapa al conectarse                     |
| `0x85` | `PLAYER_STATS`          | Stats completos del jugador (HP, maná, nivel...) |
| `0x86` | `ENTITY_SPAWN`          | Aparece entidad en el mapa (jugador o NPC)       |
| `0x87` | `ENTITY_DESPAWN`        | Desaparece entidad del mapa                      |
| `0x88` | `ENTITY_MOVE`           | Entidad se movió a nueva posición                |
| `0x89` | `DAMAGE_DEALT`          | Daño infligido a una entidad                     |
| `0x8A` | `DAMAGE_RECEIVED`       | Daño recibido por el jugador                     |
| `0x8B` | `ATTACK_DODGED`         | Ataque esquivado                                 |
| `0x8C` | `ENTITY_DIED`           | Una entidad murió                                |
| `0x8D` | `PLAYER_RESPAWNED`      | Jugador resucitó                                 |
| `0x8E` | `MEDITATION_START`      | Confirmación de inicio de meditación             |
| `0x8F` | `MEDITATION_STOP`       | Confirmación de fin de meditación                |
| `0x90` | `INVENTORY_UPDATE`      | Estado actual del inventario del jugador         |
| `0x91` | `EQUIP_UPDATE`          | Estado actual del equipamiento                   |
| `0x92` | `GOLD_UPDATE`           | Cantidad de oro actualizada                      |
| `0x93` | `ITEM_DROPPED`          | Item apareció en el suelo en (x, y)              |
| `0x94` | `ITEM_PICKED`           | Item fue recogido del suelo                      |
| `0x95` | `NPC_ITEM_LIST`         | Lista de items de un NPC (respuesta a /listar)   |
| `0x96` | `TRANSACTION_OK`        | Compra/venta/depósito/retiro exitoso             |
| `0x97` | `TRANSACTION_ERROR`     | Compra/venta/depósito/retiro fallido             |
| `0x98` | `CHAT_MSG`              | Mensaje de chat recibido                         |
| `0x99` | `CLAN_NOTIFICATION`     | Notificación del clan (entrada/salida/ataque)    |
| `0x9A` | `CLAN_UPDATE`           | Estado actualizado del clan                      |
| `0x9B` | `SERVER_MSG`            | Mensaje del servidor (info/error genérico)       |

---

## 4. Mensajes Cliente → Servidor

---

### `0x01` LOGIN

El primer mensaje que envía el cliente tras conectarse.

```
0x01 <len_username> <username> <len_password> <password>
```

---

### `0x02` CREATE_CHARACTER

```
0x02 <len_username> <username> <len_password> <password> <race> <class>
```

---

### `0x03` MOVE

```
TBD
```

---

### `0x04` ATTACK

```
TBD
Diferenciar melee y a distancia/hechizo
```

---

### `0x05` CAST_SPELL

```
TBD
```

---

### `0x06` PICKUP_ITEM

El servidor toma el item en la celda donde está parado el jugador.  

```
TBD
```

---

### `0x07` DROP_ITEM

```
TBD
```

---

### `0x08` EQUIP_ITEM

```
TBD
```

Si es una poción, se consume inmediatamente. Si ya hay algo equipado en ese slot, se intercambia.

---

### `0x09` UNEQUIP_ITEM

```
TBD
```

---

### `0x0A` MEDITATE  

Si no está meditando, entra en meditación. Cualquier otro comando cancela.

```
TBD
```

---

### `0x0B` RESURRECT

```
TBD
```

---

### `0x0C` NPC_BUY

```
TBD
```

---

### `0x0D` NPC_SELL

```
TBD
```

---

### `0x0E` NPC_HEAL

```
TBD
```

---

### `0x0F` BANK_DEPOSIT

```
TBD
```

Si `deposit_type == 0x00`:
```
TBD
```

Si `deposit_type == 0x01`:
```
TBD
```

---

### `0x10` BANK_WITHDRAW

```
TBD
```

Si `withdraw_type == 0x00`:
```
TBD
```

Si `withdraw_type == 0x01`:
```
TBD
```

---

### `0x11` NPC_LIST  

```
TBD
```

---

### `0x12` PRIVATE_MSG  

```
TBD
```

---

### `0x13` CLAN_FOUND  

```
TBD
```

---

### `0x14` CLAN_JOIN_REQUEST  

```
TBD
```

---

### `0x15` CLAN_REVIEW  

```
TBD
```

---

### `0x16` CLAN_ACCEPT  

```
TBD
```

---

### `0x17` CLAN_REJECT  

```
TBD
```

---

### `0x18` CLAN_BAN  

```
TBD
```

---

### `0x19` CLAN_KICK  

```
TBD
```

---

### `0x1A` CLAN_LEAVE  

```
TBD
```

---

### `0x1B` CHEAT_INFINITE_HP  

El servidor activa/desactiva vida infinita para este jugador.

```
TBD
```

---

### `0x1C` CHEAT_INFINITE_MANA  

```
TBD
```

---

### `0x1D` CHEAT_DIE  

```
TBD
```

---

## 5. Mensajes Servidor → Cliente

---

### `0x80` LOGIN_OK

```
0x80 <player_id> <len_username> <username> <race> <class> <level> <experience> <hp_curr> <hp_max> <mana_current> <mana_max> <gold> <pos_x> <pos_y>
```

Inmediatamente después el servidor envía `MAP_INFO` e `INVENTORY_UPDATE`.

---

### `0x81` LOGIN_ERROR

```
0x81 <error_code> <message>
```

---

### `0x82` CHARACTER_CREATED

Misma estructura que `LOGIN_OK`: el personaje fue creado y la sesión queda abierta.

```
0x82 <player_id> <len_username> <username> <race> <class> <level> <experience> <hp_curr> <hp_max> <mana_current> <mana_max> <gold> <pos_x> <pos_y>
```

---

### `0x83` CHARACTER_ERROR  

```
0x83 <error_code> <message>
```

---

### `0x84` MAP_INFO  

Enviado una vez después del login. El cliente lo usa para construir el mapa.

```
TBD
```

---

### `0x85` PLAYER_STATS  

Enviado cuando cambian los stats del jugador propio (subir de nivel, recibir daño, etc).

```
TBD
```

---

### `0x86` ENTITY_SPAWN  

```
TBD
```

---

### `0x87` ENTITY_DESPAWN  

```
TBD
```

---

### `0x88` ENTITY_MOVE  

```
TBD
```

---

### `0x89` DAMAGE_DEALT  

Daño que el jugador le infligió a otro.

```
TBD
```

---

### `0x8A` DAMAGE_RECEIVED  

Daño que recibió el jugador. Va al mini-chat.

```
TBD
```

---

### `0x8B` ATTACK_DODGED  

```
TBD
```

---

### `0x8C` ENTITY_DIED  

```
TBD
```

---

### `0x8D` PLAYER_RESPAWNED  

```
TBD
```

---

### `0x8E` MEDITATION_START  

```
TBD
```

---

### `0x8F` MEDITATION_STOP  

```
TBD
```

---

### `0x90` INVENTORY_UPDATE  

Estado completo del inventario. Enviado tras login y tras cualquier cambio.

```
TBD
```

---

### `0x91` EQUIP_UPDATE  

Estado del equipamiento actual. Enviado tras login y tras equip/unequip.

```
TBD
```

---

### `0x92` GOLD_UPDATE  

```
TBD
```

---

### `0x93` ITEM_DROPPED  

Item apareció en el suelo (por drop de jugador, muerte de NPC, etc).

```
TBD
```

---

### `0x94` ITEM_PICKED  

Item del suelo fue recogido (por el jugador u otro).

```
TBD
```

---

### `0x95` NPC_ITEM_LIST  

Respuesta a `NPC_LIST`. Lista de items que el NPC tiene para vender, o items del banco.

```
TBD
```

---

### `0x96` TRANSACTION_OK  

```
TBD
```

---

### `0x97` TRANSACTION_ERROR  

```
TBD
```

---

### `0x98` CHAT_MSG  

Mensaje de chat (privado o de sistema).

```
TBD
```

---

### `0x99` CLAN_NOTIFICATION  

Notificación de evento del clan.

```
TBD
```

---

### `0x9A` CLAN_UPDATE  

Estado actualizado del clan (respuesta a CLAN_REVIEW o cambios).

```
TBD
```

---

### `0x9B` SERVER_MSG  

Mensaje de texto genérico del servidor. Para info, errores no críticos, eventos del sistema.

```
TBD
```

---

## 6. Flujos de sesión

### 6.1 Login de jugador existente

```
Cliente                                 Servidor
   |                                        |
   |------- LOGIN (0x01) ------------------>|
   |                                        | valida credenciales
   |<------ LOGIN_OK (0x80) ----------------|
   |<------ MAP_INFO (0x84) ----------------|
   |<------ INVENTORY_UPDATE (0xB0) --------|
   |<------ EQUIP_UPDATE (0xB1) ------------|
   |<------ GOLD_UPDATE (0xB2) -------------|
   |        (ENTITY_SPAWN x N cercanos)     |
   |             ... juego ...              |
   |                                        |
   |        [socket cerrado por cliente]    |
   |                                        | libera al jugador
```

### 6.2 Creación de personaje

```
Cliente                                 Servidor
   |                                        |
   |--- CREATE_CHARACTER (0x02) ----------->|
   |                                        | valida nombre único
   |<-- CHARACTER_CREATED (0x82) -----------|  (mismo payload que LOGIN_OK)
   |<-- MAP_INFO (0x84) --------------------|
   |<-- INVENTORY_UPDATE (0xB0) ------------|  (inventario vacío)
   |<-- EQUIP_UPDATE (0xB1) ----------------|  (sin equipamiento)
   |<-- GOLD_UPDATE (0xB2) ---------------- |  (oro inicial = 0)
```

### 6.3 Flujo de combate melee

```
TBD
```

### 6.4 Muerte y resurrección

```
TBD
```

### 6.5 Equipar un item

```
TBD
```

---

## 7. Enums

Todos los valores son `uint8_t`.

### Race
```
0x01    HUMAN
0x02    ELF
0x03    DWARF
0x04    GNOME
```

### Class
```
0x01    MAGE
0x02    CLERIC
0x03    PALADIN
0x04    WARRIOR
```

### Direction
```
0x00    NORTH
0x01    SOUTH
0x02    EAST
0x03    WEST
```

### ItemType
```
0x00    NONE
0x01    SWORD
0x02    AXE
0x03    HAMMER
0x04    ASH_STAFF          (vara de fresno - flecha mágica)
0x05    ELVEN_FLUTE        (flauta élfica - curar)
0x06    KNOTTED_STAFF      (báculo nudoso - misil)
0x07    STUDDED_STAFF      (báculo engarzado - explosión)
0x08    SIMPLE_BOW
0x09    COMPOSITE_BOW
0x0A    LEATHER_ARMOR
0x0B    PLATE_ARMOR
0x0C    BLUE_TUNIC
0x0D    HOOD
0x0E    IRON_HELMET
0x0F    TURTLE_SHIELD
0x10    IRON_SHIELD
0x11    MAGIC_HAT
0x12    HEALTH_POTION
0x13    MANA_POTION
0x14    GOLD_DROP          (oro caído al piso)
```

### EquipSlot
```
0x00    WEAPON
0x01    ARMOR
0x02    HELMET
0x03    SHIELD
```

### TileType
```
0x00    GRASS
0x01    SAND
0x02    FOREST
0x03    WATER
0x04    ROAD
0x05    DUNGEON_FLOOR
0x06    WALL
0x07    BUILDING_WALL
```

### LogInError
```
0x01    INVALID_CREDENTIALS
0x02    ALREADY_LOGGED_IN
0x03    SERVER_FULL
```

### CharacterError
```
0x01    USERNAME_TAKEN
0x02    INVALID_USERNAME    (vacío, muy largo, caracteres inválidos)
```

### TransactionType
```
0x01    BUY
0x02    SELL
0x03    DEPOSIT
0x04    WITHDRAW
0x05    HEAL
```

### ChatMsgType
```
0x00    SYSTEM              (mensaje del servidor)
0x01    PRIVATE             (mensaje privado @nick)
0x02    CLAN                (mensaje del sistema de clanes)
```

### ClanNotifType
```
0x00    MEMBER_ONLINE       (un miembro del clan entró)
0x01    MEMBER_OFFLINE      (un miembro del clan salió)
0x02    MEMBER_ATTACKED     (un miembro del clan está siendo atacado)
0x03    JOIN_REQUEST        (alguien pidió unirse)
0x04    JOIN_ACCEPTED       (tu pedido fue aceptado)
0x05    JOIN_REJECTED       (tu pedido fue rechazado)
0x06    KICKED              (fuiste expulsado)
```

---

## 8. Notas de implementación

### Byte order
Todos los `uint16_t` y `uint32_t` deben ser convertidos al enviar (`htons`, `htonl`) y al recibir (`ntohs`, `ntohl`). Los `uint8_t` no necesitan conversión.

### Manejo de errores de protocolo
- Si el servidor recibe un opcode desconocido, cierra la conexión con ese cliente.
- Si el cliente recibe un opcode desconocido, cierra la conexión y muestra error.

### Strings y encoding
- Encoding: UTF-8.
- Longitud máxima de username: 20 caracteres.
- Longitud máxima de nombre de clan: 30 caracteres.
- Longitud máxima de mensaje de chat: 255 caracteres.
- Estos límites deben validarse en ambos lados antes de serializar.

### Cheats
- Solo disponibles en builds de Debug o si el servidor tiene `enable_cheats = true` en el TOML.
- En build Release con cheats deshabilitados, el servidor ignora silenciosamente los mensajes `0x60`–`0x62`.
