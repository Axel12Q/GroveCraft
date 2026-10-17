# Validación de 0.1.0-alpha.1 — 2026-10-16

Calificación para una **alpha experimental**, no una versión estable.

- Compilación desde el código público regenerado: ASI idéntica byte a byte;
  mismas clases y recursos del JAR. Loom puede variar su índice informativo
  del manifiesto entre compilaciones; no se promete identidad binaria del JAR.
- Java: 166 pruebas, 165 aprobadas,
  1 omitida, ninguna fallida. Prueba CPU de retención COM aprobada.
- ASI realmente cargada en un proceso D3D9 de prueba: profundidad, restauración
  del estado, Reset y sustitución de dispositivo aprobados. El candidato anterior
  fallaba la sustitución; se conserva esa regresión como evidencia.
- GTA real y cliente Minecraft de desarrollo: arranque, escena exterior,
  dos ciclos minimizar/restaurar, imagen de CJ/árboles/mobs/HUD/radar inspeccionada
  y cierre final normal con guardado del guest. No se observaron fallos de
  colisión en esa escena. Las repeticiones previas fallaron y no se ocultan.
- Instalador/desinstalador en carpetas aisladas: hashes, vista previa sin
  cambios, rechazo de duplicados, archivos alterados preservados, conversión y
  restauración de SilentPatch original, dependencias previas y partidas conservadas.
- Conversor del mapa ejecutado desde el script público con una instalación propia.
- Revisión de publicación: fuente y descarga sin fallos ni avisos del linter;
  sin juegos, credenciales, mundos ni historial privado en los paquetes.
- Auditoría local: 72 mundos protegidos y
  3447 archivos, partidas GTA y ejecutable original intactos.

Pendiente: instalación completa desde el launcher oficial en otro equipo,
sesiones largas al recuperar foco, campaña completa, rendimiento en otros
equipos y el resto de límites de KNOWN_ISSUES.md. Las pruebas cortas finales
no demuestran que todos los cierres intermitentes anteriores hayan desaparecido.
La instalación habitual del desarrollador no se reemplazó con el candidato.

SHA-256 ASI: `4ab39ded141427f032b69304c9f960e5cd0c093dbcfa079f112ae3536ab6bb14`

SHA-256 guest: `ea1d4f0cb9b621e3f365709579c45bfd30732a68d15ee7e6cf52bbcd23db6188`

