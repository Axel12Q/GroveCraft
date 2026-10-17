# Changelog

## 0.1.0-alpha.1

Primera preparación de descarga pública: puente GTA SA/Minecraft, mecánicas de
supervivencia, construcción, criaturas, comercio y campaña con guardado coordinado.
Incluye destinos iniciales en el mapa, casas nuevas con tejados y LOD de follaje.

Retención explícita del dispositivo D3D9 entre fotogramas y liberación en Reset.
Las texturas se recrean al cambiar de dispositivo, evitando reutilizar recursos
del vídeo de introducción en el renderizado del juego. La prueba sintética cubre
Reset, sustitución de dispositivo, profundidad y restauración del estado gráfico.
La generación del código desde cero activa los mismos sistemas que el desarrollo.
Configuración compartida para un perfil Fabric normal, conversión local del mapa,
instalación con comprobación de hashes, respaldo y desinstalación conservando partidas.
Consulta KNOWN_ISSUES.md: no representa una certificación de campaña completa.
