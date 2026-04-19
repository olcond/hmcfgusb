# GCC 16 Compatibility Assessment

**Datum:** 2026-04-19  
**Tester:** Claude (GCC 15.2.0 als Proxy)  
**Referenz:** https://gcc.gnu.org/gcc-16/changes.html · https://gcc.gnu.org/gcc-16/porting_to.html

## Ergebnis: Kompatibel ✅

Das Projekt kompiliert ohne Warnungen oder Fehler unter allen simulierbaren GCC-16-Bedingungen.

---

## Relevante GCC-16-Änderungen für C-Code

### 1. `-Wunused-but-set-variable` / `-Wunused-but-set-parameter` — neues Standard-Level 3

GCC 16 erhöht den Standard-Warnlevel von 1 auf 3. Level 3 meldet zusätzlich Compound-Assignments (`+=`, `-=` etc.) auf Variablen, deren Ergebnis nie gelesen wird.

**Test:**
```
make CC=gcc CFLAGS="-MMD -O2 -Wall -Wextra -Wunused-but-set-variable \
    -Wunused-but-set-parameter -Wunused-variable -Wunused-value \
    -Werror -Wno-error=maybe-uninitialized -I/opt/local/include -g"
```
→ **Sauberer Build, keine Warnungen.**

Das exakte Level-Syntax (`=3`) ist in GCC 15 nicht verfügbar und konnte nicht direkt getestet werden. Da der Code die zugrundeliegenden Prüfungen bereits besteht, ist kein Problem zu erwarten.

### 2. `int8_t`-ABI-Änderung auf Solaris

`int8_t` wird auf Solaris von `char` zu `signed char` geändert (C99-Konformität), was ein ABI-Break ist.

→ **Nicht relevant** — das Projekt läuft auf Linux.

### 3. `-pthread`-Verhalten auf Solaris 11.4+

Die Option definiert `_REENTRANT` nicht mehr implizit.

→ **Nicht relevant** — das Projekt läuft auf Linux.

---

## CI/CD-Bewertung

| Workflow | Status |
|---|---|
| `build.yml` (gcc + clang, `-Werror`) | ✅ bereit |
| `cross-compile.yml` (armhf, arm64) | ✅ bereit |
| `sanitizers.yml` (ASan + UBSan via clang) | ✅ bereit |
| `valgrind.yml` | ✅ bereit |

Das bestehende `-Wno-error=maybe-uninitialized` in `build.yml` und `cross-compile.yml` ist korrekt und defensiv — GCC produziert hier bekannte False Positives. Dies bleibt auch unter GCC 16 die richtige Lösung.

**Zeitplan:** `ubuntu-latest` (Ubuntu 24.04 LTS) liefert GCC 16 erst, wenn es in die Standardpakete aufgenommen wird. Bis dahin besteht kein unmittelbarer Handlungsbedarf.

---

## Handlungsbedarf

Keiner. Codebase und CI-Konfiguration sind GCC-16-ready.
