#ifndef VECTOR_H
#define VECTOR_H

typedef struct {
    char *items;
    int size;
} vector;

void vector_init(vector *v);
void vector_from_arr(vector *v, char *arr, int size);
int vector_size(vector *v);
void vector_push_back(vector *v, char elem);
void vector_push_front(vector *v, char elem);
void vector_push_at(vector *v, int index, char elem);
void vector_set(vector *v, int index, char elem);
char vector_get(vector *v, int index);
void vector_delete(vector *v, int index);
void vector_free(vector *v);

#endif