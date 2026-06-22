# Argentum MALT — Protocolo de Comunicación Binario

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
- Los strings se transmiten como: `uint16_t length` y luego `length` bytes (sin conversión de byte order, se mandan tal cual). **Sin null terminator**.
- Los valores booleanos se transmiten como 1 byte crudo (no normalizado a 0x00/0x01): `protocol.send_bool`/`recv_bool` hacen `sendall(&value, sizeof(bool))`.
- Los enums se transmiten siempre como `uint8_t` (1 byte).

---

## 2. Tipos de datos primitivos

| Tipo       | Tamaño | Descripción                                      |
|------------|--------|--------------------------------------------------|
| `uint8_t`  | 1 byte | Entero sin signo de 8 bits. OpCodes, enums, bool |
| `uint16_t` | 2 bytes | Entero sin signo de 16 bits. Big-Endian          |
| `uint32_t` | 4 bytes | Entero sin signo de 32 bits. Big-Endian          |
| `string`   | variable | `uint16_t length` + `length` bytes              |
| `bool`     | 1 byte | Valor crudo de `bool` (no se normaliza a 0x00/0x01 explícitamente, pero en la práctica siempre vale 0 o 1) |

---

## 3. Tabla de OpCodes

> Esta tabla refleja `common/protocol.h` (`enum class OpCode`) exactamente.

### 3.1 Cliente → Servidor (`0x01` – `0x29`)

