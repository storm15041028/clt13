#include "clt13.h"
#include <aesrand.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

int expect(char * desc, int expected, int recieved);

static int test(ulong flags, ulong nzs, ulong lambda, ulong kappa)
{
    srand(time(NULL));

    clt_state mmap, mmap_;
    clt_pp pp_, pp;

    aes_randstate_t rng;
    aes_randinit(rng);

    int pows [nzs];
    for (ulong i = 0; i < nzs; i++) pows[i] = 1;

    FILE *mmap_f = tmpfile();
    if (mmap_f == NULL) {
        fprintf(stderr, "Couldn't open test.map!\n");
        exit(1);
    }

    FILE *pp_f = tmpfile();
    if (pp_f == NULL) {
        fprintf(stderr, "Couldn't open test.pp!\n");
        exit(1);
    }

    // test initialization & serialization
    clt_state_init(&mmap_, kappa, lambda, nzs, pows, flags, rng);

    if (clt_state_fsave(mmap_f, &mmap_) != 0) {
        fprintf(stderr, "clt_state_fsave failed!\n");
        exit(1);
    }
    rewind(mmap_f);
    clt_state_clear(&mmap_);
    if (clt_state_fread(mmap_f, &mmap) != 0) {
        fprintf(stderr, "clt_state_fread failed!\n");
        exit(1);
    }

    clt_pp_init(&pp_, &mmap);

    if (clt_pp_fsave(pp_f, &pp_) != 0) {
        fprintf(stderr, "clt_pp_fsave failed!\n");
        exit(1);
    }
    rewind(pp_f);
    clt_pp_clear(&pp_);
    if (clt_pp_fread(pp_f, &pp) != 0) {
        fprintf(stderr, "clt_pp_fread failed!\n");
        exit(1);
    }

    mpz_t x [1];
    mpz_init_set_ui(x[0], 0);
    while (mpz_cmp_ui(x[0], 0) <= 0) {
        mpz_set_ui(x[0], rand());
        mpz_mod(x[0], x[0], mmap.gs[0]);
    }
    gmp_printf("x = %Zd\n", x[0]);

    mpz_t zero [1];
    mpz_init_set_ui(zero[0], 0);

    mpz_t one [1];
    mpz_init_set_ui(one[0], 1);

    int top_level [nzs];
    for (ulong i = 0; i < nzs; i++) {
        top_level[i] = 1;
    }

    mpz_t x0, x1, xp;
    mpz_inits(x0, x1, xp, NULL);
    clt_encode(x0, &mmap, 1, zero, top_level, rng);
    clt_encode(x1, &mmap, 1, zero, top_level, rng);
    mpz_add(xp, x0, x1);
    mpz_mod(xp, xp, mmap.x0);
    int ok = expect("is_zero(0 + 0)", 1, clt_is_zero(&pp, xp));

    clt_encode(x0, &mmap, 1, zero, top_level, rng);
    clt_encode(x1, &mmap, 1, one,  top_level, rng);
    mpz_add(xp, x0, x1);
    mpz_mod(xp, xp, mmap.x0);
    ok &= expect("is_zero(0 + 1)", 0, clt_is_zero(&pp, xp));

    clt_encode(x0, &mmap, 1, zero, top_level, rng);
    clt_encode(x1, &mmap, 1, x,    top_level, rng);
    mpz_add(xp, x0, x1);
    mpz_mod(xp, xp, mmap.x0);
    ok &= expect("is_zero(0 + x)", 0, clt_is_zero(&pp, xp));

    clt_encode(x0, &mmap, 1, x, top_level, rng);
    clt_encode(x1, &mmap, 1, x, top_level, rng);
    mpz_sub(xp, x0, x1);
    mpz_mod(xp, xp, mmap.x0);
    ok &= expect("is_zero(x - x)", 1, clt_is_zero(&pp, xp));

    clt_encode(x0, &mmap, 1, zero, top_level, rng);
    clt_encode(x1, &mmap, 1, x,    top_level, rng);
    mpz_sub(xp, x0, x1);
    mpz_mod(xp, xp, mmap.x0);
    ok &= expect("is_zero(0 - x)", 0, clt_is_zero(&pp, xp));

    clt_encode(x0, &mmap, 1, one,  top_level, rng);
    clt_encode(x1, &mmap, 1, zero, top_level, rng);
    mpz_sub(xp, x0, x1);
    mpz_mod(xp, xp, mmap.x0);
    ok &= expect("is_zero(1 - 0)", 0, clt_is_zero(&pp, xp));

    mpz_t three[1];
    mpz_init(three[0]);
    mpz_set_ui(three[0], 3);
    clt_encode(x0, &mmap, 1, one,  top_level, rng);
    clt_encode(x1, &mmap, 1, three, top_level, rng);
    mpz_mul_ui(x0, x0, 3);
    mpz_mod(x0, x0, mmap.x0);
    mpz_sub(xp, x0, x1);
    mpz_mod(xp, xp, mmap.x0);
    ok &= expect("is_zero(3*[1] - [3])", 1, clt_is_zero(&pp, xp));

    int ix0 [nzs];
    int ix1 [nzs];
    for (ulong i = 0; i < nzs; i++) {
        if (i < nzs / 2) {
            ix0[i] = 1;
            ix1[i] = 0;
        } else {
            ix0[i] = 0;
            ix1[i] = 1;
        }
    }
    clt_encode(x0, &mmap, 1, x   , ix0, rng);
    clt_encode(x1, &mmap, 1, zero, ix1, rng);
    mpz_mul(xp, x0, x1);
    mpz_mod(xp, xp, mmap.x0);
    ok &= expect("is_zero(x * 0)", 1, clt_is_zero(&pp, xp));

    clt_encode(x0, &mmap, 1, x  , ix0, rng);
    clt_encode(x1, &mmap, 1, one, ix1, rng);
    mpz_mul(xp, x0, x1);
    mpz_mod(xp, xp, mmap.x0);
    ok &= expect("is_zero(x * 1)", 0, clt_is_zero(&pp, xp));

    clt_encode(x0, &mmap, 1, x, ix0, rng);
    clt_encode(x1, &mmap, 1, x, ix1, rng);
    mpz_mul(xp, x0, x1);
    mpz_mod(xp, xp, mmap.x0);
    ok &= expect("is_zero(x * x)", 0, clt_is_zero(&pp, xp));

    // zimmerman-like test

    mpz_t c;
    mpz_t in0 [2];
    mpz_t in1 [2];
    mpz_t cin [2];

    mpz_inits(c, in0[0], in0[1], in1[0], in1[1], cin[0], cin[1], NULL);

    mpz_urandomb_aes(in1[0], rng, lambda);
    mpz_mod(in1[0], in1[0], mmap.gs[0]);

    mpz_set_ui(in0[0], 0);
    mpz_set_ui(cin[0], 0);

    mpz_urandomb_aes(in0[1], rng, 16);
    mpz_urandomb_aes(in1[1], rng, 16);
    mpz_mul(cin[1], in0[1], in1[1]);

    clt_encode(x0, &mmap, 2, in0, ix0, rng);
    clt_encode(x1, &mmap, 2, in1, ix1, rng);
    clt_encode(c,  &mmap, 2, cin, top_level, rng);

    mpz_mul(xp, x0, x1);
    mpz_mod(xp, xp, mmap.x0);

    mpz_sub(xp, xp, c);
    mpz_mod(xp, xp, mmap.x0);

    ok &= expect("[Z] is_zero(0 * x)", 1, clt_is_zero(&pp, xp));

    mpz_set_ui(in0[0], 1);
    mpz_set_ui(in1[0], 1);
    mpz_set_ui(cin[0], 0);

    mpz_urandomb_aes(in0[0], rng, lambda);
    mpz_mod(in0[0], in0[0], mmap.gs[0]);

    mpz_urandomb_aes(in1[0], rng, lambda);
    mpz_mod(in1[0], in1[0], mmap.gs[0]);

    mpz_urandomb_aes(in0[1], rng, 16);
    mpz_urandomb_aes(in1[1], rng, 16);
    mpz_mul(cin[1], in0[1], in1[1]);

    clt_encode(x0, &mmap, 2, in0, ix0, rng);
    clt_encode(x1, &mmap, 2, in1, ix1, rng);
    clt_encode(c,  &mmap, 2, cin, top_level, rng);

    mpz_mul(xp, x0, x1);
    mpz_mod(xp, xp, mmap.x0);

    mpz_sub(xp, xp, c);
    mpz_mod(xp, xp, mmap.x0);

    ok &= expect("[Z] is_zero(x * y)", 0, clt_is_zero(&pp, xp));
    clt_state_clear(&mmap);
    clt_pp_clear(&pp);
    mpz_clears(c, x0, x1, xp, x[0], zero[0], one[0], in0[0], in0[1], in1[0], in1[1], cin[0], cin[1], NULL);
    return !ok;
}

