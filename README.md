# mis-plugins

Plugins de audio propios para Logic Pro (Audio Unit) y otros DAW (VST3).

| Plugin | Qué es |
| --- | --- |
| **Calor** | Distorsión multietapa: 3 etapas con 8 tipos de saturación, modo serie o por bandas, pad XY, LFO, envolvente y presets. |

## Descargar

Ve a **Releases** (columna derecha de esta página), abre la versión más reciente
y descarga `Calor-Mac.zip`. Dentro está `LEEME.txt` con los pasos de instalación.

Cada vez que se sube un cambio, GitHub compila los plugins automáticamente
(pestaña **Actions**) y publica una nueva versión en Releases.

## Compilar a mano

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Hecho con [JUCE](https://juce.com) (AGPLv3 / licencia JUCE).
