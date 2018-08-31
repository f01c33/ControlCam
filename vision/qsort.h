#ifndef QSORT_H
#define QSORT_H

struct mask{
    Scalar low;
    Scalar high;
};

// from rosetta code
typedef struct individual_stct{
    struct mask r;
    float fitness;
}individual;

// typedef struct individual {
// 	double max_iterations, cooling, initial_temperature, start, fitness;
// } individual;

// template <class T>
void quick_sort (individual *a, int n) {
    int i, j;
    individual p,t;
    if (n < 2)
        return;
    p = a[n / 2];
    for (i = 0, j = n - 1;; i++, j--) {
        while (a[i].fitness < p.fitness)
            i++;
        while (p.fitness < a[j].fitness)
            j--;
        if (i >= j)
            break;
        t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
    quick_sort(a, i);
    quick_sort(a + i, n - i);
}

#endif // QSORT_H
