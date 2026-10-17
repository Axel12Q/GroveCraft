# Compilar

Requisitos: Windows, Git, Python, Java 25 y LLVM-MinGW con el compilador
`i686-w64-mingw32-clang++`. La ASI es **x86**, aunque Windows sea de 64 bits.

```powershell
.\Build-GroveCraft.ps1 `
  -CompilerDirectory 'D:\Herramientas\llvm-mingw\bin' `
  -JavaHome 'D:\Herramientas\jdk-25'
```

El script obtiene LibertyCraft y MinHook en las revisiones registradas en
`dependencies.json`, genera `build/guest`, aplica los parches propios y ejecuta
las pruebas Java. No sobrescribe mundos ni instalaciones. Las dependencias
modificadas o con una revisión diferente se rechazan. `-Offline` sólo funciona
con las fuentes y dependencias Gradle ya disponibles.

Resultados: `build/host/GroveCraft.asi` y
`build/guest/build/libs/grovecraft-guest-0.1.0-alpha.1.jar`.
La compilación nativa de publicación elimina las rutas de origen de los macros
de archivo. La distribución no incluye el compilador, el JDK ni los juegos.
En la revisión comprobada, la ASI reconstruida es idéntica byte a byte; las
clases y recursos del JAR coinciden. Loom puede omitir en su primera compilación
el índice informativo `Fabric-Loom-Client-Only-Entries` del manifiesto, por lo
que no se promete que dos ZIP/JAR tengan siempre el mismo hash.

Las pruebas Java no prueban por sí solas el gameplay. La prueba sintética D3D9
requiere compilar `src/render_smoke.cpp` como `build/host/render-smoke.exe` y
ejecutar `python tools/test_transport.py` con ambos juegos cerrados. Registra
el hash de la ASI realmente probada. Para publicar una revisión nueva, conserva
ese recibo y vuelve a comprobar el juego real, guardado, inicio y recuperación de foco.

No añadas a Git `build`, `runtime`, `vendor`, `.env`, mundos, grabaciones privadas,
cachés del mapa, recursos extraídos o archivos originales de los juegos.