| OpCode | Nombre                  | Descripción                            |
|--------|-------------------------|-----------------------------------------|
| `0x01` | `LOGIN`                 | Envía credenciales de login            |
| `0x02` | `CREATE_CHARACTER`      | Crea un nuevo personaje                |
| `0x03` | `MOVE`                  | Movimiento en una dirección            |
| `0x04` | `ATTACK`                | Ataque cuerpo a cuerpo al objetivo seleccionado |
| `0x05` | `CAST_SPELL`            | Ataque a distancia / hechizo al objetivo |
| `0x06` | `PICKUP_ITEM`           | Recoger item del suelo (`/tomar`)      |
| `0x07` | `DROP_ITEM`             | Tirar item del inventario (`/tirar`)   |
| `0x08` | `EQUIP_ITEM`            | Equipar item del inventario            |
| `0x09` | `UNEQUIP_ITEM`          | Desequipar item                        |
| `0x0A` | `MEDITATE`              | Entrar en meditación |
| `0x0B` | `RESURRECT`             | Solicitar resurrección (`/resucitar`)  |
| `0x0C` | `NPC_BUY`               | Comprar item a NPC (`/comprar`)        |
| `0x0D` | `NPC_SELL`              | Vender item a NPC (`/vender`)          |
| `0x0E` | `NPC_HEAL`              | Pedir curación a sacerdote (`/curar`)  |
| `0x0F` | `BANK_DEPOSIT`          | Depositar en banco (`/depositar`)      |
| `0x10` | `BANK_WITHDRAW`         | Retirar del banco (`/retirar`)         |
| `0x11` | `NPC_LIST`              | Listar inventario de NPC (`/listar`)   |
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
| `0x1D` | `CHEAT_DIE`             | Cheat: muerte instantánea              |
| `0x1E` | `SEND_CHAT`             | Mensaje de texto al chat (incluye `/comandos`, `@privados` y chat normal — ver [4](#0x1e-send_chat)) |
| `0x1F` | `CHEAT_LEVEL_UP`        | Cheat: subir un nivel                  |
| `0x20` | `CHEAT_LEVEL_DOWN`      | Cheat: bajar un nivel                  |
| `0x21` | `CHANGE_MAP`            | Cruzar un portal/prop de transición    |
| `0x22` | `CHEAT_ADD_GOLD`        | Cheat: agregar 1000 de oro             |
| `0x23` | `CHEAT_VELOCITY`        | Cheat: velocidad aumentada (toggle)    |
| `0x24` | `CHEAT_REVIVE`          | Cheat: revivir                         |
| `0x25` | `CHEAT_RESET_GOLD`      | Cheat: resetear oro a 0                |
| `0x26` | `CHEAT_FILL_INVENTORY`  | Cheat: llenar inventario con todos los ítems |
| `0x27` | `CHEAT_RESET_MANA`      | Cheat: resetear maná a 0               |
| `0x28` | `CHEAT_CLEAR_INVENTORY` | Cheat: limpiar inventario              |
| `0x29` | `CLAN_UNBAN`            | Desbanear jugador del clan             |

### 3.2 Servidor → Cliente (`0x80` – `0x9F`)

| OpCode | Nombre                  | Descripción                                      |
|--------|-------------------------|---------------------------------------------------|
| `0x80` | `LOGIN_OK`              | Login aceptado, envía player_id y datos iniciales|
| `0x81` | `LOGIN_ERROR`           | Login rechazado con motivo                       |
| `0x82` | `CHARACTER_CREATED`     | Personaje creado exitosamente                    |
| `0x83` | `CHARACTER_ERROR`       | Error al crear personaje con motivo              |
| `0x85` | `PLAYER_STATS`          | Stats completos del jugador (HP, maná, nivel...) |
| `0x86` | `ENTITY_SPAWN`          | Aparece entidad en el mapa (jugador o NPC)       |
| `0x87` | `ENTITY_DESPAWN`        | Desaparece entidad del mapa                      |
| `0x88` | `ENTITY_MOVE`           | Entidad se movió a nueva posición                |
| `0x89` | `DAMAGE_DEALT`          | Daño infligido a otra entidad (eco para el atacante) |
| `0x8A` | `DAMAGE_RECEIVED`       | Daño recibido por una entidad (eco para el atacado, y para terceros que la ven) |
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
| `0x95` | `NPC_ITEM_LIST`         | Lista de items de un NPC (respuesta a `NPC_LIST`)|
| `0x98` | `CHAT_MSG`              | Mensaje de chat (sistema, privado, clan o chat general) — también usado para confirmaciones/errores de transacción y mensajes genéricos del servidor |
| `0x99` | `CLAN_NOTIFICATION`     | Notificación del clan (entrada/salida/ataque/pedidos) |
| `0x9A` | `CLAN_UPDATE`           | Lista de miembros del clan y su estado online/offline, a todos los miembros |
| `0x9C` | `HEAL_RECEIVED`         | El jugador recuperó vida y/o maná (regen pasiva, meditación o `/curar`) |
| `0x9D` | `MAP_TRANSITION`        | El jugador cruzó un portal: nuevo mapa y posición |
| `0x9E` | `SPELL_EFFECT`          | Efecto visual de hechizo a reproducir sobre un objetivo |
| `0x9F` | `BANK_UPDATE`           | Estado actual del banco (slots + oro)            |

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
0x03 <direction>
```

`direction`: 1 byte, ver [enum Direction](#direction).

---

### `0x04` ATTACK

Ataque cuerpo a cuerpo. Requiere estar al lado del objetivo (o dentro del rango del arma equipada). El ataque a distancia (arco) usa este mismo opcode — la diferencia melee/distancia la determina el arma equipada del atacante, no el mensaje.

```
0x04 <target_id>
```

`target_id`: `uint16_t`, id de la entidad (jugador o NPC) objetivo.

---

### `0x05` CAST_SPELL

Ataque o curación con bastón/hechizo equipado.

```
0x05 <target_id>
```

`target_id`: `uint16_t`.

---

### `0x06` PICKUP_ITEM

El servidor toma el item en la celda donde está parado el jugador.

```
0x06 <len_item_name> <item_name>
```

`item_name`: nombre del ítem a levantar. Si está vacío (`len_item_name = 0`), se levanta cualquier ítem apilado en esa posición (`/tomar` sin argumento).

---

### `0x07` DROP_ITEM

```
0x07 <len_item_name> <item_name>
```

`item_name`: nombre del ítem del inventario a tirar.

---

### `0x08` EQUIP_ITEM

```
0x08 <slot_index>
```

`slot_index`: `uint8_t`, índice del slot del inventario (no del equipamiento) a equipar. Si es una poción, se consume inmediatamente. Si ya hay algo equipado en el slot de destino, se intercambia con lo que estaba en el inventario.

---

### `0x09` UNEQUIP_ITEM

```
0x09 <slot>
```

`slot`: 1 byte, ver [enum EquipSlot](#equipslot). Indica qué ranura de equipamiento desequipar (no un índice de inventario).

---

### `0x0A` MEDITATE

Si el jugador no está meditando, entra en meditación. Cualquier otra acción (moverse, atacar, comandos) lo saca del estado de meditación del lado del servidor.

```
0x0A
```

Sin payload — solo el OpCode.

---

### `0x0B` RESURRECT

```
0x0B
```

Sin payload.

---

### `0x0C` NPC_BUY

```
0x0C <len_item_name> <item_name>
```

---

### `0x0D` NPC_SELL

```
0x0D <len_item_name> <item_name>
```

---

### `0x0E` NPC_HEAL

```
0x0E
```

Sin payload.

---

### `0x0F` BANK_DEPOSIT

```
0x0F <is_gold>
```

`is_gold`: `uint8_t` (`0x00`/`0x01`). Según su valor, sigue:

Si `is_gold == 0x01`:
```
<gold_amount>
```
`gold_amount`: `uint32_t`.

Si `is_gold == 0x00`:
```
<len_item_name> <item_name>
```

---

### `0x10` BANK_WITHDRAW

Mismo formato que `BANK_DEPOSIT`:

```
0x10 <is_gold> [<gold_amount> | <len_item_name> <item_name>]
```

---

### `0x11` NPC_LIST

```
0x11
```

Sin payload.

---

### `0x13` CLAN_FOUND

```
0x13 <len_clan_name> <clan_name>
```

---

### `0x14` CLAN_JOIN_REQUEST

```
0x14 <len_clan_name> <clan_name>
```

---

### `0x15` CLAN_REVIEW

Solo el fundador puede usarlo. La respuesta **no** es un mensaje dedicado: el servidor arma
un texto con la lista de miembros (con su estado online/offline) y los pedidos pendientes,
y lo manda como `CHAT_MSG` (`0x98`) tipo `SYSTEM`.

```
0x15
```

Sin payload.

---

### `0x16` CLAN_ACCEPT

```
0x16 <len_target_nick> <target_nick>
```

---

### `0x17` CLAN_REJECT

```
0x17 <len_target_nick> <target_nick>
```

---

### `0x18` CLAN_BAN

```
0x18 <len_target_nick> <target_nick>
```

---

### `0x19` CLAN_KICK

```
0x19 <len_target_nick> <target_nick>
```

---

### `0x1A` CLAN_LEAVE

```
0x1A
```

Sin payload.

---

### `0x1B` CHEAT_INFINITE_HP

El servidor alterna vida infinita para este jugador. Sin efecto si `cheats_enabled = false` en `server.toml` (el servidor ignora el comando silenciosamente).

```
0x1B
```

Sin payload.

---

### `0x1C` CHEAT_INFINITE_MANA

```
0x1C
```

Sin payload.

---

### `0x1D` CHEAT_DIE

```
0x1D
```

Sin payload.

---

### `0x1E` SEND_CHAT

```
0x1E <len_text> <text>
```

`text`: el mensaje tal cual lo escribió el jugador. El servidor (`Game::handle_send_chat_msg`) lo interpreta así:
- Empieza con `@`: mensaje privado. Se parsea `@<nick> <mensaje>`; si `nick` existe y está conectado, se emite `CHAT_MSG` tipo `PRIVATE` al emisor y al destinatario. Si no existe, el emisor recibe `CHAT_MSG` tipo `SYSTEM` con el error.
- Empieza con `/`: comando (`/help`, comandos de clan vía `ClanCommandHandler`, etc). Si no se reconoce, el emisor recibe `CHAT_MSG` tipo `SYSTEM` con el error.
- Cualquier otro texto: chat general, se hace `broadcast` como `CHAT_MSG` tipo `SAY`.

---

### `0x1F` CHEAT_LEVEL_UP / `0x20` CHEAT_LEVEL_DOWN / `0x22` CHEAT_ADD_GOLD / `0x23` CHEAT_VELOCITY / `0x24` CHEAT_REVIVE / `0x25` CHEAT_RESET_GOLD / `0x26` CHEAT_FILL_INVENTORY / `0x27` CHEAT_RESET_MANA / `0x28` CHEAT_CLEAR_INVENTORY

Todos sin payload, solo el OpCode. Mismo gate de `cheats_enabled` que `0x1B`–`0x1D`.

---

### `0x21` CHANGE_MAP

Enviado cuando el jugador interactúa con un prop de transición (portal).

```
0x21 <len_prop_name> <prop_name>
```

`prop_name`: nombre del prop de transición pisado/clickeado, usado por el servidor para resolver a qué mapa y posición transportar al jugador (responde con `MAP_TRANSITION`).

---

### `0x29` CLAN_UNBAN

```
0x29 <len_target_nick> <target_nick>
```

---

## 5. Mensajes Servidor → Cliente

---

### `0x80` LOGIN_OK

```
0x80 <player_id> <len_username> <username> <race> <class> <level> <experience> <exp_to_next> <hp_curr> <hp_max> <mana_current> <mana_max> <gold> <pos_x> <pos_y>
```

Inmediatamente después el servidor envía `INVENTORY_UPDATE`, `EQUIP_UPDATE`, `GOLD_UPDATE` y un `ENTITY_SPAWN` por cada entidad cercana (ver [6.1](#61-login-de-jugador-existente)). No hay un mensaje de "info de mapa": el cliente carga la geometría desde `config/*.toml` en disco.

---

### `0x81` LOGIN_ERROR

```
0x81 <error_code> <len_message> <message>
```

`error_code`: 1 byte, ver [enum LoginError](#loginerror).

---

### `0x82` CHARACTER_CREATED

Mismo payload que `LOGIN_OK`: el personaje fue creado y la sesión queda abierta.

```
0x82 <player_id> <len_username> <username> <race> <class> <level> <experience> <exp_to_next> <hp_curr> <hp_max> <mana_current> <mana_max> <gold> <pos_x> <pos_y>
```

---

### `0x83` CHARACTER_ERROR

```
0x83 <error_code> <len_message> <message>
```

`error_code`: 1 byte, ver [enum CharacterError](#charactererror).

---

### `0x85` PLAYER_STATS

Enviado cuando cambian los stats derivados del jugador propio (después de cada ataque propio, al subir/bajar de nivel, etc). Incluye los rangos de daño/defensa ya resueltos contra el arma/armadura equipada.

```
0x85 <level> <experience> <exp_to_next> <hp_current> <hp_max> <mana_current> <mana_max> <crit_chance> <damage_min> <damage_max> <defense_min> <defense_max> <dodge_chance> <strength> <agility>
```

| Campo | Tipo |
|---|---|
| `level` | `uint8_t` |
| `experience`, `exp_to_next`, `hp_current`, `hp_max`, `mana_current`, `mana_max` | `uint32_t` |
| `crit_chance` | `uint8_t` (0–100) |
| `damage_min`, `damage_max`, `defense_min`, `defense_max` | `uint16_t` |
| `dodge_chance` | `uint8_t` (0–100) |
| `strength`, `agility` | `uint16_t` |

---

### `0x86` ENTITY_SPAWN

```
0x86 <entity_id> <entity_type> <pos_x> <pos_y> <entity_dir> <len_entity_name> <entity_name> <entity_race> <entity_class> <weapon_type> <armor_type> <helmet_type> <shield_type> <sprite_id> <len_clan_name> <clan_name>
```

| Campo | Tipo |
|---|---|
| `entity_id` | `uint16_t` |
| `entity_type` | `uint8_t`, ver [enum EntityType](#entitytype) |
| `pos_x`, `pos_y` | `uint16_t` |
| `entity_dir` | `uint8_t`, ver [enum Direction](#direction) |
| `entity_race`, `entity_class` | `uint8_t` (para NPCs se mandan igual, sin significado especial) |
| `weapon_type`/`armor_type`/`helmet_type`/`shield_type` | `uint8_t`, ver [enum ItemType](#itemtype); `0x00` (`NONE`) si no tiene equipado |
| `sprite_id` | `uint16_t` (NPCs) |
| `clan_name` | string vacío si la entidad no tiene clan |

---

### `0x87` ENTITY_DESPAWN

```
0x87 <entity_id>
```

---

### `0x88` ENTITY_MOVE

```
0x88 <entity_id> <pos_x> <pos_y> <entity_dir>
```

---

### `0x89` DAMAGE_DEALT

Eco al atacante: cuánto daño acaba de infligir.

```
0x89 <target_id> <damage>
```

`damage`: `uint32_t`.

---

### `0x8A` DAMAGE_RECEIVED

Enviado al objetivo (y a quien más esté mirando ese combate) con el daño recibido y su HP resultante.

```
0x8A <target_id> <attacker_id> <damage> <hp_current> <hp_max>
```

---

### `0x8B` ATTACK_DODGED

```
0x8B <player_id>
```

`player_id`: quién esquivó el ataque.

---

### `0x8C` ENTITY_DIED

```
0x8C <entity_id>
```

---

### `0x8D` PLAYER_RESPAWNED

```
0x8D <entity_id> <hp_current> <hp_max>
```

---

### `0x8E` MEDITATION_START

```
0x8E
```

Sin payload.

---

### `0x8F` MEDITATION_STOP

```
0x8F
```

Sin payload.

---

### `0x90` INVENTORY_UPDATE

Estado completo del inventario. Enviado tras login y tras cualquier cambio (recoger/tirar/comprar/vender/equipar/desequipar/morir).

```
0x90 <slot_count> <slot_index_1> <item_type_1> <len_item_name_1> <item_name_1> ... <slot_index_N> <item_type_N> <len_item_name_N> <item_name_N>
```

`slot_count`: `uint8_t`. Por cada slot ocupado: `slot_index` (`uint8_t`), `item_type` (`uint8_t`), `item_name` (string).

---

### `0x91` EQUIP_UPDATE

Estado del equipamiento actual. Enviado tras login y tras equipar/desequipar. Siempre manda las 4 ranuras en este orden fijo: arma, armadura, casco, escudo (sin slot de ranura vacía explícito; `item_type = NONE` indica ranura vacía).

```
0x91 <entity_id> <weapon_type> <len_weapon_name> <weapon_name> <armor_type> <len_armor_name> <armor_name> <helmet_type> <len_helmet_name> <helmet_name> <shield_type> <len_shield_name> <shield_name>
```

---

### `0x92` GOLD_UPDATE

```
0x92 <gold>
```

`gold`: `uint32_t`.

---

### `0x93` ITEM_DROPPED

Item apareció en el suelo (por drop de jugador al morir/tirar, o muerte de NPC).

```
0x93 <pos_x> <pos_y> <item_type> <len_item_name> <item_name> <amount>
```

`amount`: `uint32_t`, solo > 0 cuando `item_type == GOLD_DROP` (representa la cantidad de oro).

---

### `0x94` ITEM_PICKED

Item del suelo fue recogido (por el jugador u otro, para que todos dejen de renderizarlo).

```
0x94 <pos_x> <pos_y> <len_item_name> <item_name> <amount>
```

---

### `0x95` NPC_ITEM_LIST

Respuesta a `NPC_LIST`. Lista de items que el NPC (comerciante/sacerdote) tiene para vender, o el contenido del banco.

```
0x95 <item_count> <len_item_name_1> <item_name_1> <item_type_1> <sprite_id_1> <price_1> ... 
```

`item_count`: `uint16_t` (no `uint8_t`, a diferencia de `INVENTORY_UPDATE`/`BANK_UPDATE`). Por cada ítem: `item_name` (string), `item_type` (`uint8_t`), `sprite_id` (`uint8_t`), `price` (`uint32_t`).

---

### `0x98` CHAT_MSG

Las confirmaciones y errores de compra/venta/depósito/retiro (y los mensajes genéricos del
servidor en general) se comunican con este mensaje, tipo `SYSTEM`, en vez de un opcode de
transacción dedicado — siempre acompañado del evento de estado correspondiente
(`GOLD_UPDATE`, `INVENTORY_UPDATE`, `BANK_UPDATE`).

```
0x98 <type> <len_sender_name> <sender_name> <len_message> <message> <recipient_id> <sender_id>
```

| Campo | Tipo | Notas |
|---|---|---|
| `type` | `uint8_t` | ver [enum ChatMsgType](#chatmsgtype) |
| `sender_name` | string | vacío para mensajes `SYSTEM` |
| `message` | string | |
| `recipient_id`, `sender_id` | `uint16_t` | usados en mensajes `PRIVATE` para que el cliente sepa con quién es la conversación; `0` en los demás tipos |

---

### `0x99` CLAN_NOTIFICATION

Notificación de evento del clan (entrada/salida de un miembro, ataque a un compañero, pedido de ingreso, resultado del propio pedido, expulsión).

```
0x99 <type> <len_username> <username> <len_clan_name> <clan_name>
```

`type`: `uint8_t`, ver [enum ClanNotifType](#clannotiftype).

---

### `0x9A` CLAN_UPDATE

Se manda a todos los miembros del clan (no solo al que disparó la acción) cada vez que cambia
la membresía: fundar, aceptar, expulsar, banear o dejar el clan. **No** es la respuesta a
`CLAN_REVIEW` (ver [0x15](#0x15-clan_review)) — ese comando responde con `CHAT_MSG`. Los
pedidos pendientes de ingreso se notifican al fundador por separado, vía `CLAN_NOTIFICATION`
tipo `JOIN_REQUEST`, en el momento en que se piden.

```
0x9A <len_clan_name> <clan_name> <member_count> <len_username_1> <username_1> <is_founder_1> <is_online_1> ...
```

`member_count`: `uint8_t`. Por cada miembro: `username` (string), `is_founder` (`bool`), `is_online` (`bool`).

---

### `0x9C` HEAL_RECEIVED

El jugador recuperó vida y/o maná: regeneración pasiva por tiempo, meditación, o `/curar` de un sacerdote.

```
0x9C <player_id> <hp_current> <mana_current>
```

---

### `0x9D` MAP_TRANSITION

Respuesta a `CHANGE_MAP`: el jugador cruzó un portal.

```
0x9D <len_map_name> <map_name> <pos_x> <pos_y>
```

`map_name`: nombre del mapa de destino (coincide con las claves de `config/map_list.toml`).

---

### `0x9E` SPELL_EFFECT

Efecto visual a reproducir sobre la entidad objetivo (no representa daño/curación por sí mismo, esos van en `DAMAGE_RECEIVED`/`HEAL_RECEIVED`).

```
0x9E <target_id> <effect_type>
```

`effect_type`: `uint8_t`. `0x00` = curar (único valor usado actualmente, ver `Flauta élfica` en el manual).

---

### `0x9F` BANK_UPDATE

Estado completo del banco (objetos + oro). Enviado tras login (si el cliente lo solicita) y tras cualquier depósito/retiro.

```
0x9F <slot_count> <slot_index_1> <item_type_1> <len_item_name_1> <item_name_1> ... <gold>
```

Mismo formato de slots que `INVENTORY_UPDATE`, con `gold` (`uint32_t`) al final.

---

## 6. Flujos de sesión

### 6.1 Login de jugador existente

```
Cliente                                 Servidor
   |                                        |
   |------- LOGIN (0x01) ------------------>|
   |                                        | valida credenciales
   |<------ LOGIN_OK (0x80) ----------------|
   |<------ INVENTORY_UPDATE (0x90) --------|
   |<------ EQUIP_UPDATE (0x91) ------------|
   |<------ GOLD_UPDATE (0x92) -------------|
   |        (ENTITY_SPAWN x N cercanos)     |
   |             ... juego ...              |
   |                                        |
   |        [socket cerrado por cliente]    |
   |                                        | libera al jugador
```

> No hay un mensaje de "info de mapa" en el protocolo: el cliente carga la geometría
> directamente desde `config/*.toml` en disco (los mismos archivos que usa el servidor),
> no por red.

### 6.2 Creación de personaje

```
Cliente                                 Servidor
   |                                        |
   |--- CREATE_CHARACTER (0x02) ----------->|
   |                                        | valida nombre único
   |<-- CHARACTER_CREATED (0x82) -----------|  (mismo payload que LOGIN_OK)
   |<-- INVENTORY_UPDATE (0x90) ------------|  (con los ítems iniciales de la clase)
   |<-- EQUIP_UPDATE (0x91) ----------------|  (sin equipamiento)
   |<-- GOLD_UPDATE (0x92) ----------------- |  (oro inicial = 0)
```

### 6.3 Flujo de combate melee

```
Cliente A (atacante)                    Servidor                    Cliente B (objetivo)
   |                                        |                              |
   |------- ATTACK (0x04) ----------------->|                              |
   |                                        | valida rango, cooldown,      |
   |                                        | fair play (newbie/nivel)     |
   |                                        | calcula daño, crítico,       |
   |                                        | evasión y defensa            |
   |<------ DAMAGE_DEALT (0x89) ------------|                              |
   |<------ PLAYER_STATS (0x85) ------------|  (propio, post-ataque)       |
   |                                        |---- DAMAGE_RECEIVED (0x8A) ->|
   |                                        |     [o ATTACK_DODGED (0x8B) si esquivó]
   |                                        |                              |
   |                                        | si target murió:             |
   |                                        |---- ENTITY_DIED (0x8C) ----->|  (broadcast)
   |                                        |---- PLAYER_STATS (0x85) ---->|  (target, vida/exp post-muerte)
   |                                        |---- GOLD_UPDATE (0x92) ----->|  (si perdió oro)
   |                                        |---- INVENTORY_UPDATE (0x90)->|  (vacío, soltó todo)
   |<------ GOLD_UPDATE (0x92) -------------|  (si el atacante robó el oro en exceso)
```

El ataque a distancia (`CAST_SPELL`, 0x05) sigue exactamente el mismo flujo de eventos.

### 6.4 Muerte y resurrección

```
Cliente                                 Servidor
   |                                        |
   |  (HP llega a 0 durante el combate)     |
   |<------ ENTITY_DIED (0x8C) -------------|  (broadcast a todos en el mapa)
   |             ... jugador es fantasma,   |
   |             puede moverse pero no      |
   |             interactuar ...            |
   |                                        |
   |------- RESURRECT (0x0B) -------------->|
   |                                        | calcula demora segun
   |                                        | distancia al sanador
   |             ... espera ...             |
   |<------ PLAYER_RESPAWNED (0x8D) --------|  (hp_current = hp_max, posicion del sanador)
```

### 6.5 Equipar un item

```
Cliente                                 Servidor
   |                                        |
   |------- EQUIP_ITEM (0x08) ------------->|  (slot_index del inventario)
   |                                        | si es pocion: se consume y
   |                                        | aplica su efecto inmediato
   |                                        | si no: intercambia con lo
   |                                        | que estuviera en esa ranura
   |<------ EQUIP_UPDATE (0x91) ------------|
   |<------ INVENTORY_UPDATE (0x90) --------|
   |<------ PLAYER_STATS (0x85) ------------|  (stats derivados cambiaron)
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

### Class (PlayerClass)
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
0x04    CONSUMABLE         (solo se usa internamente; nunca es una ranura equipable real)
0xFF    NONE
```

### EntityType
```
0x00    PLAYER
0x01    NPC
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

> `TileType` está definido en `common/messages.h` pero no viaja por el protocolo binario
> (no hay un `TILE_*` opcode ni un campo que lo use en ningún mensaje): el mapa se carga
> desde `config/*.toml` en disco, no por red. Ver [8](#8-notas-de-implementación).

### LoginError
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

### ChatMsgType
```
0x00    SYSTEM              (mensaje del servidor: errores, confirmaciones, info)
0x01    PRIVATE             (mensaje privado @nick)
0x02    CLAN                (mensaje del chat de clan, /c)
0x03    SAY                 (chat general, sin prefijo)
```

### ClanNotifType
```
0x00    MEMBER_ONLINE       (un miembro del clan entró)
0x01    MEMBER_OFFLINE      (un miembro del clan salió)
0x02    MEMBER_ATTACKED     (un miembro del clan está siendo atacado)
0x03    JOIN_REQUEST        (alguien pidió unirse; solo lo recibe el fundador)
0x04    JOIN_ACCEPTED       (tu pedido fue aceptado)
0x05    JOIN_REJECTED       (tu pedido fue rechazado)
0x06    KICKED              (fuiste expulsado o baneado)
```

---

## 8. Notas de implementación

### Byte order
Todos los `uint16_t` y `uint32_t` deben ser convertidos al enviar (`htons`, `htonl`) y al recibir (`ntohs`, `ntohl`). Los `uint8_t` no necesitan conversión.

### Manejo de errores de protocolo
- Si el servidor recibe un opcode desconocido, tira una excepción (`"Unknown command opcode: N"`) que termina cerrando la conexión con ese cliente.
- Si el cliente recibe un opcode desconocido, tira una excepción (`"Unknown event opcode: N"`) análoga.

### Strings y encoding
- Longitud máxima de username: 20 caracteres.
- Longitud máxima de nombre de clan: 30 caracteres.
- Longitud máxima de mensaje de chat: 255 caracteres.
- Estos límites se validan en la capa de juego (`Game::handle_login`, etc), no en `Protocol`: el primitivo `send_str`/`recv_str` acepta cualquier longitud representable en `uint16_t`.

### Cheats
- Solo tienen efecto si el servidor tiene `cheats_enabled = true` en `server.toml` (`[server]`). Si está en `false`, el servidor recibe el comando normalmente pero el handler correspondiente no hace nada (`Game::process_command` devuelve un `CommandResult{}` vacío para los 12 `Cheat*Cmd`).

### Mensajes que no tienen un opcode dedicado

Algunos casos de uso no tienen su propio mensaje binario; se resuelven reutilizando otros ya
existentes:
- **Mensajes privados** (`@nick mensaje`): viajan como `SEND_CHAT` (`0x1E`), parseados por el
  servidor (`Game::handle_send_chat_msg`) y reenviados como `CHAT_MSG` tipo `PRIVATE`.
- **Confirmaciones y errores de transacción** (compra/venta/depósito/retiro) y **mensajes
  genéricos del servidor**: viajan como `CHAT_MSG` tipo `SYSTEM`, junto con el evento de
  estado correspondiente (`GOLD_UPDATE`, `INVENTORY_UPDATE`, `BANK_UPDATE`).
- **Info del mapa**: no se transmite — el cliente carga la geometría directamente de
  `config/*.toml` en disco, los mismos archivos que usa el servidor.
