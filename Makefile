# Makefile para MazeRay - Un juego estilo Wolfenstein 3D que cabe en un disquete

# Compilador
CC = gcc

# Banderas de compilación (optimizadas para tamaño)
CFLAGS = -Wall -Os -flto -ffunction-sections -fdata-sections

# Banderas del enlazador (optimizadas para tamaño)
LDFLAGS = -flto -Wl,--gc-sections

# Sistema operativo
ifeq ($(OS),Windows_NT)
	# Configuración para Windows
	EXE_EXT = .exe
	RAYLIB_PATH = C:/raylib
	INCLUDE_PATHS = -I$(RAYLIB_PATH)/include
	LDLIBS = -L$(RAYLIB_PATH)/lib -lraylib -lopengl32 -lgdi32 -lwinmm
	RM = del /q
	PLATFORM = PLATFORM_DESKTOP
	EXECUTABLE = build/MazeRay$(EXE_EXT)
else
	# Configuración para Linux/macOS
	EXE_EXT =
	RAYLIB_PATH = /usr/local
	INCLUDE_PATHS = -I$(RAYLIB_PATH)/include
	LDLIBS = -L$(RAYLIB_PATH)/lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
	RM = rm -f
	PLATFORM = PLATFORM_DESKTOP
	EXECUTABLE = build/MazeRay$(EXE_EXT)
endif

# Directorios
SRC_DIR = src
BUILD_DIR = build
FLOPPY_DIR = floppy_contents

# Archivos fuente
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/game.c $(SRC_DIR)/maze.c $(SRC_DIR)/utils.c
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))

# Reglas
.PHONY: all floppy clean

all: $(EXECUTABLE)

# Crear directorio build si no existe
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compilar archivos fuente
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $(INCLUDE_PATHS) -D$(PLATFORM) $< -o $@

# Enlazar para crear el ejecutable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXECUTABLE) $(LDFLAGS) $(LDLIBS)

# Comprimir ejecutable con UPX (si está disponible)
	@if command -v upx &> /dev/null; then \
		echo "Comprimiendo ejecutable con UPX..."; \
		upx -9 $(EXECUTABLE); \
	else \
		echo "UPX no encontrado, saltando compresión..."; \
	fi

# Crear carpeta de contenido para disquete
floppy: $(EXECUTABLE)
	@echo "Preparando contenido para disquete..."
	mkdir -p $(FLOPPY_DIR)
	cp $(EXECUTABLE) $(FLOPPY_DIR)/
	cp README.txt $(FLOPPY_DIR)/
	
	@echo "Verificando tamaño..."
	@du -sh $(FLOPPY_DIR)
	@echo "Límite recomendado: 1.44MB"

# Limpiar archivos compilados
clean:
	$(RM) $(BUILD_DIR)/*$(EXE_EXT) $(BUILD_DIR)/*.o
	$(RM) $(FLOPPY_DIR)/*$(EXE_EXT)