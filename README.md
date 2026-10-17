# GroveCraft — Minecraft × GTA San Andreas

**0.1.0-alpha.1 · Windows · un jugador · MIT**

Mod experimental que ejecuta Minecraft Java junto a GTA San Andreas y dibuja sus
bloques, criaturas e interfaz dentro del mundo de GTA. CJ conserva los vehículos,
las misiones y los controles de San Andreas. **Esto es el mod, no los juegos completos.**

Incluye construcción y minería, inventario/crafteo/hornos, hambre, herramientas,
criaturas, agua, camas, recursos recuperables al morir, comercio y encargos,
equipaje y asistencia de carretera. El paisaje añade árboles y pequeñas
aldeas, granjas, campamentos, ruinas, puestos de saqueadores y paradas comerciales.
Las parcelas resueltas aparecen en el mapa desde el inicio; el detalle del follaje
se reduce a distancia. Consulta [los límites de esta alpha](KNOWN_ISSUES.md).

## Requisitos exactos

- Copias propias de **ambos juegos**. No incluye ejecutables, cuentas, recursos
  gráficos ni partidas de GTA/Minecraft.
- GTA San Andreas clásico **NewSteam R2 LV**, ejecutable `gta-sa.exe` SHA-256:
  `aa07fcdc8723a2831f77a35491dea6cdd58f6733e3ac69a10ddf5cdf95034fe3`.
  **No compatible de momento con 1.0, Definitive Edition, SA-MP o MTA.**
  El instalador rechaza otras versiones y nunca reemplaza el ejecutable.
- Minecraft Java **26.3**, Java **25**, Fabric Loader **0.19.5** y
  Fabric API **0.161.0+26.3**. Usa el inicio de sesión normal del launcher oficial.
- [Ultimate ASI Loader 9.7.4, x86](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/tag/v9.7.4),
  `dinput8.dll`, y [SilentPatchSA 34.1](https://github.com/CookiePLMonster/SilentPatch/releases/tag/1.1-BUILD34.1-SA).
- Python **3.12 o posterior** con `venv` y Git para preparar el mapa. El script instala NumPy 2.5.3
  en su propio entorno y obtiene el lector [DragonFF](https://github.com/Parik27/DragonFF)
  en una revisión fijada. Estos pasos requieren Internet y espacio para las herramientas.

No se ha establecido un mínimo fiable de RAM/GPU: se ejecutan los dos juegos a la vez.

## Instalar la descarga

1. Descarga **GroveCraft-0.1.0-alpha.1-windows.zip** de Releases y extráelo a una
   carpeta propia. El ZIP de código fuente de GitHub necesita compilación.
2. En el launcher oficial prepara una instalación Fabric 26.3 con un directorio
   de juego separado, por ejemplo `%APPDATA%\.minecraft-grovecraft`.
   Descarga [Fabric](https://fabricmc.net/use/installer/) y
   [Fabric API](https://modrinth.com/mod/fabric-api) de sus proyectos oficiales.
   Conserva el JAR de la versión exacta indicada arriba.
3. Extrae Ultimate ASI Loader y SilentPatchSA. Cierra los juegos y el launcher.
4. Abre PowerShell en la carpeta extraída y adapta estas rutas:

```powershell
.\Install-GroveCraft.ps1 `
  -GameDirectory 'D:\Juegos\GTA San Andreas' `
  -AsiLoaderPath 'D:\Descargas\UAL\dinput8.dll' `
  -SilentPatchDirectory 'D:\Descargas\SilentPatchSA' `
  -FabricApiJar 'D:\Descargas\fabric-api-0.161.0+26.3.jar'
```

Si necesitas elegir Python, añade `-PythonExecutable 'D:\Python\python.exe'`.
Puedes revisar primero los destinos con **`-PlanOnly`**, que no instala archivos.
La preparación inicial del mapa puede tardar; después se reutiliza la caché.
No cierres la consola hasta que termine. Si Windows bloquea un archivo, consulta
el aviso y su procedencia; estos scripts no cambian políticas ni exclusiones.

El instalador verifica las versiones, conserva los archivos existentes compatibles
y se niega a reemplazar archivos distintos. Para este ejecutable aplica localmente
un ajuste de patrón a SilentPatchSA 34.1; no es una versión oficial de SilentPatch.
Si ya tenías la edición original exacta, la respalda y la restaura al desinstalar.
Crea una copia local de las partidas
y ajustes de GTA antes de copiar el mod. No instala otros coches ni modifica
los archivos de tráfico originales en esta primera descarga.

## Jugar y guardar

En los argumentos JVM de esa instalación Fabric añade:

```text
--enable-native-access=ALL-UNNAMED
```

Inicia el perfil de Minecraft y después GTA normalmente desde su carpeta.
El guest espera la conexión. No abras dos clientes del puente a la vez.
La primera campaña crea su propio mundo Minecraft; el menú de inventario y
los bloques se activan al llegar a una situación de juego compatible.

**Guarda desde GTA:** el puente solicita el guardado de ambos lados. Conserva
también `%LOCALAPPDATA%\GroveCraft\campaigns` y los `saves` del perfil Fabric:
copiar sólo la ranura GTA no transporta el mundo asociado. No desinstales el mod
en medio de una operación de guardado. F6 es un guardado adicional de Minecraft.
El perfil público empieza en supervivencia con una guía y tres panes, sin kit creativo.

| Control | Acción |
|---|---|
| 1–9 / rueda | Seleccionar Minecraft |
| Clic izquierdo mantenido | Minar; pulsar para atacar |
| Clic derecho | Colocar, interactuar o comer |
| E / I | Inventario |
| R | Comer el alimento seleccionado |
| G junto a la cajuela | Equipaje |
| Alt+G | Taller móvil con mesa guardada |
| Alt+B | Asistencia de carretera |
| Escape | Cerrar contenedor; después pausa GTA |

Las interacciones de Minecraft tienen restricciones durante misiones, vehículos,
interiores y menús para que no se mezclen acciones incompatibles.
No abras manualmente el mundo espejo como una partida independiente de Minecraft.

## Desinstalar y diagnosticar

Cierra ambos juegos y ejecuta `Uninstall-GroveCraft.ps1 -VerifyOnly` para revisar,
o `Uninstall-GroveCraft.ps1` para retirar los archivos creados por el instalador.
Los archivos modificados externamente se conservan y requieren revisión.
Partidas, mundos, respaldos y caché **no se borran**.

Si no conecta, verifica los SHA y versiones, el directorio Fabric, Fabric API y
el argumento JVM. Registros: `GroveCraft.log` junto al ejecutable de GTA y
`logs/latest.log` en el perfil Minecraft. Al reportar un error comparte versión,
pasos y fragmentos relevantes; no subas partidas, tokens, cuentas ni la caché del mapa.

## Código y créditos

Consulta [BUILDING.md](BUILDING.md), [CREDITS.md](CREDITS.md), [CHANGELOG.md](CHANGELOG.md)
y [LICENSE](LICENSE). Proyecto de [Axel12Q](https://github.com/Axel12Q), desarrollado
con asistencia de Codex (GPT-6). GroveCraft no es un producto oficial de Rockstar,
Mojang o Microsoft. Las marcas y los juegos pertenecen a sus respectivos titulares.
