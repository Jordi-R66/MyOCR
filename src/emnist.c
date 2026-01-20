#include "emnist.h"
#include <maths/vectors/vectors.h> 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void load_mapping(const char* filename, char mapping_array[NUM_CLASSES]) {
	FILE* fp = fopen(filename, "r");
	if (!fp) {
		perror("Erreur : Impossible d'ouvrir le fichier de mapping");
		exit(1);
	}

	int label;
	int ascii_code;

	// On lit ligne par ligne : un entier, espace, un entier
	while (fscanf(fp, "%d %d", &label, &ascii_code) != EOF) {
		// Sécurité : on vérifie qu'on ne sort pas du tableau
		if (label >= 0 && label < NUM_CLASSES) {
			// En C, un 'char' est juste un petit entier. 
			// On peut stocker 65 directement, ça vaudra 'A'.
			mapping_array[label] = (char)ascii_code;
		}
	}

	fclose(fp);
	printf("Mapping chargé !\n");
}

// Fonction pour parser une ligne du CSV et remplir la struct
int parse_csv_line(char* line, EmnistPtr img) {
	char* token;

	// 1. Récupérer le label (premier nombre)
	token = strtok(line, ",");
	if (token == NULL) return 0; // Ligne vide ou erreur
	img->label = (unsigned char)atoi(token);

	// 2. Récupérer les 784 pixels
	int pixel_index = 0;
	while (pixel_index < PIXEL_COUNT) {
		token = strtok(NULL, ",");
		if (token == NULL) break;

		unsigned char val = (unsigned char)atoi(token);

		// Conversion du vecteur 1D vers matrice 2D
		int r = pixel_index / IMG_SIZE;
		int c = pixel_index % IMG_SIZE;

		img->pixels[r][c] = val;
		pixel_index++;
	}
	return 1; // Succès
}

void fix_emnist_orientation(EmnistPtr src, EmnistPtr dest) {
	dest->label = src->label;
	for (int i = 0; i < IMG_SIZE; i++) {
		for (int j = 0; j < IMG_SIZE; j++) {
			// Transposition : ligne i, col j devient ligne j, col i
			dest->pixels[j][i] = src->pixels[i][j];
		}
	}
}

void print_ascii_art(EmnistPtr img) {
	printf("Label: %d (%c)\n", img->label, img->label);
	for (int i = 0; i < IMG_SIZE; i++) {
		for (int j = 0; j < IMG_SIZE; j++) {
			// Si le pixel est > 128 (foncé), on met un #, sinon un point
			printf("%c ", img->pixels[i][j] > 100 ? '#' : '.');
		}
		printf("\n");
	}
	printf("\n");
}

