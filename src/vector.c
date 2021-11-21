#include <stdio.h>
#include <stdlib.h>

#include "vector.h"

void vector_init(vector *v)
{
    v->size = 0;
    v->items = NULL;
}

void vector_from_arr(vector *v, char *arr, int size)
{
    v->size = size;
    v->items = arr;
}

int vector_size(vector *v)
{
    return v->size;
}

void vector_resize(vector *v, int newSize)
{
    char *items = realloc(v->items, sizeof(char *) * newSize);
    if (items)
    {
        v->items = items;
        v->size = newSize;
    }
}

void vector_push_back(vector *v, char elem)
{
    vector_resize(v, v->size+1);
    v->items[v->size-1] = elem;
}

void vector_push_front(vector *v, char elem)
{
    vector_push_at(v, 0, elem);
}

void vector_push_at(vector *v, int index, char elem)
{
    vector_resize(v, v->size+1);
    for (int i = v->size-1; i > index; i--)
        v->items[i] = v->items[i-1];
    v->items[index] = elem;
}

void vector_set(vector *v, int index, char elem)
{
    if (index >= 0 && index < v->size)
        v->items[index] = elem;
}

char vector_get(vector *v, int index)
{
    if (index >= 0 && index < v->size)
        return v->items[index];
    return -1;
}

void vector_delete(vector *v, int index)
{
    if (index < 0 || index >= v->size)
        return;

    for (int i = index; i < v->size - 1; i++) {
        v->items[i] = v->items[i + 1];
    }

    v->size--;

    if (v->size > 0)
        vector_resize(v, v->size);
}

void vector_free(vector *v)
{
    free(v->items);
}
