#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <glib.h>


void print_key_val( gpointer key, gpointer value, gpointer userData ) {
    int _key = GPOINTER_TO_INT( key );
    short* _val = (short*)value;
    printf("key=%d, val=%d\n", _key, _val);
}

int main(int argc, char **argv) {
    GHashTable* hash_table1 = g_hash_table_new(g_direct_hash, g_direct_equal);
    GHashTable* hash_table2 = g_hash_table_new(g_direct_hash, g_direct_equal);
    GHashTable* intersect_hash_table = g_hash_table_new(g_direct_hash, g_direct_equal);
    static struct timespec start, end;

    int l1_len = 1000;
//    int l2_len = 100000000;
    int l2_len = 100000;

    for (int i = 0; i < l1_len; i++) {
        g_hash_table_insert(hash_table1,  GINT_TO_POINTER(i), (short*)1);
    }
    for (int i = l2_len; i > 0; i--) {
        g_hash_table_insert(hash_table2,  GINT_TO_POINTER(i), (short*)1);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int key = 0; key <= g_hash_table_size(hash_table1); key++) {
        if (g_hash_table_lookup_extended(hash_table2, GINT_TO_POINTER(key), NULL, NULL) == (gboolean)TRUE) {
            g_hash_table_insert(intersect_hash_table, GINT_TO_POINTER(key), (short*)1);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (
            (end.tv_sec - start.tv_sec)*1000 + (end.tv_nsec - start.tv_nsec)/(1.0 * 1000000)
    );
    double elapsed_nano = elapsed*1000000;
    double elapsed_ms =  elapsed_nano/1000;

    printf("intersect hash table size = %d, elapsed ms =  %0.3f\n",
            g_hash_table_size(intersect_hash_table), elapsed_ms);

//    GHashTableIter iter;
//    gpointer key, value;
//
//    g_hash_table_iter_init (&iter, intersect_hash_table);
//    while (g_hash_table_iter_next (&iter, &key, &value))
//    {
//        int _key = GPOINTER_TO_INT( key );
//        short* _val = (short*)value;
//        printf("key=%d, val=%d\n", _key, _val);
//    }

//    g_hash_table_foreach( intersect_hash_table, print_key_val, NULL );
    g_hash_table_destroy(hash_table2);
    g_hash_table_destroy(hash_table1);
    g_hash_table_destroy(intersect_hash_table);
    return 0;
}