void save_as_bmp(uint imgNumber, EmnistPtr img) {
	FILE* f;
	int w = IMG_SIZE;
	int h = IMG_SIZE;

	char* filename = calloc(256, sizeof(char));
	snprintf(filename, 256, "emnist_%c_%u.bmp", img->label, imgNumber);

	// En BMP, chaque ligne doit être un multiple de 4 octets (padding).
	// Pour 28 pixels * 3 octets (RGB) = 84 octets. 84 est divisible par 4.
	// Donc padding = 0. C'est pratique !
	int padding = (4 - (w * 3) % 4) % 4;
	int filesize = 54 + (3 * w + padding) * h; // 54 = taille des headers

	// 1. Définition des headers (tableaux d'octets bruts)
	unsigned char bmpfileheader[14] = {
		'B','M',      // Magic number
		0,0,0,0,      // Taille du fichier (sera rempli plus bas)
		0,0,          // Reserved
		0,0,          // Reserved
		54,0,0,0      // Offset des données (toujours 54 pour du 24-bit standard)
	};

	unsigned char bmpinfoheader[40] = {
		40,0,0,0,     // Taille du header info
		0,0,0,0,      // Largeur (width)
		0,0,0,0,      // Hauteur (height)
		1,0,          // Nombre de plans (toujours 1)
		24,0,         // Bit count (24 pour RGB)
		0,0,0,0,      // Compression (0 = aucune)
		0,0,0,0,      // Taille de l'image (peut rester 0 si pas de compression)
		0,0,0,0,      // X pixels per meter
		0,0,0,0,      // Y pixels per meter
		0,0,0,0,      // Colors used
		0,0,0,0       // Important colors
	};

	// Remplissage des valeurs dynamiques dans les tableaux
	// On utilise le decalage de bit (shift) pour mettre les entiers en Little Endian

	// File Size
	bmpfileheader[2] = (unsigned char)(filesize);
	bmpfileheader[3] = (unsigned char)(filesize >> 8);
	bmpfileheader[4] = (unsigned char)(filesize >> 16);
	bmpfileheader[5] = (unsigned char)(filesize >> 24);

	// Width
	bmpinfoheader[4] = (unsigned char)(w);
	bmpinfoheader[5] = (unsigned char)(w >> 8);
	bmpinfoheader[6] = (unsigned char)(w >> 16);
	bmpinfoheader[7] = (unsigned char)(w >> 24);

	// Height
	bmpinfoheader[8] = (unsigned char)(h);
	bmpinfoheader[9] = (unsigned char)(h >> 8);
	bmpinfoheader[10] = (unsigned char)(h >> 16);
	bmpinfoheader[11] = (unsigned char)(h >> 24);

	// 2. Écriture du fichier
	f = fopen(filename, "wb");
	if (!f) {
		free(filename);
		perror("Erreur création fichier BMP");
		return;
	}

	fwrite(bmpfileheader, 1, 14, f);
	fwrite(bmpinfoheader, 1, 40, f);

	// 3. Écriture des pixels
	// ATTENTION : Le format BMP stocke les lignes de BAS en HAUT (Bottom-Up).
	// Il faut donc lire notre matrice 'img->pixels' à l'envers sur l'axe Y.
	for (int i = h - 1; i >= 0; i--) {
		for (int j = 0; j < w; j++) {
			// EMNIST est en noir et blanc (0-255).
			// Pour faire du gris en RGB, R = G = B = valeur du pixel.
			// Note: BMP écrit en ordre BGR (Blue, Green, Red).
			unsigned char color = img->pixels[i][j];

			// Inversion des couleurs ? 
			// Souvent EMNIST est : 0=Fond noir, 255=Texte blanc.
			// Si vous voulez un rendu "papier" (fond blanc, texte noir), faites :
			// color = 255 - color; 

			unsigned char pixel[3] = { color, color, color }; // B, G, R
			fwrite(pixel, 1, 3, f);
		}
		// Écriture du padding (s'il y en avait, ici 0)
		unsigned char bmppad[3] = { 0,0,0 };
		fwrite(bmppad, 1, padding, f);
	}

	fclose(f);
	printf("Image sauvegardee : %s\n", filename);
	free(filename);
}

VectorPtr load_bmp_image(const char* filename) {
	FILE* f = fopen(filename, "rb");
	if (!f) {
		perror("Impossible d'ouvrir l'image BMP");
		return NULL;
	}

	unsigned char header[54];
	if (fread(header, 1, 54, f) != 54) {
		fprintf(stderr, "Erreur: Fichier invalide ou trop petit.\n");
		fclose(f);
		return NULL;
	}

	if (header[0] != 'B' || header[1] != 'M') {
		fprintf(stderr, "Erreur: Ce n'est pas un fichier BMP.\n");
		fclose(f);
		return NULL;
	}

	int w = *(int*)&header[18];
	int h = *(int*)&header[22];

	if (w != 28 || h != 28) {
		fprintf(stderr, "Erreur: L'image doit faire 28x28 pixels (actuel: %dx%d).\n", w, h);
		fclose(f);
		return NULL;
	}

	// Allocation du vecteur (Note: on utilise malloc pour que le vecteur survive au retour de fonction)
	VectorPtr input = (VectorPtr)malloc(sizeof(Vector));
	*input = createVector(w * h);

	int padding = (4 - (w * 3) % 4) % 4;
	unsigned char pixel[3];

	// Position au début des données pixels (Offset standard 54 ou lu dans le header)
	fseek(f, *(int*)&header[10], SEEK_SET);

	// Lecture : Le BMP stocke souvent l'image à l'envers (Bas -> Haut)
	for (int y = h - 1; y >= 0; y--) {
		for (int x = 0; x < w; x++) {
			fread(pixel, 1, 3, f); // B, G, R

			// 1. Conversion Niveaux de gris (Moyenne)
			double gray = (pixel[0] + pixel[1] + pixel[2]) / 3.0;

			// 2. Normalisation et Inversion
			// Paint : Fond Blanc (255) / Texte Noir (0)
			// IA EMNIST : Fond Noir (0.0) / Texte Blanc (1.0)
			double normalized = (255.0 - gray) / 255.0;

			input->data[y * w + x] = normalized;
		}
		fseek(f, padding, SEEK_CUR);
	}

	fclose(f);
	return input;
}

