# Límites de 0.1.0-alpha.1

Esta versión es una alpha para pruebas; no se ha completado la campaña entera de GTA.
Las pruebas de campo locales usan el cliente de desarrollo. El instalador y la
configuración del perfil Fabric normal se verificaron por separado; aún falta
una prueba completa desde el launcher oficial en un equipo limpio.

- Se observaron cierres intermitentes al iniciar o recuperar el foco en sesiones
  de desarrollo. Se corrigió una referencia de dispositivo D3D9 sin retención;
  esa corrección por sí sola no demuestra que todos los cierres estén resueltos.
  El historial de validación de esta descarga describe lo comprobado.
- El LOD actual simplifica regiones completas de hojas lejanas. No es un sistema
  de geometría de kilómetros, y todavía pueden aparecer elementos al viajar rápido.
  Falta verificar rutas largas y todos los cruces de distancia en distintos equipos.
- Los marcadores iniciales representan parcelas resueltas por el atlas del mundo.
  Obstáculos o construcciones añadidas pueden impedir su construcción definitiva.
  Se muestran hasta 64 destinos, conservando espacio para las misiones GTA.
- La minería de geometría nativa, explosiones y reconstrucción de fachadas siguen
  siendo experimentales. Se recomienda empezar una campaña de prueba y conservar
  el respaldo; no se promete que cualquier edificio pueda excavarse sin defectos.
- La ropa y armadura pueden atravesar el cuerpo en algunas animaciones. La
  calibración privada del equipo de desarrollo no se redistribuye con esta descarga;
  el ajuste visual de respaldo tiene limitaciones.
- Las nuevas casas mejoradas sólo se generan en parcelas nuevas. No se reemplazan
  casas que ya están guardadas ni construcciones del jugador.
- El equipaje es personal por mundo, compartido entre vehículos compatibles.
  Los objetos visibles en la cajuela representan su contenido, no cada material apilado.
- No se verificó compatibilidad con paquetes grandes de mods, mapas reemplazados,
  otros plugins que modifiquen cámara/render/guardado, ni otras versiones de GTA.
- No incluye el pack opcional de variedad de tráfico probado en desarrollo, minas
  nuevas de gran tamaño, sombras nativas completas para cada bloque ni multijugador.

No se deben sustituir estos límites por una promesa de estabilidad o de FPS.
