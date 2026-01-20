#!/usr/bin/env bash
set -euo pipefail

SCRIPT_NAME="$(basename "$0")"

usage() {
	cat <<EOF
Usage:
  $SCRIPT_NAME <command> [options]

Commands:
  add <url> <path>        Ajouter un submodule
  remove <path>           Supprimer proprement un submodule
  update [path]           Mettre à jour un submodule (ou tous)
  init                    Initialiser tous les submodules
  status                  Afficher l'état des submodules
  sync                    Synchroniser les URLs des submodules
  foreach <cmd>           Exécuter une commande dans chaque submodule

Examples:
  $SCRIPT_NAME add https://github.com/foo/bar.git libs/bar
  $SCRIPT_NAME update
  $SCRIPT_NAME update libs/bar
  $SCRIPT_NAME remove libs/bar
  $SCRIPT_NAME foreach "git status"

EOF
	exit 1
}

require_git_repo() {
	if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
		echo "Erreur: ce script doit être exécuté dans un dépôt Git." >&2
		exit 1
	fi
}

cmd_add() {
	local url="$1"
	local path="$2"

	echo "Ajout du submodule $url -> $path"
	git submodule add "$url" "$path"

	# AJOUT : Initialisation récursive des sous-modules du module ajouté
	echo "Initialisation récursive des dépendances de $path"
	git submodule update --init --recursive "$path"

	git commit -m "Add submodule $path"
}

cmd_remove() {
	local path="$1"

	echo "Suppression du submodule $path"

	git submodule deinit -f "$path"
	rm -rf ".git/modules/$path"
	git rm -f "$path"

	git commit -m "Remove submodule $path"
}

cmd_update() {
	if [[ $# -eq 1 ]]; then
		local path="$1"
		echo "Mise à jour du submodule $path"
		# MODIFICATION : Ajout de --recursive
		git submodule update --remote --recursive "$path"
	else
		echo "Mise à jour de tous les submodules"
		git submodule update --remote --recursive
	fi

	git commit -am "Update submodules" || echo "Aucun changement à valider"
}

cmd_init() {
	echo "Initialisation des submodules"
	git submodule update --init --recursive
}

cmd_status() {
	git submodule status --recursive
}

cmd_sync() {
	echo "Synchronisation des URLs des submodules"
	git submodule sync --recursive
}

cmd_foreach() {
	local cmd="$1"
	git submodule foreach --recursive "$cmd"
}

### MAIN ###

require_git_repo

[[ $# -lt 1 ]] && usage

command="$1"
shift

case "$command" in
	add)
		[[ $# -ne 2 ]] && usage
		cmd_add "$@"
		;;
	remove)
		[[ $# -ne 1 ]] && usage
		cmd_remove "$@"
		;;
	update)
		cmd_update "$@"
		;;
	init)
		cmd_init
		;;
	status)
		cmd_status
		;;
	sync)
		cmd_sync
		;;
	foreach)
		[[ $# -ne 1 ]] && usage
		cmd_foreach "$@"
		;;
	*)
		usage
		;;
esac
