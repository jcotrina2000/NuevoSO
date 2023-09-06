#include "hashmap.h"
#include <string.h>
#include <stdlib.h>

int hash(char *key){

	return (int)(key[1] - 'a');

}

void iniciarMap(HashMap *map){

	for(int i = 0; i < HASHMAP_SIZE; i++){
		map->table[i] = NULL;
	}

}

int insertar_wildcard(HashMap *map, char *key, int value, int clave){

	if(map == NULL || key == NULL) return -1;
	if(clave < -1 || clave > 1) return -1;

	int subs = 0;
	for(size_t i = 0; i < HASHMAP_SIZE; i++){
		if(map->table[i] != NULL){

			char temp_t[100] = {0};
			char temp_w[100] = {0};

			strncpy(temp_t, map->table[i]->key, strlen(map->table[i]->key));
			strncpy(temp_w, key, strlen(key));

			char *temp1 = temp_t;
			char *temp2 = temp_w;

			char *t1 = strsep(&temp1, "/");
			char *t2 = strsep(&temp2, "/");

			if(strncmp(t1, t2, strlen(t1)) != 0) continue;

			while(t1 != NULL && t2 != NULL){

				if(strncmp(t1, t2, strlen(t1)) != 0)
		  			if(strncmp(t2, "#", strlen("#")) == 0){

		    				if(clave == 0 || clave == -1){
		      					if(map->table[i]->p1.subs != NULL)
								*(map->table[i]->p1.subs + map->table[i]->p1.size++) = value;
		    				}

						if(clave == 1 || clave == -1){
							if(map->table[i]->p2.subs != NULL)
								*(map->table[i]->p2.subs + map->table[i]->p2.size++) = value;
						}

						subs++;
		  			}

				t1 = strsep(&temp1, "/");
				t2 = strsep(&temp2, "/");

			}

		}	
	}

	if(subs == 0) return -1;

	return 0;
}


int insertar(HashMap *map, char *key, int value, int clave){

	int index = hash(key);
	int initialIndex = index;

	if(map == NULL || key == NULL) return -1;
	if(clave < -1 || clave > 1) return -1;

	while(map->table[index] != NULL){

		if(!strcmp(key, map->table[index]->key)){

			if(clave == 0 || clave == -1){
				if(map->table[index]->p1.subs == NULL) map->table[index]->p1.subs = (int *)malloc(sizeof(int) * 10);
					*(map->table[index]->p1.subs + map->table[index]->p1.size++) = value;
			}

			if(clave == 1 || clave == -1){
				if(map->table[index]->p2.subs == NULL) map->table[index]->p2.subs = (int *)malloc(sizeof(int) * 10);
					*(map->table[index]->p2.subs + map->table[index]->p2.size++) = value;
			}

			return index;

		}

		index = (index + 1) % HASHMAP_SIZE;
		if (index == initialIndex)  return -1;

	}


	HashValue *nuevoItemHash = (HashValue *)malloc(sizeof(HashValue));
	nuevoItemHash->key = key;

	if(clave == 0 || clave == -1){
		nuevoItemHash->p1.subs = (int *)malloc(sizeof(int) * 10);
		*(nuevoItemHash->p1.subs) = value;
		nuevoItemHash->p1.size = 1;
	}

	if(clave == 1 || clave == -1){
		nuevoItemHash->p2.subs = (int *)malloc(sizeof(int) * 10);
		*(nuevoItemHash->p2.subs) = value;
		nuevoItemHash->p2.size = 1;
	} 

	map->table[index] = nuevoItemHash;

	return index;

}

HashValue *buscar(HashMap *map, char *key){

	int index = hash(key);

	int initialIndex = index;
		while (map->table[index] != NULL) {

		if (!strcmp(key, map->table[index]->key)) {
			return map->table[index];
		}

		index = (index + 1) % HASHMAP_SIZE;

		if (index == initialIndex) {
			break; 
		}

	}

	return NULL;

}

void destruir_mapa(HashMap *map){

	for (int i = 0; i < HASHMAP_SIZE; i++) {

		if (map->table[i] != NULL) {

			free(map->table[i]->p1.subs);
			free(map->table[i]->p2.subs);
			free(map->table[i]);

			map->table[i] = NULL;

		}

	}

}
