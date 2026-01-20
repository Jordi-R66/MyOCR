#include <network.h>
#include "emnist.h"
#include <stdio.h>
#include <stdlib.h> // Pour malloc/free si besoin

// --- VOS FONCTIONS EXISTANTES (INCHANGÉES) ---

NeuralNet creer_reseau_ocr() {
	NeuralNet net = createNeuralNet(PIXEL_COUNT);
	neuralNetAddLayer(&net, 128, ACTIVATION_RELU);
	neuralNetAddLayer(&net, 47, ACTIVATION_SOFTMAX);
	return net;
}

void train_one_epoch(NeuralNet* net, const char* csv_path, double learning_rate) {
	FILE* fp = fopen(csv_path, "r");
	if (!fp) {
		perror("Erreur ouverture fichier train");
		return;
	}

	char buffer[4096];
	EmnistImage raw_img, img_data;

	Vector inputVec = createVector(784);
	Vector targetVec = createVector(47);

	int count = 0;
	while (fgets(buffer, sizeof(buffer), fp)) {
		if (!parse_csv_line(buffer, &raw_img)) continue;

		fix_emnist_orientation(&raw_img, &img_data);

		for (int i = 0; i < PIXEL_COUNT; i++) {
			inputVec.data[i] = (Value)(img_data.pixels[i / 28][i % 28]) / 255.0;
		}

		for (int k = 0; k < 47; k++) targetVec.data[k] = 0.0;
		if (img_data.label < 47) {
			targetVec.data[img_data.label] = 1.0;
		}

		neuralNetForward(net, &inputVec);
		neuralNetBackward(net, &targetVec);
		neuralNetUpdate(net, learning_rate);

		count++;
		if (count % 2000 == 0) printf("Images entrainees: %d\r", count);
	}
	printf("\n");

	deallocVector(&inputVec);
	deallocVector(&targetVec);
	fclose(fp);
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
	if (argc != 2) {
		printf("Usage: %s <chemin_vers_prefixe_dataset>\n", argv[0]);
		return -1;
	}

	char filename_mapping[128] = { 0 };
	char filename_train[128] = { 0 };
	char filename_test[128] = { 0 }; // Ajout du chemin test

	snprintf(filename_mapping, 128, "%s-mapping.txt", argv[1]);
	snprintf(filename_train, 128, "%s-train.csv", argv[1]);
	snprintf(filename_test, 128, "%s-test.csv", argv[1]); // Construction du chemin test

	// 1. Initialisation
	char ascii_map[NUM_CLASSES];
	load_mapping(filename_mapping, ascii_map);

	printf("Création du réseau...\n");
	NeuralNet net = creer_reseau_ocr();

	// 2. Entraînement
	double learning_rate = 0.1;
	int epoch;
	for (epoch = 1; epoch <= 3; epoch++) {
		printf("--- EPOCH %d ---\n", epoch);
		train_one_epoch(&net, filename_train, learning_rate);

		learning_rate *= 0.8; // Decay léger
	}

	// Sauvegarde
	char save_name[50] = { 0 };
	sprintf(save_name, "emnist_epoch_%d.neuralnet", epoch);
	saveNeuralNet(&net, save_name);

	// 3. Évaluation sur le dataset de Test
	evaluate_network(&net, filename_test);

	// 4. Test sur une image BMP externe (Optionnel)
	printf("\n--- TEST IMAGE EXTERNE ---\n");
	VectorPtr bmpInput = load_bmp_image("test.bmp");
	if (bmpInput) {
		VectorPtr output = neuralNetForward(&net, bmpInput);

		int best = 0;
		double maxProb = -1.0;
		for (uint i = 0; i < output->size; i++) {
			if (output->data[i] > maxProb) { maxProb = output->data[i]; best = i; }
		}

		printf("Image 'test.bmp' reconnue comme : '%c' (Prob: %.2f%%)\n",
			ascii_map[best], maxProb * 100.0);

		// Nettoyage spécifique BMP
		deallocVector(bmpInput);
		free(bmpInput);
	}
	else {
		printf("Aucun fichier 'test.bmp' trouvé. Placez une image 28x28 pour tester.\n");
	}

	// 5. Nettoyage final
	freeNeuralNet(&net);
	return 0;
}