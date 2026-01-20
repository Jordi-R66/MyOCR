#include <network.h>
#include "emnist.h"
#include <stdio.h>

NeuralNet creer_reseau_ocr() {
	NeuralNet net = createNeuralNet(PIXEL_COUNT);

	neuralNetAddLayer(&net, 128, ACTIVATION_RELU);
	neuralNetAddLayer(&net, 47, ACTIVATION_SOFTMAX);

	return net;
}

void train_one_epoch(NeuralNet* net, const char* csv_path, double learning_rate) {
	FILE* fp = fopen(csv_path, "r");
	if (!fp) return;

	char buffer[4096];
	EmnistImage raw_img, img_data;

	// Création des vecteurs temporaires (pour éviter de malloc à chaque tour)
	Vector inputVec = createVector(784);
	Vector targetVec = createVector(47); // 47 classes pour Balanced

	int count = 0;
	while (fgets(buffer, sizeof(buffer), fp)) {
		// 1. Parsing CSV (votre fonction précédente)
		if (!parse_csv_line(buffer, &raw_img)) continue;

		fix_emnist_orientation(&raw_img, &img_data);
		// 2. Préparer l'INPUT (Normalisation 0.0 à 1.0)
		// Note: EMNIST est inversé/pivoté, assurez-vous de le corriger si besoin
		// ou d'entraîner sur les données brutes si vous testez sur des données brutes.
		for (int i = 0; i < PIXEL_COUNT; i++) {
			// img_data.pixels est en 2D, ici on suppose qu'on a aplati ou qu'on accède linéairement
			// Attention : cast en double pour la division
			inputVec.data[i] = (Value)(img_data.pixels[i / 28][i % 28]) / 255.0;
		}

		// 3. Préparer le TARGET (One-Hot Encoding)
		// On remet tout à 0
		for (int k = 0; k < 47; k++) targetVec.data[k] = 0.0;
		// On met à 1 l'index correspondant au label
		if (img_data.label < 47) {
			targetVec.data[img_data.label] = 1.0;
		}

		// 4. --- MAGIE DU ML ---

		// A. Propagation avant (Prédiction)
		neuralNetForward(net, &inputVec);

		// B. Rétropropagation (Calcul de l'erreur et des gradients)
		neuralNetBackward(net, &targetVec);

		// C. Mise à jour des poids (Descente de gradient)
		neuralNetUpdate(net, learning_rate);

		count++;
		if (count % 1000 == 0) printf("Images entrainees: %d\n", count);
	}

	// Nettoyage
	deallocVector(&inputVec);
	deallocVector(&targetVec);
	fclose(fp);
}

int predict_character(NeuralNet* net, double pixels[784]) {
	Vector inputVec = createVector(784);

	// Copie et normalisation
	for (int i = 0; i < 784; i++) inputVec.data[i] = pixels[i]; // Supposons déjà normalisé

	// Forward
	VectorPtr output = neuralNetForward(net, &inputVec);

	// Trouver l'index max (Argmax)
	int best_label = 0;
	double max_prob = -1.0;

	for (int i = 0; i < output->size; i++) {
		if (output->data[i] > max_prob) {
			max_prob = output->data[i];
			best_label = i;
		}
	}

	deallocVector(&inputVec); // Attention à ne pas free output car c'est un pointeur vers le cache interne du layer
	return best_label;
}

int main() {
	// 1. Initialisation
	char ascii_map[NUM_CLASSES];
    load_mapping("datasets/emnist/emnist-balanced-mapping.txt", ascii_map);

	printf("Création du réseau...\n");
	NeuralNet net = creer_reseau_ocr();

	// 2. Entraînement
	// On fait plusieurs passes (Epochs) pour améliorer la précision
	double learning_rate = 0.1;
	for (int epoch = 1; epoch <= 3; epoch++) {
		printf("--- EPOCH %d ---\n", epoch);
		train_one_epoch(&net, "datasets/emnist/emnist-balanced-train.csv", learning_rate);

		// On réduit souvent le taux d'apprentissage petit à petit
		learning_rate *= 0.9;

		// Sauvegarde intermédiaire
		char save_name[50];
		sprintf(save_name, "emnist_epoch_%d.neuralnet", epoch);
		saveNeuralNet(&net, save_name);
	}

	// 3. Test Rapide
	// (Ici vous utiliseriez votre fonction load_image ou lire le CSV de test)
	//int prediction = predict_character(&net, pixels_test);
	//printf("Le réseau pense que c'est : %c\n", mapping[prediction]);

	// 4. Nettoyage
	freeNeuralNet(&net);
	return 0;
}