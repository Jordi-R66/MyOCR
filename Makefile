CC = gcc
CFLAGS = -O3 -Wall -Wextra -Iinclude -Iexternal/myOwnAILib/external/myOwnCLib -Iexternal/myOwnAILib/src
LDFLAGS = -lm

# --- Dossiers de destination ---
BUILD_DIR = build
LIB_DIR   = lib
BIN_DIR   = bin
ASM_DIR   = asm

# --- Sources ---

AI_SRCS = external/myOwnAILib/src/network.c \
		  external/myOwnAILib/src/layer.c

CLIB_SRCS = external/myOwnCLib/maths/matrices/matrix.c \
			external/myOwnCLib/maths/matrices/mlMatrix.c \
			external/myOwnCLib/maths/vectors/vectors.c

MAIN_SRCS = src/main.c

# --- Objets (.o) ---
AI_OBJS   = $(patsubst %.c, $(BUILD_DIR)/%.o, $(AI_SRCS))
CLIB_OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(CLIB_SRCS))
MAIN_OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(MAIN_SRCS))

# --- Assembleur (.s) ---
# On crée la liste des fichiers .s attendus dans le dossier asm/
AI_ASMS   = $(patsubst %.c, $(ASM_DIR)/%.s, $(AI_SRCS))
CLIB_ASMS = $(patsubst %.c, $(ASM_DIR)/%.s, $(CLIB_SRCS))
MAIN_ASMS = $(patsubst %.c, $(ASM_DIR)/%.s, $(MAIN_SRCS))

# On regroupe tout pour la cible "make asm"
ALL_ASMS  = $(AI_ASMS) $(CLIB_ASMS) $(MAIN_ASMS)

# --- Noms des fichiers finaux ---
EXEC      = $(BIN_DIR)/ocr_app
LIB_CLIB  = $(LIB_DIR)/libmyownclib.a
LIB_AILIB = $(LIB_DIR)/libmyownailib.a

# --- Cibles ---

.PHONY: all clean asm directories

all: directories $(EXEC)

# Nouvelle cible pour générer uniquement l'assembleur
asm: directories $(ALL_ASMS)
	@echo "--- Génération des fichiers assembleur terminée dans $(ASM_DIR)/ ---"

# --- Règles de Linkage ---

$(EXEC): $(MAIN_OBJS) $(LIB_AILIB) $(LIB_CLIB)
	@mkdir -p $(BIN_DIR)
	$(CC) $(MAIN_OBJS) $(LIB_AILIB) $(LIB_CLIB) $(LDFLAGS) -o $@
	@echo "--- Exécutable créé : $@ ---"

$(LIB_CLIB): $(CLIB_OBJS)
	@mkdir -p $(LIB_DIR)
	ar rcs $@ $^

$(LIB_AILIB): $(AI_OBJS)
	@mkdir -p $(LIB_DIR)
	ar rcs $@ $^

# --- Règles de Compilation ---

# 1. Règle .c -> .o (Objet)
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 2. Règle .c -> .s (Assembleur)
# Le flag -S arrête la compilation après la génération de l'assembleur
# On garde $(CFLAGS) pour que l'assembleur reflète bien les optimisations (-O3)
$(ASM_DIR)/%.s: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -S $< -o $@

directories:
	@mkdir -p $(BUILD_DIR) $(LIB_DIR) $(BIN_DIR) $(ASM_DIR)

clean:
	rm -rf $(BUILD_DIR) $(LIB_DIR) $(BIN_DIR) $(ASM_DIR)
	@echo "--- Nettoyage complet terminé ---"