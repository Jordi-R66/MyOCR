#include <stdio.h>
#include <stdlib.h> // Pour malloc/free
#include <time.h>   // Pour clock() et CLOCKS_PER_SEC
#include <network.h>
#include "emnist.h"

// --- VOS FONCTIONS EXISTANTES (INCHANGÉES) ---

NeuralNet creer_reseau_ocr() {
	NeuralNet net = createNeuralNet(PIXEL_COUNT);

	neuralNetAddLayer(&net, 256, ACTIVATION_RELU);
	//neuralNetAddLayer(&net, PIXEL_COUNT + 1, ACTIVATION_RELU);
	//neuralNetAddLayer(&net, PIXEL_COUNT + 1, ACTIVATION_RELU);
	neuralNetAddLayer(&net, 47, ACTIVATION_SOFTMAX);

	return net;
}

void train_one_epoch_from_ram(NeuralNetPtr net, DatasetPtr ds, double learning_rate) {
	Vector inputVec = createVector(PIXEL_COUNT);
	Vector targetVec = createVector(47);

	for (int i = 0; i < ds->count; i++) {
		// 1. Input (Normalisation à la volée, rapide)
		// Astuce d'optimisation : diviser par 255.0 est lent. Multiplier par (1/255.0) est plus rapide.
		double inv_255 = 1.0 / 255.0;
		for (int p = 0; p < PIXEL_COUNT; p++) {
			// Accès linéaire au tableau 2D via cast
			unsigned char val = ((unsigned char*)ds->images[i].pixels)[p];
			inputVec.data[p] = (double)val * inv_255;
		}

		// 2. Target
		for (int k = 0; k < 47; k++) targetVec.data[k] = 0.0;
		if (ds->images[i].label < 47) targetVec.data[ds->images[i].label] = 1.0;

		// 3. Apprentissage
		neuralNetForward(net, &inputVec);
		neuralNetBackward(net, &targetVec);
		neuralNetUpdate(net, learning_rate);

		//if ((i + 1) % 10000 == 0) printf("  -> %d / %d images\r", i + 1, ds->count);
	}
	printf("\n");
	deallocVector(&inputVec);
	deallocVector(&targetVec);
}

int predict_character(NeuralNet* net, double pixels[784]) {
	Vector inputVec = createVector(784);
	for (int i = 0; i < 784; i++) inputVec.data[i] = pixels[i];

	VectorPtr output = neuralNetForward(net, &inputVec);

	int best_label = 0;
	double max_prob = -1.0;
	for (uint i = 0; i < output->size; i++) {
		if (output->data[i] > max_prob) {
			max_prob = output->data[i];
			best_label = i;
		}
	}

	deallocVector(&inputVec);
	return best_label;
}

// --- NOUVELLE FONCTION : ÉVALUATION ---

void evaluate_network(NeuralNetPtr net, const char* csv_path) {
	printf("\n--- EVALUATION SUR %s ---\n", csv_path);
	FILE* fp = fopen(csv_path, "r");
	if (!fp) {
		perror("Impossible d'ouvrir le fichier de test");
		return;
	}

	char buffer[4096];
	EmnistImage raw_img, img_data;
	Vector inputVec = createVector(PIXEL_COUNT);

	int total = 0;
	int correct = 0;

	while (fgets(buffer, sizeof(buffer), fp)) {
		if (!parse_csv_line(buffer, &raw_img)) continue;
		fix_emnist_orientation(&raw_img, &img_data);

		// Préparation Input
		for (int i = 0; i < PIXEL_COUNT; i++) {
			inputVec.data[i] = (Value)(img_data.pixels[i / 28][i % 28]) / 255.0;
		}

		// Forward uniquement
		VectorPtr output = neuralNetForward(net, &inputVec);

		// Argmax
		int predicted = 0;
		double max_prob = -1.0;
		for (uint i = 0; i < output->size; i++) {
			if (output->data[i] > max_prob) {
				max_prob = output->data[i];
				predicted = i;
			}
		}

		if (predicted == img_data.label) correct++;
		total++;

		if (total % 2000 == 0) printf("Test: %d images... (Precision: %.2f%%)\r", total, (double)correct / total * 100.0);
	}

	printf("\nRESULTAT FINAL : %d / %d corrects.\n", correct, total);
	printf("PRECISION      : %.2f%%\n", (double)correct / total * 100.0);

	deallocVector(&inputVec);
	fclose(fp);
}

// --- MAIN MODIFIÉ ---

int main(int argc, char** argv) {
	if (argc != 3) {
		printf("Usage: %s -t/-e datasets/emnist/emnist-balanced\n", argv[0]);
		return -1;
	}

	NeuralNet net;

	char filename_mapping[128] = { 0 };
	char filename_train[128] = { 0 };
	char filename_test[128] = { 0 }; // Ajout du chemin test

	snprintf(filename_mapping, 128, "%s-mapping.txt", argv[2]);
	snprintf(filename_train, 128, "%s-train.csv", argv[2]);
	snprintf(filename_test, 128, "%s-test.csv", argv[2]); // Construction du chemin test

	uint8 mode = 0;

	if (strcmp("-t", argv[1]) == 0) {
		mode = 1;
	} else if (strcmp("-e", argv[1]) == 0) {
		mode = 2;
	}

	if (mode == 1) {
		// 1. Chargement UNIQUE
		printf("Chargement des données...\n");
		Dataset train_data = load_dataset_in_memory(filename_train);
		if (train_data.count == 0) return 1;

		// 2. Création Réseau
		printf("Création du réseau...\n");
		net = creer_reseau_ocr();

		// 3. Entraînement
		double learning_rate = 0.1;
		int epoch;
		for (epoch = 0; epoch < 5; epoch++) {
			clock_t start = clock();
			printf("--- EPOCH %d ---\n", epoch);

			train_one_epoch_from_ram(&net, &train_data, learning_rate);

			double time_taken = ((double)(clock() - start)) / CLOCKS_PER_SEC;
			printf("Epoque terminée en %.2f secondes.\n", time_taken);

			learning_rate *= 0.8;
		}

		char save_name[64];
		sprintf(save_name, "emnist.neuralnet", epoch);
		saveNeuralNet(&net, save_name);

		free(train_data.images);
	} else if (mode == 2) {
		net = loadNeuralNet("emnist.neuralnet");

		evaluate_network(&net, filename_test);
	}

	// Nettoyage
	if (mode == 1 || mode == 2) {freeNeuralNet(&net);}
	return 0;
}