#ifndef HASM_MAP_H

#define HASHMAP_SIZE 26

typedef struct {
	int *subs;
	int size;
} Partition;

typedef struct{
  	Partition p1;
  	Partition p2;
  	char *key;
} HashValue;

typedef struct{
  	HashValue *table[HASHMAP_SIZE];
} HashMap;

int hash(char *key);
void iniciarMap(HashMap *map);
int insertar(HashMap *map, char *key, int value, int clave);
int insertar_wildcard(HashMap *map, char *key, int value, int clave);
HashValue *buscar(HashMap *map, char *key);
void destruir_mapa(HashMap *map);

#endif // !HASM_MAP_H