Dataset load_dataset_in_memory(const char* csv_path) {
	Dataset ds = { 0, NULL };
	FILE* fp = fopen(csv_path, "r");
	if (!fp) { perror("Erreur ouverture CSV"); return ds; }

	// Estimation ou comptage préalable (EMNIST Balanced train ~112800 lignes)
	// On alloue large pour être sûr ou on utilise realloc. Ici on fixe pour l'exemple.
	int max_samples = 200000;
	ds.images = (EmnistImage*)calloc(max_samples, EMNIST_SIZE);

	char buffer[4096];
	int i = 0;
	while (fgets(buffer, sizeof(buffer), fp) && i < max_samples) {
		if (parse_csv_line(buffer, &ds.images[i])) {
			// On peut aussi faire la correction d'orientation ICI une fois pour toutes
			EmnistImage fixed;
			fix_emnist_orientation(&ds.images[i], &fixed);
			ds.images[i] = fixed;
			i++;
		}
	}

	ds.count = i;
	ds.images = (EmnistImage*)realloc(ds.images, ds.count * EMNIST_SIZE);
	fclose(fp);
	printf("Dataset chargé en RAM : %d images.\n", ds.count);

	return ds;
}

/*
int fake_main() {
	uint compteur = 0;

	char ascii_map[NUM_CLASSES];
	load_mapping("datasets/emnist/emnist-balanced-mapping.txt", ascii_map);

	// 1. Ouvrir le CSV
	FILE* fp = fopen("datasets/emnist/emnist-balanced-train.csv", "r");
	if (!fp) {
		perror("Impossible d'ouvrir le fichier CSV");
		return 1;
	}

	char buffer[4096];
	char filename[64]; // Buffer pour stocker le nom du fichier de sortie (ex: "img_12.bmp")

	EmnistImage raw_img, fixed_img;

	printf("Début du traitement...\n");

	// 2. La boucle fgets lit ligne par ligne jusqu'à la fin du fichier
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		compteur++;

		// Parsing et correction
		if (parse_csv_line(buffer, &raw_img)) {
			fix_emnist_orientation(&raw_img, &fixed_img);

			// --- OPTION A : Sauvegarder TOUT (Déconseillé pour le test) ---
			// sprintf(filename, "img_%u_label_%d.bmp", compteur, fixed_img.label);
			// save_as_bmp(filename, &fixed_img);

			// --- OPTION B : Sauvegarder seulement les 20 premières pour vérifier ---
			if (compteur <= 20) {
				printf("Sauvegarde image %u (Label: %d)\n", compteur, fixed_img.label);

				// On crée un nom de fichier dynamique : "img_0.bmp", "img_1.bmp"...
				//sprintf(filename, "img_%u_label_%d.bmp", compteur, fixed_img.label);
				fixed_img.label = ascii_map[fixed_img.label];
				save_as_bmp(compteur, &fixed_img);

				// Afficher l'ASCII art seulement pour les premières aussi
				// print_ascii_art(&fixed_img);
			}

			// Si on veut juste entraîner le modèle plus tard, on ne sauvegardera pas en BMP,
			// on stockera les données en RAM ou dans un format binaire optimisé.
		}

		// Petit log pour savoir où on en est (toutes les 5000 images)
		if (compteur % 5000 == 0) {
			printf("Images traitées : %u\n", compteur);
		}
	}

	printf("Terminé ! Total images lues : %u\n", compteur);
	fclose(fp);
	return 0;
}
*/