int main(void)
{
    ulong default_flags = CLT_FLAG_NONE | CLT_FLAG_VERBOSE;
    ulong flags;
    printf("* No optimizations\n");
    flags = default_flags;
    if (test(flags, 10, 30, 2) == 1)
        return 1;
    printf("* CRT tree\n");
    flags = default_flags | CLT_FLAG_OPT_CRT_TREE;
    if (test(flags, 10, 30, 2) == 1)
        return 1;
    printf("* CRT tree + parallel encode\n");
    flags = default_flags | CLT_FLAG_OPT_CRT_TREE | CLT_FLAG_OPT_PARALLEL_ENCODE;
    if (test(flags, 10, 30, 2) == 1)
        return 1;
    printf("* CRT tree + composite ps\n");
    flags = default_flags | CLT_FLAG_OPT_CRT_TREE | CLT_FLAG_OPT_COMPOSITE_PS;
    if (test(flags, 10, 30, 2) == 1)
        return 1;
    printf("* CRT tree + parallel encode + composite ps\n");
    flags = default_flags | CLT_FLAG_OPT_CRT_TREE | CLT_FLAG_OPT_PARALLEL_ENCODE | CLT_FLAG_OPT_COMPOSITE_PS;
    if (test(flags, 10, 30, 2) == 1)
        return 1;
    return 0;
}

int expect(char * desc, int expected, int recieved)
{
    if (expected != recieved) {
        printf("\033[1;41m");
    }
    printf("%s = %d", desc, recieved);
    if (expected != recieved) {
        printf("\033[0m");
    }
    puts("");
    return expected == recieved;
}
